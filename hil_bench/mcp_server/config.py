"""Carga y validación de la configuración declarativa del banco (``bench.yaml``).

Un banco HIL concreto se describe en un archivo YAML que lista los *canales*:
las entradas físicas del dispositivo bajo prueba (DUT) que el banco puede
accionar. El core del banco razona en primitivas de actuador genéricas
(``boton``, ``interruptor``), no en conceptos de dominio: 'timbre', 'pir',
'reset'... son solo ETIQUETAS que el usuario asigna a cada canal en su YAML.
Así la herramienta sirve para cualquier proyecto embebido; wifi_PIR es solo un
ejemplo (ver ``examples/wifi_pir/bench.yaml``).

Localización del YAML
---------------------
El server busca su configuración en la variable de entorno
``HIL_BENCH_CONFIG`` (ruta al archivo). Si no está definida o el archivo no
existe, el banco arranca en MODO DEGRADADO: no hay canales cargados, pero el
server se instancia y las herramientas informan con claridad de que no hay
banco cargado en vez de romper.

Puertos serie
-------------
``pico_port`` y ``dut_port`` pueden fijarse en el YAML o dejarse en ``null``.
Las variables de entorno ``PICO_PORT`` y ``DUT_PORT`` tienen PRIORIDAD sobre lo
que diga el YAML, de modo que la misma configuración sirve en varias máquinas
sin editar el archivo. ``baudrate`` admite override por ``HIL_BAUDRATE``.

Esquema YAML (ver ``examples/wifi_pir/bench.yaml``)
---------------------------------------------------
    nombre: wifi_PIR            # opcional, nombre del banco (informativo)
    pico_port: null             # o COMx; env PICO_PORT tiene prioridad
    dut_port: null              # o COMx; env DUT_PORT tiene prioridad
    baudrate: 115200
    canales:
      - etiqueta: timbre        # nombre libre del canal en este proyecto
        tipo: boton             # boton | interruptor  (analogico: futuro)
        gpio_pico: 2            # GP2 de la Pico
        polaridad: activo_bajo  # activo_alto | activo_bajo (semántica del DUT)
        antirrebote_ms: 800     # informativo: cuánto esperar entre estímulos
        descripcion: '...'      # opcional
"""

from __future__ import annotations

import os
from dataclasses import dataclass, field

try:
    import yaml  # pyyaml
except ImportError:  # pragma: no cover - pyyaml es dependencia declarada
    yaml = None


# Tipos de actuador soportados por el core. 'analogico' queda DISEÑADO pero no
# implementado todavía (fijaría un nivel vía DAC/pot digital en el futuro); por
# eso no está en el conjunto de tipos aceptados al validar.
TIPOS_VALIDOS = ("boton", "interruptor")
TIPOS_FUTUROS = ("analogico",)

POLARIDADES_VALIDAS = ("activo_alto", "activo_bajo")

# Antirrebote informativo por defecto si un canal no lo declara (ms).
ANTIRREBOTE_POR_DEFECTO_MS = 200


class ConfigError(Exception):
    """La configuración del banco (``bench.yaml``) es inválida."""


@dataclass
class Canal:
    """Un canal: una entrada física concreta del DUT que el banco acciona.

    Atributos
    ---------
    etiqueta:
        Nombre libre del canal en este proyecto (p.ej. 'timbre', 'pir',
        'reset'). Es la clave con la que el LLM se refiere al canal.
    tipo:
        Primitiva de actuador: ``boton`` (momentáneo, se pulsa y se suelta) o
        ``interruptor`` (sostenido on/off).
    gpio_pico:
        Número de GPIO de la Pico que acciona el optoacoplador de este canal.
    polaridad:
        Semántica del DUT: ``activo_alto`` (el DUT lee 'activo' cuando su pin
        va a nivel ALTO) o ``activo_bajo`` (activo en nivel BAJO). Es
        informativa a nivel de GPIO de la Pico (ver nota de polaridad abajo),
        pero describe el cableado del canal y guía al LLM.
    antirrebote_ms:
        Antirrebote del DUT en ms, informativo: le dice al LLM cuánto esperar
        entre estímulos y se usa para derivar la ventana de correlación.
    descripcion:
        Texto libre que ayuda al LLM a entender el canal.
    """

    etiqueta: str
    tipo: str
    gpio_pico: int
    polaridad: str = "activo_alto"
    antirrebote_ms: int = ANTIRREBOTE_POR_DEFECTO_MS
    descripcion: str = ""

    def como_dict(self) -> dict:
        """Representación serializable (la que devuelven las herramientas MCP)."""
        return {
            "etiqueta": self.etiqueta,
            "tipo": self.tipo,
            "gpio_pico": self.gpio_pico,
            "polaridad": self.polaridad,
            "antirrebote_ms": self.antirrebote_ms,
            "descripcion": self.descripcion,
        }


