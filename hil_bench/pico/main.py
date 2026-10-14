"""Firmware MicroPython para la Raspberry Pi Pico: las "manos" del banco HIL.

La Pico es una ejecutora GENÉRICA de GPIO: recibe comandos de texto (una línea
por comando) por su USB serial (stdin) a 115200 baud y acciona cualquier GPIO
que le indiquen POR NÚMERO. No conoce etiquetas de dominio (timbre, pir...) ni
polaridades: toda esa semántica vive en el MCP server (que la lee de
``bench.yaml``). La Pico queda "tonta" y 100% reutilizable entre proyectos; el
YAML del server es la única fuente de verdad del mapa etiqueta -> GPIO.

Cada GPIO acciona el LED de un optoacoplador PC817 que "cierra el botón" o
inyecta nivel en una entrada del dispositivo bajo prueba (DUT), con las masas
aisladas (ver ``docs/DISENO_BANCO_HIL.md``).

Convención física (vale para todo canal, cualquier polaridad)
--------------------------------------------------------------
GPIO de la Pico en ALTO = LED del opto encendido = opto activo = entrada del
DUT ESTIMULADA. GPIO en BAJO = opto apagado = canal en REPOSO. El cableado de
cada canal (pull-up/pull-down en el pin del DUT) se diseña para que "opto
activo" signifique "entrada estimulada" en ese canal. El ESTADO SEGURO es
TODOS los GPIO usados en BAJO (optos apagados).

PRECAUCIÓN CRÍTICA — pines boot-strap del DUT (p.ej. GPIO0/D3 del ESP8266)
--------------------------------------------------------------------------
Algunos pines del DUT son de arranque (boot strap). En el ejemplo wifi_PIR, D3
= GPIO0 del ESP8266: si está en LOW durante el reset/encendido, el ESP entra en
modo flash y NO ejecuta su firmware. Como el canal de ese pin es activo_bajo y
su opto en reposo (apagado) deja el pin en alto por el pull-up, el estado
seguro NO lo activa. Por eso este firmware arranca SIEMPRE en estado seguro
(todos los GPIO que toca en BAJO) ANTES de nada, y solo estimula en ventanas
cortas cuando el DUT ya está corriendo. ``MAX_PULSE_MS`` acota los pulsos como
salvaguarda anti-bloqueo (nunca dejar un pin estimulado indefinidamente).

Protocolo serial (PC -> Pico), genérico por número de GPIO
----------------------------------------------------------
    PING                 -> responde 'PONG v<VERSION>'
    PULSE <gpio> <ms>    -> pone <gpio> ALTO durante <ms> (acotado) y lo baja
    SET <gpio> <0|1>     -> fija <gpio> en BAJO (0) o ALTO (1), sostenido
    SAFE                 -> todos los GPIO tocados hasta ahora a BAJO (reposo)
    SAFE <g1> <g2> ...   -> además registra esos GPIO y los pone a BAJO

Respuestas: 'OK <eco>' en éxito o 'ERR <motivo>' en error.

El server manda ``SAFE <g1> <g2> ...`` con los GPIO de sus canales al conectar,
de modo que la Pico solo gestiona pines explícitamente declarados y arranca ese
conjunto en reposo. Un ``SAFE`` sin argumentos reafirma el reposo de todos los
GPIO ya conocidos.

Trade-off de los pulsos
-----------------------
Los pulsos usan ``time.sleep_ms`` SOLO durante su duración: el firmware queda
ocupado mientras dura el pulso y no procesa otro comando hasta soltar. Es un
diseño sencillo y determinista; los pulsos del banco son cortos (decenas a
cientos de ms) y las pruebas son secuenciales, así que el bloqueo breve es
aceptable.

Para flashear: copiar este archivo como ``main.py`` en la raíz de una Pico con
MicroPython; arranca solo al encender.
"""

VERSION = "2.0"

# Salvaguarda anti-bloqueo: ningún pulso dura más que esto, aunque lo pidan.
# Protege a cualquier pin (en especial pines boot-strap del DUT) de quedar
# estimulado demasiado tiempo.
MAX_PULSE_MS = 10000

# GPIO máximo direccionable (la Pico expone GP0..GP28). Acota el parseo para no
# intentar crear Pines inexistentes.
GPIO_MAX = 28

# ---------------------------------------------------------------------------
# Importación de hardware aislada para que el archivo COMPILE en CPython
# (py_compile) aunque ``machine`` solo exista en MicroPython. En la Pico el
# import tiene éxito y _HW = True; en el PC el import falla, _HW = False y el
# bucle principal no se ejecuta (ver guarda al final). El código sigue siendo
# plenamente válido en la Pico: allí no se toma la rama de excepción.
# ---------------------------------------------------------------------------
try:
    from machine import Pin  # type: ignore
    import time  # time.sleep_ms existe en MicroPython
    _HW = True
except ImportError:
    _HW = False


