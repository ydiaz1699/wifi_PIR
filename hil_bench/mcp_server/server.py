"""MCP server del banco HIL ("manos y oídos" del LLM) sobre FastMCP (stdio).

Banco de pruebas HIL GENÉRICO: le da al LLM "manos" para accionar las entradas
físicas de cualquier dispositivo bajo prueba (DUT) y "oídos" para leer su
monitor serial. El core razona en primitivas de actuador genéricas: un canal es
de tipo ``boton`` (momentáneo) o ``interruptor`` (sostenido). Las etiquetas de
dominio ('timbre', 'pir', 'reset'...) son solo nombres que el usuario asigna a
cada canal en su ``bench.yaml``; wifi_PIR es solo un ejemplo
(``examples/wifi_pir/bench.yaml``).

Las herramientas de estímulo cierran el lazo acción -> reacción: accionan a
través de la Pico (por número de GPIO, resuelto desde el YAML), esperan una
ventana corta y devuelven lo que el DUT imprimió, para que el LLM confirme la
reacción. La ventana de correlación se deriva del ``antirrebote_ms`` del canal
(con margen), no de valores hardcodeados.

Configuración
-------------
    HIL_BENCH_CONFIG   ruta al ``bench.yaml`` que describe el banco. Si falta o
                       no existe, el server arranca en modo degradado (sin
                       canales) e informa por ``estado_banco``/``listar_canales``.
    PICO_PORT          puerto serie de la Pico (prioridad sobre el YAML).
    DUT_PORT           puerto serie del DUT (prioridad sobre el YAML).
    HIL_BAUDRATE       baudrate común (prioridad sobre el YAML; por defecto 115200).

Diseño sin hardware: ni la instanciación ni el arranque del server abren
puertos reales. Los puertos se abren de forma perezosa al usar una herramienta,
y si el hardware no está presente la herramienta devuelve un mensaje claro en
vez de propagar una excepción. Así el server se puede cargar y registrar en
Kiro aunque no haya nada conectado.

Arranque: ``python -m hil_bench.mcp_server.server`` (transporte stdio).

Nota de concurrencia: las herramientas son asíncronas y NO bloquean el event
loop con esperas largas. La ventana de correlación se espera con
``anyio.sleep``; la E/S serial puntual (write + readline con timeout corto) se
delega a un hilo con ``anyio.to_thread.run_sync`` para no bloquear el loop.
"""

from __future__ import annotations

import anyio
from mcp.server.fastmcp import FastMCP

from .config import Canal, cargar_config
from .dut_serial import DutSerial
from .pico_link import PicoError, PicoLink

# --- Configuración declarativa (bench.yaml) ----------------------------------
# Se carga una vez al importar el módulo. En modo degradado (sin YAML) CONFIG
# trae 0 canales y una ``nota`` explicando el motivo; el server igualmente
# arranca y registra las herramientas.
CONFIG = cargar_config()

# Margen que se añade al antirrebote del canal para fijar la ventana de
# correlación (esperar lo suficiente para captar la reacción del DUT).
MARGEN_VENTANA_MS = 400
# Ventana por defecto si un canal no declara antirrebote útil.
VENTANA_POR_DEFECTO_MS = 600

# --- Enlaces de hardware (perezosos, no abren puerto al construir) -----------
pico = PicoLink(CONFIG.pico_port, baudrate=CONFIG.baudrate)
dut = DutSerial(CONFIG.dut_port, baudrate=CONFIG.baudrate)

mcp = FastMCP("hil-bench")

# Marca para mandar el SAFE inicial de los GPIO de los canales una sola vez,
# la primera vez que se logre hablar con la Pico.
_safe_inicial_hecho = False


# --- Utilidades internas -----------------------------------------------------

def _ventana_canal(canal: Canal) -> int:
    """Ventana de correlación (ms) derivada del antirrebote del canal + margen."""
    base = canal.antirrebote_ms if canal.antirrebote_ms > 0 else VENTANA_POR_DEFECTO_MS
    return base + MARGEN_VENTANA_MS


async def _enviar_pico(comando: str) -> dict:
    """Envía un comando a la Pico en un hilo y devuelve un dict de resultado.

    Nunca lanza: traduce los fallos a ``{"ok": False, "error": ...}`` para que
    las herramientas devuelvan un mensaje claro sin hardware.
    """
    try:
        respuesta = await anyio.to_thread.run_sync(pico.enviar, comando)
        ok = respuesta.startswith("OK") or respuesta.startswith("PONG")
        return {"ok": ok, "respuesta": respuesta}
    except PicoError as exc:
        return {"ok": False, "error": str(exc)}