@dataclass
class BenchConfig:
    """Configuración completa de un banco HIL.

    Nota sobre polaridad y GPIO de la Pico
    --------------------------------------
    La convención FÍSICA del banco (ver ``docs/DISENO_BANCO_HIL.md``) es:
    GPIO de la Pico en ALTO = LED del opto encendido = opto activo = entrada
    del DUT ESTIMULADA; GPIO en BAJO = opto apagado = canal en REPOSO. Esto
    vale para AMBAS polaridades porque el cableado de cada canal (pull-up o
    pull-down en el pin del DUT, y a qué conmuta el fototransistor) se diseña
    para que "opto activo" signifique "entrada estimulada" en ese canal
    concreto. Por eso ``estimular``->GPIO ALTO y ``reposo``->GPIO BAJO para
    todo canal, y la ``polaridad`` describe la semántica del DUT y el cableado,
    no invierte el GPIO de la Pico.

    El estado SEGURO/REPOSO de todo canal es siempre "no estimulado" = GPIO en
    BAJO (opto apagado). Esto respeta la precaución GPIO0/D3: un canal
    activo_bajo sobre un pin boot-strap nunca queda activo (su opto apagado
    deja el pin en alto por el pull-up) durante el reset del DUT.
    """

    nombre: str = "sin-nombre"
    pico_port: str | None = None
    dut_port: str | None = None
    baudrate: int = 115200
    canales: list[Canal] = field(default_factory=list)
    # Ruta del YAML cargado (None si se arrancó en modo degradado).
    origen: str | None = None
    # Motivo por el que no hay banco cargado (None si cargó bien).
    nota: str | None = None

    @property
    def cargado(self) -> bool:
        """¿Hay un banco realmente cargado (había YAML válido)?"""
        return self.origen is not None

    def canal(self, etiqueta: str) -> Canal | None:
        """Devuelve el canal con esa etiqueta, o ``None`` si no existe."""
        for c in self.canales:
            if c.etiqueta == etiqueta:
                return c
        return None

    def etiquetas(self) -> list[str]:
        """Lista de etiquetas de canal definidas."""
        return [c.etiqueta for c in self.canales]

    def gpios(self) -> list[int]:
        """GPIO de la Pico usados por los canales (para el SAFE al conectar)."""
        return [c.gpio_pico for c in self.canales]


def _normaliza_puerto(valor) -> str | None:
    """Convierte '', 'null', None a None; deja un COMx tal cual."""
    if valor is None:
        return None
    texto = str(valor).strip()
    if not texto or texto.lower() == "null":
        return None
    return texto


def _valida_canal(bruto: dict, indice: int) -> Canal:
    """Valida un canal del YAML y lo convierte en ``Canal``. Lanza ConfigError."""
    if not isinstance(bruto, dict):
        raise ConfigError("canal #%d: debe ser un mapa (clave: valor)" % indice)

    etiqueta = bruto.get("etiqueta")
    if not etiqueta or not isinstance(etiqueta, str):
        raise ConfigError("canal #%d: falta 'etiqueta' (texto)" % indice)

    tipo = bruto.get("tipo")
    if tipo in TIPOS_FUTUROS:
        raise ConfigError(
            "canal %r: el tipo %r está diseñado pero aún no implementado"
            % (etiqueta, tipo)
        )
    if tipo not in TIPOS_VALIDOS:
        raise ConfigError(
            "canal %r: 'tipo' debe ser uno de %s (recibido %r)"
            % (etiqueta, " / ".join(TIPOS_VALIDOS), tipo)
        )

    gpio = bruto.get("gpio_pico")
    if not isinstance(gpio, int) or isinstance(gpio, bool) or gpio < 0:
        raise ConfigError(
            "canal %r: 'gpio_pico' debe ser un entero >= 0 (recibido %r)"
            % (etiqueta, gpio)
        )

    polaridad = bruto.get("polaridad", "activo_alto")
    if polaridad not in POLARIDADES_VALIDAS:
        raise ConfigError(
            "canal %r: 'polaridad' debe ser uno de %s (recibido %r)"
            % (etiqueta, " / ".join(POLARIDADES_VALIDAS), polaridad)
        )

    antirrebote = bruto.get("antirrebote_ms", ANTIRREBOTE_POR_DEFECTO_MS)
    if not isinstance(antirrebote, int) or isinstance(antirrebote, bool) or antirrebote < 0:
        raise ConfigError(
            "canal %r: 'antirrebote_ms' debe ser un entero >= 0 (recibido %r)"
            % (etiqueta, antirrebote)
        )

    descripcion = bruto.get("descripcion", "") or ""
    if not isinstance(descripcion, str):
        raise ConfigError("canal %r: 'descripcion' debe ser texto" % etiqueta)

    return Canal(
        etiqueta=etiqueta,
        tipo=tipo,
        gpio_pico=gpio,
        polaridad=polaridad,
        antirrebote_ms=antirrebote,
        descripcion=descripcion,
    )


