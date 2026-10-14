"""Lectura del monitor serial del dispositivo bajo prueba (los "oídos" del banco).

Abre el puerto serie del DUT (dispositivo bajo prueba) en modo lectura a su
baudrate y, en un hilo de fondo, acumula las últimas N líneas en un buffer
circular, cada una con su timestamp. Permite recuperar las líneas aparecidas en
una ventana de tiempo reciente para correlacionar un estímulo (acción de las
"manos") con la reacción del DUT (lo que imprime: flanco leído, antirrebote,
evento encolado...).

El DUT es cualquier dispositivo embebido bajo prueba; en el ejemplo wifi_PIR es
un D1 mini (ESP8266), pero el banco no asume nada de su firmware.

Robusto ante desconexión: el hilo tolera errores de lectura (marca el puerto
como caído y sigue reintentando sin romper el proceso). La apertura es
perezosa, de modo que el MCP server se puede instanciar sin hardware presente.
"""

from __future__ import annotations

import threading
import time
from collections import deque

try:
    import serial  # pyserial
except ImportError:  # pragma: no cover - pyserial es dependencia declarada
    serial = None


class DutSerial:
    """Lee el serial del DUT en segundo plano y conserva las últimas líneas.

    Parámetros
    ----------
    port:
        Puerto del DUT (p.ej. ``"COM6"``). Puede ser ``None``.
    baudrate:
        Velocidad (por defecto 115200).
    max_lineas:
        Capacidad del buffer circular de líneas recientes.
    """

    def __init__(self, port: str | None, baudrate: int = 115200, max_lineas: int = 500):
        self.port = port
        self.baudrate = baudrate
        self._ser = None
        # Cada elemento es (timestamp_monotonic, texto_linea).
        self._buffer: "deque[tuple[float, str]]" = deque(maxlen=max_lineas)
        self._lock = threading.Lock()
        self._hilo: threading.Thread | None = None
        self._parar = threading.Event()
        # Último motivo de fallo de lectura (para diagnósticos).
        self.ultimo_error: str | None = None

    # -- ciclo de vida -------------------------------------------------------------

    def iniciar(self) -> bool:
        """Abre el puerto y arranca el hilo lector. Idempotente.

        Devuelve True si quedó leyendo; False si no se pudo abrir (sin lanzar,
        para que el server tolere la ausencia de hardware). El motivo queda en
        ``ultimo_error``.
        """
        if self._hilo is not None and self._hilo.is_alive():
            return self._ser is not None
        if serial is None:
            self.ultimo_error = "pyserial no está instalado"
            return False
        if not self.port:
            self.ultimo_error = "no hay puerto configurado para el DUT (DUT_PORT)"
            return False
        try:
            self._ser = serial.Serial(self.port, self.baudrate, timeout=0.5)
        except Exception as exc:
            self._ser = None
            self.ultimo_error = "no se pudo abrir el puerto del DUT %r: %s" % (
                self.port,
                exc,
            )
            return False
        self.ultimo_error = None
        self._parar.clear()
        self._hilo = threading.Thread(target=self._bucle_lectura, daemon=True)
        self._hilo.start()
        return True

    def detener(self):
        """Para el hilo lector y cierra el puerto. Nunca lanza."""
        self._parar.set()
        hilo = self._hilo
        if hilo is not None and hilo.is_alive():
            hilo.join(timeout=1.5)
        self._hilo = None
        if self._ser is not None:
            try:
                self._ser.close()
            except Exception:
                pass
            self._ser = None

    @property
    def activo(self) -> bool:
        """¿Hay un hilo leyendo el puerto ahora mismo?"""
        return self._hilo is not None and self._hilo.is_alive() and self._ser is not None

    # -- hilo de fondo -------------------------------------------------------------

    def _bucle_lectura(self):
        """Lee líneas del DUT hasta que se pida parar o falle el puerto."""
        while not self._parar.is_set():
            try:
                cruda = self._ser.readline()
            except Exception as exc:
                # Desconexión o error de E/S: marca el puerto caído y sale del
                # bucle sin tumbar el proceso. Un nuevo ``iniciar()`` reintenta.
                self.ultimo_error = "lectura del DUT interrumpida: %s" % exc
                try:
                    self._ser.close()
                except Exception:
                    pass
                self._ser = None
                return
            if not cruda:
                # Timeout de lectura sin datos: normal, sigue esperando.
                continue
            texto = cruda.decode("utf-8", "replace").rstrip("\r\n")
            if texto:
                with self._lock:
                    self._buffer.append((time.monotonic(), texto))

    # -- consulta ------------------------------------------------------------------

    def lineas_recientes(self, ventana_ms: int = 2000) -> list[str]:
        """Devuelve las líneas aparecidas en los últimos ``ventana_ms`` ms."""
        corte = time.monotonic() - (ventana_ms / 1000.0)
        with self._lock:
            return [texto for ts, texto in self._buffer if ts >= corte]

    def lineas_desde(self, desde_monotonic: float) -> list[str]:
        """Devuelve las líneas con timestamp posterior a ``desde_monotonic``.

        Útil para correlacionar: se toma ``time.monotonic()`` justo antes del
        estímulo y luego se piden las líneas aparecidas desde ese instante.
        """
        with self._lock:
            return [texto for ts, texto in self._buffer if ts > desde_monotonic]

    def marca_tiempo(self) -> float:
        """Instante de referencia (``time.monotonic()``) para usar con ``lineas_desde``."""
        return time.monotonic()
