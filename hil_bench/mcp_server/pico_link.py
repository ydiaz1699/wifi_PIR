"""Enlace serial hacia la Raspberry Pi Pico (las "manos").

Encapsula el puerto serie con ``pyserial``: abrir/cerrar perezosamente, enviar
un comando de texto y leer la respuesta ``OK ...`` / ``ERR ...`` con timeout, y
un ``ping()`` para comprobar que la Pico está viva.

Diseño robusto: el constructor NO abre el puerto (apertura perezosa), de modo
que el MCP server se puede instanciar sin hardware presente. Los fallos de
puerto se traducen a excepciones claras del propio módulo
(``PicoNoDisponible``, ``PicoTimeout``) en vez de colgar o propagar trazas de
bajo nivel sin contexto.
"""

from __future__ import annotations

import threading

try:
    import serial  # pyserial
except ImportError:  # pragma: no cover - pyserial es dependencia declarada
    serial = None


class PicoError(Exception):
    """Error base del enlace con la Pico."""


class PicoNoDisponible(PicoError):
    """El puerto de la Pico no se pudo abrir (no conectada, en uso, etc.)."""


class PicoTimeout(PicoError):
    """La Pico no respondió dentro del timeout esperado."""


class PicoLink:
    """Gestiona el puerto serie hacia la Pico.

    Parámetros
    ----------
    port:
        Nombre del puerto (p.ej. ``"COM5"`` en Windows). Puede ser ``None``
        para instanciar sin puerto configurado (el enlace quedará "no
        disponible" hasta que se asigne uno).
    baudrate:
        Velocidad del serial de la Pico (por defecto 115200).
    timeout:
        Timeout de lectura en segundos para esperar la respuesta de un comando.
    """

    def __init__(self, port: str | None, baudrate: int = 115200, timeout: float = 2.0):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self._ser = None
        # Serializa el ciclo escribir-comando/leer-respuesta: evita que dos
        # llamadas concurrentes mezclen sus respuestas en el mismo puerto.
        self._lock = threading.Lock()

    # -- ciclo de vida del puerto -------------------------------------------------

    def abrir(self):
        """Abre el puerto si aún no lo está. Idempotente.

        Lanza ``PicoNoDisponible`` si pyserial no está instalado, si no hay
        puerto configurado o si el sistema no puede abrirlo.
        """
        if self._ser is not None and getattr(self._ser, "is_open", False):
            return
        if serial is None:
            raise PicoNoDisponible("pyserial no está instalado")
        if not self.port:
            raise PicoNoDisponible("no hay puerto configurado para la Pico (PICO_PORT)")
        try:
            self._ser = serial.Serial(
                self.port,
                self.baudrate,
                timeout=self.timeout,
                write_timeout=self.timeout,
            )
        except Exception as exc:  # serial.SerialException y afines
            self._ser = None
            raise PicoNoDisponible(
                "no se pudo abrir el puerto de la Pico %r: %s" % (self.port, exc)
            ) from exc

    def cerrar(self):
        """Cierra el puerto si está abierto. Nunca lanza."""
        if self._ser is not None:
            try:
                self._ser.close()
            except Exception:
                pass
            self._ser = None

    @property
    def abierto(self) -> bool:
        """¿El puerto está abierto?"""
        return self._ser is not None and getattr(self._ser, "is_open", False)

    # -- comandos ------------------------------------------------------------------

    def enviar(self, comando: str) -> str:
        """Envía ``comando`` a la Pico y devuelve su respuesta (una línea).

        Abre el puerto si hace falta. Lanza ``PicoNoDisponible`` si el puerto
        no se puede usar y ``PicoTimeout`` si la Pico no responde a tiempo.
        """
        with self._lock:
            self.abrir()
            linea = (comando.strip() + "\n").encode("ascii", "ignore")
            try:
                self._ser.reset_input_buffer()
                self._ser.write(linea)
                self._ser.flush()
                cruda = self._ser.readline()
            except Exception as exc:
                # Un fallo de E/S suele significar que la Pico se desconectó.
                self.cerrar()
                raise PicoNoDisponible(
                    "fallo de E/S hablando con la Pico: %s" % exc
                ) from exc
            if not cruda:
                raise PicoTimeout(
                    "la Pico no respondió a %r en %.1fs" % (comando, self.timeout)
                )
            return cruda.decode("ascii", "ignore").strip()

    def ping(self) -> str:
        """Comprueba que la Pico está viva. Devuelve su respuesta (``PONG v...``).

        Propaga ``PicoNoDisponible`` / ``PicoTimeout`` si no hay respuesta.
        """
        return self.enviar("PING")

    # -- atajos del protocolo genérico por GPIO -----------------------------------

    def pulse_gpio(self, gpio: int, ms: int) -> str:
        """Pulso: pone ``gpio`` ALTO durante ``ms`` y lo baja (``PULSE``)."""
        return self.enviar("PULSE %d %d" % (int(gpio), int(ms)))

    def set_gpio(self, gpio: int, alto: bool) -> str:
        """Fija ``gpio`` en ALTO (estimulado) o BAJO (reposo), sostenido (``SET``)."""
        return self.enviar("SET %d %d" % (int(gpio), 1 if alto else 0))

    def safe(self, gpios: "list[int] | None" = None) -> str:
        """Lleva a reposo los ``gpios`` indicados (o todos los conocidos) (``SAFE``)."""
        if gpios:
            return self.enviar("SAFE " + " ".join(str(int(g)) for g in gpios))
        return self.enviar("SAFE")

    # -- soporte para 'with' -------------------------------------------------------

    def __enter__(self) -> "PicoLink":
        self.abrir()
        return self

    def __exit__(self, *exc):
        self.cerrar()