def cargar_desde_texto(texto: str, origen: str | None = None) -> BenchConfig:
    """Parsea y valida un YAML de banco ya leído en memoria.

    Aplica también los overrides de puerto por entorno (``PICO_PORT`` /
    ``DUT_PORT`` / ``HIL_BAUDRATE``). Lanza ``ConfigError`` si el YAML es
    inválido.
    """
    if yaml is None:
        raise ConfigError("pyyaml no está instalado (añádelo con requirements.txt)")

    try:
        datos = yaml.safe_load(texto)
    except yaml.YAMLError as exc:
        raise ConfigError("YAML malformado: %s" % exc) from exc

    if datos is None:
        datos = {}
    if not isinstance(datos, dict):
        raise ConfigError("el YAML raíz debe ser un mapa (clave: valor)")

    canales_brutos = datos.get("canales") or []
    if not isinstance(canales_brutos, list):
        raise ConfigError("'canales' debe ser una lista")

    canales: list[Canal] = []
    vistas: set[str] = set()
    gpios_vistos: dict[int, str] = {}
    for i, bruto in enumerate(canales_brutos, start=1):
        canal = _valida_canal(bruto, i)
        if canal.etiqueta in vistas:
            raise ConfigError("etiqueta de canal duplicada: %r" % canal.etiqueta)
        if canal.gpio_pico in gpios_vistos:
            raise ConfigError(
                "GPIO %d repetido en los canales %r y %r"
                % (canal.gpio_pico, gpios_vistos[canal.gpio_pico], canal.etiqueta)
            )
        vistas.add(canal.etiqueta)
        gpios_vistos[canal.gpio_pico] = canal.etiqueta
        canales.append(canal)

    baudrate = datos.get("baudrate", 115200)
    env_baud = os.environ.get("HIL_BAUDRATE")
    if env_baud:
        try:
            baudrate = int(env_baud)
        except ValueError as exc:
            raise ConfigError("HIL_BAUDRATE no es un entero: %r" % env_baud) from exc
    if not isinstance(baudrate, int) or isinstance(baudrate, bool) or baudrate <= 0:
        raise ConfigError("'baudrate' debe ser un entero positivo (recibido %r)" % baudrate)

    # Puertos: env tiene prioridad sobre el YAML.
    pico_port = _normaliza_puerto(os.environ.get("PICO_PORT")) or _normaliza_puerto(
        datos.get("pico_port")
    )
    dut_port = _normaliza_puerto(os.environ.get("DUT_PORT")) or _normaliza_puerto(
        datos.get("dut_port")
    )

    nombre = datos.get("nombre") or "sin-nombre"
    if not isinstance(nombre, str):
        raise ConfigError("'nombre' debe ser texto")

    return BenchConfig(
        nombre=nombre,
        pico_port=pico_port,
        dut_port=dut_port,
        baudrate=baudrate,
        canales=canales,
        origen=origen,
        nota=None,
    )


def cargar_config(ruta: str | None = None) -> BenchConfig:
    """Carga la configuración del banco.

    Si ``ruta`` es ``None`` se usa la variable de entorno ``HIL_BENCH_CONFIG``.
    Si tampoco está definida, o el archivo no existe, se devuelve una
    ``BenchConfig`` en MODO DEGRADADO (sin canales) con ``nota`` explicando el
    motivo, en vez de lanzar: así el server arranca y las herramientas informan
    con claridad de que no hay banco cargado.

    Un YAML presente pero inválido SÍ se reporta como modo degradado con la
    nota del error (no se rompe el arranque del server), para que el LLM pueda
    verlo vía ``estado_banco``/``listar_canales`` y corregir el archivo.
    """
    if ruta is None:
        ruta = os.environ.get("HIL_BENCH_CONFIG") or None

    if not ruta:
        return BenchConfig(
            nota=(
                "no hay banco cargado: define la variable de entorno "
                "HIL_BENCH_CONFIG con la ruta a un bench.yaml"
            )
        )

    if not os.path.isfile(ruta):
        return BenchConfig(
            nota="no se encontró el archivo de configuración: %r" % ruta
        )

    try:
        with open(ruta, "r", encoding="utf-8") as fh:
            texto = fh.read()
        return cargar_desde_texto(texto, origen=ruta)
    except ConfigError as exc:
        return BenchConfig(
            nota="configuración inválida en %r: %s" % (ruta, exc)
        )
    except OSError as exc:
        return BenchConfig(
            nota="no se pudo leer %r: %s" % (ruta, exc)
        )