async def _asegurar_safe_inicial() -> None:
    """Manda ``SAFE <gpios>`` de los canales la primera vez que la Pico responde.

    Deja en reposo exactamente los GPIO declarados en el YAML, respetando la
    precaución de pines boot-strap. Se intenta de forma perezosa: si la Pico no
    está, no pasa nada (se reintentará en la siguiente acción).
    """
    global _safe_inicial_hecho
    if _safe_inicial_hecho or not CONFIG.cargado:
        return
    gpios = CONFIG.gpios()
    if not gpios:
        _safe_inicial_hecho = True
        return
    comando = "SAFE " + " ".join(str(g) for g in gpios)
    res = await _enviar_pico(comando)
    if res["ok"]:
        _safe_inicial_hecho = True


def _asegurar_dut() -> bool:
    """Intenta tener el lector del DUT activo. Devuelve si quedó leyendo."""
    if dut.activo:
        return True
    return dut.iniciar()


async def _estimular_y_leer(comando: str, ventana_ms: int, canal: Canal) -> dict:
    """Patrón acción -> reacción.

    1) marca el instante, 2) manda el comando a la Pico, 3) espera la ventana
    sin bloquear el loop, 4) recoge las líneas del DUT aparecidas desde la
    marca. Devuelve un dict con el resultado de la Pico y los logs del DUT.
    """
    await _asegurar_safe_inicial()
    dut_ok = _asegurar_dut()
    marca = dut.marca_tiempo()
    resultado_pico = await _enviar_pico(comando)
    await anyio.sleep(ventana_ms / 1000.0)
    if dut_ok and dut.activo:
        logs = dut.lineas_desde(marca)
        nota_dut = None
    else:
        logs = []
        nota_dut = dut.ultimo_error or "DUT no disponible"
    return {
        "canal": canal.etiqueta,
        "accion": comando,
        "pico": resultado_pico,
        "dut_logs": logs,
        "dut_nota": nota_dut,
    }


def _error(mensaje: str) -> dict:
    """Dict de error uniforme para las herramientas."""
    return {"ok": False, "error": mensaje}


def _requiere_canal(etiqueta: str, tipo_esperado: str) -> "tuple[Canal | None, dict | None]":
    """Resuelve y valida un canal por etiqueta y tipo.

    Devuelve ``(canal, None)`` si todo bien, o ``(None, dict_error)`` con un
    mensaje claro si no hay banco, el canal no existe o no es del tipo pedido.
    """
    if not CONFIG.cargado:
        return None, _error(
            "no hay banco cargado (%s). Define HIL_BENCH_CONFIG."
            % (CONFIG.nota or "sin configuración")
        )
    canal = CONFIG.canal(etiqueta)
    if canal is None:
        return None, _error(
            "canal %r no existe. Canales disponibles: %s"
            % (etiqueta, ", ".join(CONFIG.etiquetas()) or "(ninguno)")
        )
    if canal.tipo != tipo_esperado:
        return None, _error(
            "canal %r es de tipo %r, se esperaba %r"
            % (etiqueta, canal.tipo, tipo_esperado)
        )
    return canal, None


# --- Herramientas MCP (lo que el LLM llama) ----------------------------------

@mcp.tool()
async def listar_canales() -> dict:
    """Lista los canales definidos en el banco (bench.yaml).

    Primer paso para que el LLM sepa qué puede accionar: devuelve, por cada
    canal, su etiqueta, tipo (boton/interruptor), gpio de la Pico, polaridad,
    antirrebote_ms (informativo) y descripción. Si no hay banco cargado, lo
    indica en ``nota`` y devuelve una lista vacía.
    """
    return {
        "banco": CONFIG.nombre,
        "cargado": CONFIG.cargado,
        "origen": CONFIG.origen,
        "nota": CONFIG.nota,
        "canales": [c.como_dict() for c in CONFIG.canales],
    }