class Manos:
    """Controla GPIO de la Pico por número, bajo demanda.

    Mantiene un diccionario ``gpio -> Pin`` con los pines ya usados. Cada Pin se
    crea en BAJO (opto apagado) la primera vez que se toca, garantizando el
    estado seguro antes de cualquier estímulo. ``safe()`` baja todos los pines
    conocidos.
    """

    def __init__(self):
        self._pines = {}  # gpio(int) -> Pin

    def _pin(self, gpio):
        """Devuelve el Pin de ``gpio``, creándolo en BAJO si es la primera vez."""
        pin = self._pines.get(gpio)
        if pin is None:
            # value=0 => arranca en BAJO (opto apagado) = reposo seguro.
            pin = Pin(gpio, Pin.OUT, value=0)
            self._pines[gpio] = pin
        return pin

    def registrar(self, gpio):
        """Registra un GPIO (lo crea en BAJO) sin accionarlo. Idempotente."""
        self._pin(gpio).value(0)

    def set(self, gpio, nivel):
        """Fija ``gpio`` en 0 (BAJO/reposo) o 1 (ALTO/estimulado), sostenido."""
        self._pin(gpio).value(1 if nivel else 0)

    def pulse(self, gpio, ms):
        """Pone ``gpio`` ALTO durante ``ms`` (acotado) y lo baja. Devuelve los ms."""
        ms = _clamp_ms(ms)
        pin = self._pin(gpio)
        pin.value(1)
        time.sleep_ms(ms)
        pin.value(0)
        return ms

    def safe(self, gpios=None):
        """Lleva a BAJO los GPIO indicados (registrándolos) o todos los conocidos."""
        if gpios:
            for g in gpios:
                self.registrar(g)
        else:
            for pin in self._pines.values():
                pin.value(0)


def _clamp_ms(ms):
    """Acota una duración de pulso al rango [0, MAX_PULSE_MS]."""
    if ms < 0:
        return 0
    if ms > MAX_PULSE_MS:
        return MAX_PULSE_MS
    return ms


def _parse_entero(token):
    """Convierte un token a entero no negativo. Lanza ValueError si no es válido."""
    value = int(token)
    if value < 0:
        raise ValueError("valor negativo")
    return value


def _parse_gpio(token):
    """Convierte un token a número de GPIO válido [0, GPIO_MAX]. Lanza ValueError."""
    gpio = _parse_entero(token)
    if gpio > GPIO_MAX:
        raise ValueError("gpio fuera de rango")
    return gpio


def procesar_comando(linea, manos):
    """Interpreta una línea de comando y devuelve la respuesta de texto.

    Función pura respecto al parseo: facilita razonar sobre el protocolo. Las
    acciones de hardware las delega en ``manos``. Devuelve 'OK <eco>' o
    'ERR <motivo>' sin el salto de línea final.
    """
    linea = linea.strip()
    if not linea:
        return "ERR vacio"

    partes = linea.split()
    cmd = partes[0].upper()

    try:
        if cmd == "PING":
            return "PONG v" + VERSION

        if cmd == "SAFE":
            # SAFE opcional con lista de GPIO a registrar y poner en reposo.
            gpios = [_parse_gpio(t) for t in partes[1:]]
            manos.safe(gpios if gpios else None)
            if gpios:
                return "OK SAFE " + " ".join(str(g) for g in gpios)
            return "OK SAFE"

        if cmd == "PULSE":
            if len(partes) < 3:
                return "ERR PULSE requiere <gpio> <ms>"
            gpio = _parse_gpio(partes[1])
            ms = manos.pulse(gpio, _parse_entero(partes[2]))
            return "OK PULSE %d %d" % (gpio, ms)

        if cmd == "SET":
            if len(partes) < 3:
                return "ERR SET requiere <gpio> <0|1>"
            gpio = _parse_gpio(partes[1])
            nivel_token = partes[2]
            if nivel_token not in ("0", "1"):
                return "ERR SET nivel debe ser 0 o 1"
            manos.set(gpio, nivel_token == "1")
            return "OK SET %d %s" % (gpio, nivel_token)

        return "ERR comando desconocido"
    except ValueError:
        return "ERR argumento invalido"


def main():
    """Bucle principal: lee líneas de stdin y responde por stdout.

    Arranca en estado seguro (no se toca ningún pin hasta que llega un comando;
    cada pin se crea en BAJO al primer uso). Solo se ejecuta en la Pico
    (_HW True).
    """
    manos = Manos()
    manos.safe()  # reafirma el estado seguro al arrancar (aún sin pines)
    while True:
        try:
            linea = input()
        except EOFError:
            # Sin datos todavía (host no conectado o stream cerrado). Pausa
            # corta para no entrar en busy-loop quemando CPU y reintenta.
            time.sleep_ms(50)
            continue
        respuesta = procesar_comando(linea, manos)
        print(respuesta)


# Solo arranca el bucle de hardware en la Pico. En CPython (_HW False) el
# módulo se puede importar/compilar sin tocar GPIO ni bloquear.
if _HW:
    main()