@mcp.tool()
async def estado_banco() -> dict:
    """Informa del estado del banco HIL: ¿Pico viva? ¿DUT emitiendo? puertos.

    Hace PING a la Pico y comprueba si el lector del DUT está capturando serial.
    No acciona nada. Útil como primer paso antes de estimular. Incluye el nombre
    del banco y el número de canales definidos.
    """
    await _asegurar_safe_inicial()
    pico_res = await _enviar_pico("PING")
    dut_ok = _asegurar_dut()
    # Pequeña espera para ver si el DUT imprime algo (está "emitiendo").
    await anyio.sleep(0.3)
    hay_serial = bool(dut.lineas_recientes(2000)) if dut_ok else False
    return {
        "banco": CONFIG.nombre,
        "cargado": CONFIG.cargado,
        "nota": CONFIG.nota,
        "num_canales": len(CONFIG.canales),
        "canales": CONFIG.etiquetas(),
        "pico": {
            "puerto": CONFIG.pico_port,
            "viva": pico_res["ok"],
            "respuesta": pico_res.get("respuesta"),
            "error": pico_res.get("error"),
        },
        "dut": {
            "puerto": CONFIG.dut_port,
            "leyendo": bool(dut_ok and dut.activo),
            "emitiendo_serial": hay_serial,
            "error": dut.ultimo_error,
        },
        "baudrate": CONFIG.baudrate,
    }


@mcp.tool()
async def pulsar_boton(canal: str, ms: int = 50) -> dict:
    """Pulsa un canal de tipo ``boton`` ``ms`` milisegundos y lee la reacción del DUT.

    Valida que ``canal`` exista y sea de tipo ``boton``. Acciona un pulso en el
    GPIO de la Pico correspondiente (ALTO durante ``ms``, luego BAJO) y devuelve
    las líneas que el DUT imprimió, para confirmar si detectó el flanco y
    respetó su antirrebote. Pulsos cortos (p.ej. 50 ms) suelen bastar: el DUT
    lee el flanco, no la duración. Devuelve un error claro si el canal no existe
    o no es un botón.
    """
    c, err = _requiere_canal(canal, "boton")
    if err:
        return err
    comando = "PULSE %d %d" % (c.gpio_pico, int(ms))
    return await _estimular_y_leer(comando, _ventana_canal(c), c)


@mcp.tool()
async def fijar_interruptor(canal: str, estado: str) -> dict:
    """Fija un canal de tipo ``interruptor`` en un estado sostenido y lee la reacción.

    ``estado`` acepta: ``'alto'``/``'activo'`` (estimulado) o
    ``'bajo'``/``'reposo'`` (en reposo). Valida que ``canal`` exista y sea de
    tipo ``interruptor``, sostiene el nivel en el GPIO de la Pico y devuelve la
    reacción del DUT. Recuerda dejarlo en reposo al terminar (o usa
    ``reset_banco``). Error claro si el canal no existe o no es interruptor.
    """
    c, err = _requiere_canal(canal, "interruptor")
    if err:
        return err
    valor = (estado or "").strip().lower()
    if valor in ("alto", "activo", "1", "on", "high"):
        nivel = 1
    elif valor in ("bajo", "reposo", "0", "off", "low"):
        nivel = 0
    else:
        return _error(
            "estado %r no válido. Usa 'alto'/'activo' o 'bajo'/'reposo'" % estado
        )
    comando = "SET %d %d" % (c.gpio_pico, nivel)
    return await _estimular_y_leer(comando, _ventana_canal(c), c)


@mcp.tool()
async def leer_serial_dut(ventana_ms: int = 2000) -> dict:
    """Devuelve las últimas líneas del monitor serial del DUT.

    No acciona nada: solo escucha. Útil para ver qué está imprimiendo el DUT por
    su cuenta (heartbeats, arranque, etc.) o para releer tras un estímulo.
    """
    dut_ok = _asegurar_dut()
    if not dut_ok:
        return {"ok": False, "lineas": [], "error": dut.ultimo_error}
    return {"ok": True, "lineas": dut.lineas_recientes(int(ventana_ms)), "error": None}


@mcp.tool()
async def reset_banco() -> dict:
    """Lleva la Pico a estado seguro (SAFE): todos los canales a reposo.

    Imprescindible tras cualquier interruptor sostenido y recomendable al
    terminar una sesión, para no dejar ningún canal estimulado durante un
    posible reset del DUT (crítico para pines boot-strap como GPIO0/D3 en el
    ejemplo wifi_PIR). Pone en reposo los GPIO de los canales del banco.
    """
    gpios = CONFIG.gpios()
    if gpios:
        comando = "SAFE " + " ".join(str(g) for g in gpios)
    else:
        comando = "SAFE"
    resultado = await _enviar_pico(comando)
    return {"accion": comando, "pico": resultado}


def main():
    """Arranca el MCP server por stdio."""
    mcp.run(transport="stdio")


if __name__ == "__main__":
    main()
