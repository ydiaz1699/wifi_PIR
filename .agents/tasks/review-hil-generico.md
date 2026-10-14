# Generalización del banco HIL a herramienta reutilizable

La refactorización convierte `hil_bench/` de un banco atado a wifi_PIR en una herramienta genérica: el core razona en primitivas de actuador (`boton`/`interruptor`) sobre un DUT descrito en un `bench.yaml` declarativo, la Pico pasa a ser una ejecutora "tonta" de GPIO por número, y wifi_PIR queda como un ejemplo en `examples/wifi_pir/`. Se renombró `EmisorSerial`→`DutSerial`, `EMISOR_PORT`→`DUT_PORT`, las herramientas de dominio (`pulsar_timbre`/`disparar_pir`/`mantener_pir`) se sustituyen por genéricas (`pulsar_boton`/`fijar_interruptor`/...), y se añade `config.py` con carga y validación de YAML. El objetivo central de reusabilidad se cumple: el único acoplamiento a 'timbre'/'pir'/'emisor' que queda está en docstrings de ejemplo y en `examples/wifi_pir`, nunca en la lógica del core.

Watch for: ninguna de las comprobaciones críticas revela un fallo bloqueante. El estado seguro al arranque, el mapeo polaridad→nivel, la tolerancia a ausencia de hardware/YAML y la validación de canales son correctos y están bien documentados (todo **confirmed** por lectura del código). Dos observaciones no bloqueantes de comportamiento (ventana de correlación malgastada en fallo de Pico; `_safe_inicial_hecho` se marca "hecho" aunque el banco tenga 0 canales) se detallan abajo. No se hizo commit ni push: todo el árbol `hil_bench/` está untracked (**confirmed** vía `git status`).

**Verdict**: APPROVED

## High-level view

El vocabulario de dominio desapareció del core. `config.py`, `server.py`, `pico_link.py`, `dut_serial.py` y el firmware razonan en canales/GPIO/tipos genéricos; 'timbre', 'pir' y 'emisor' sólo sobreviven como texto ilustrativo en docstrings y en el YAML de ejemplo. La separación es limpia: la Pico opera por número de GPIO y el mapa etiqueta→GPIO vive sólo en el YAML que lee el server.

El estado seguro al arranque es correcto para pines boot-strap. El firmware crea cada Pin en BAJO (`value=0`) la primera vez que lo toca y arranca llamando a `safe()`; el server manda `SAFE <gpios>` de sus canales en la primera comunicación con la Pico. La convención "GPIO ALTO = opto activo = entrada estimulada" vale para ambas polaridades porque la inversión, si hace falta, está en el cableado del canal, no en el GPIO. Esto está documentado de forma coherente en el firmware, en `config.py`, en el diseño y en el README, y garantiza que un canal `activo_bajo` sobre GPIO0/D3 nunca quede activo durante el reset del DUT.

El mapeo polaridad→nivel no invierte nada por error. `fijar_interruptor` traduce `alto`/`activo`→GPIO 1 y `bajo`/`reposo`→GPIO 0 directamente, y la `polaridad` del canal es puramente descriptiva (semántica del DUT y guía de cableado). No hay ninguna rama que use `polaridad` para decidir el nivel del GPIO, que es exactamente lo que evita la inversión accidental.

La tolerancia a ausencia de hardware y de configuración es total. Los puertos se abren de forma perezosa; sin puerto, sin pyserial o con fallo de apertura, cada capa traduce el problema a un error con mensaje claro (dict `{ok: False, ...}` en las herramientas, `PicoNoDisponible`/`PicoTimeout` en el enlace) en vez de propagar una traza. Un YAML ausente, no encontrado o inválido no rompe el arranque: el server entra en modo degradado con una `nota` explicativa visible por `estado_banco`/`listar_canales`.

Las herramientas genéricas validan el canal antes de accionar. `_requiere_canal` comprueba, en orden, que haya banco cargado, que el canal exista (listando los disponibles) y que su tipo coincida con el esperado, devolviendo un error distinto y legible en cada caso.

Las dos observaciones menores previas están corregidas: el `EOFError` del bucle del firmware ahora duerme 50 ms antes de reintentar (sin busy-loop) y `_enviar_pico` captura sólo `PicoError`, no una excepción genérica.

<details>
<summary>Issues (2)</summary>

1. **Ventana de correlación malgastada sin Pico** (possible, no bloqueante) — en `_estimular_y_leer`, si el comando a la Pico falla por ausencia de hardware, igualmente se espera la ventana completa (`antirrebote_ms + 400`) antes de devolver. Opcional: cortocircuitar la espera cuando `resultado_pico["ok"]` es `False` para que el error llegue antes.
2. **`_safe_inicial_hecho` se marca hecho con 0 canales** (confirmed, no bloqueante) — cuando el banco está cargado pero sin GPIO, `_asegurar_safe_inicial` marca la bandera sin haber hablado con la Pico. Es benigno (no hay pines que asegurar), pero conviene un comentario que lo deje explícito para no confundir en el futuro.

</details>

<details>
<summary>Details</summary>

## Vocabulario de dominio fuera del core

El objetivo de reusabilidad se cumple de forma verificable. Un `grep` de `emisor|EMISOR_PORT|leer_serial_emisor|EmisorSerial|pulsar_timbre|disparar_pir|mantener_pir|timbre` sobre `hil_bench/**/*.py` sólo devuelve coincidencias dentro de docstrings y comentarios que usan 'timbre'/'pir' como ejemplos ("son solo ETIQUETAS que el usuario asigna"), más el YAML de ejemplo en `examples/wifi_pir/bench.yaml` (**confirmed**). La lógica —`config.Canal`, el resolutor etiqueta→GPIO del server, el protocolo del firmware— no contiene ningún concepto de dominio. `EmisorSerial`→`DutSerial` y `EMISOR_PORT`→`DUT_PORT` quedaron aplicados en todas las referencias; `emisor_serial.py` fue eliminado y no quedan imports colgando (el server importa `from .dut_serial import DutSerial`).

El tipo `analogico` está diseñado pero no implementado, y `config.py` lo rechaza explícitamente con un `ConfigError` claro ("el tipo 'analogico' está diseñado pero aún no implementado") en vez de aceptarlo y fallar más tarde. Es la decisión correcta: no da falsa sensación de soporte.

## Estado seguro al arranque y pines boot-strap

```
power-on Pico ──► main(): manos.safe()      (sin pines registrados: no-op)
                      │
                      ▼
primer SAFE <g..> del server (al 1er PING OK) ──► Pin(g, OUT, value=0) por cada GPIO
                      │
                      ▼
             todos los canales en BAJO = opto apagado = reposo
```

El firmware crea cada `Pin` con `value=0` la primera vez que lo toca (`_pin`), de modo que ningún GPIO se conduce ALTO antes de una orden explícita. Entre el encendido de la Pico y el primer `SAFE` del server, los pines ni siquiera están configurados como salida: quedan como entradas de alta impedancia, lo que para un canal `activo_bajo` sobre GPIO0/D3 (opto apagado → pin en alto por el pull-up del DUT) es precisamente el estado que no fuerza el modo flash del ESP8266. El server envía `SAFE <gpios de sus canales>` en la primera comunicación con éxito (`_asegurar_safe_inicial`), dejando en reposo exactamente los pines declarados. `MAX_PULSE_MS` (10 s) acota cualquier pulso como salvaguarda anti-bloqueo. Esta cadena está documentada de forma consistente en el firmware, `config.BenchConfig`, el diseño (§3, §4, §11) y el README (**confirmed**).

## Mapeo polaridad → nivel de GPIO

El punto que la tarea pedía vigilar —no invertir sin querer un canal `activo_bajo`— está resuelto por diseño: la `polaridad` nunca entra en la decisión del nivel del GPIO. `fijar_interruptor` normaliza el estado pedido y asigna `nivel = 1` para `alto`/`activo`/`on`/`high`/`1` y `nivel = 0` para `bajo`/`reposo`/`off`/`low`/`0`, y manda `SET <gpio> <nivel>` tal cual. La convención física "GPIO ALTO = opto activo = entrada estimulada" se mantiene idéntica para ambas polaridades porque la inversión, cuando el canal la necesita (activo en LOW), está en el cableado del PC817 y los pull-up/pull-down del DUT, no en el software (**confirmed**, documentado en `config.BenchConfig`, firmware y diseño §3/§5). Un valor de `estado` no reconocido devuelve un error claro en vez de un nivel por defecto silencioso.

## Tolerancia a ausencia de hardware y de configuración

El manejo de fallos es uniforme y fail-safe en las tres capas. En `pico_link`, el constructor no abre el puerto; `abrir()` lanza `PicoNoDisponible` si falta pyserial, no hay puerto o el SO no puede abrirlo, y `enviar()` convierte un fallo de E/S en `PicoNoDisponible` (cerrando el puerto para reintentar) y la ausencia de respuesta en `PicoTimeout`. En `dut_serial`, `iniciar()` devuelve `False` y deja el motivo en `ultimo_error` sin lanzar; el hilo de lectura tolera la desconexión marcando el puerto caído y saliendo sin tumbar el proceso. En `server`, `_enviar_pico` traduce cualquier `PicoError` a `{ok: False, error: ...}`, de modo que las herramientas siempre devuelven un dict legible (**confirmed**).

La configuración es igual de robusta. `cargar_config` devuelve `BenchConfig` en modo degradado (0 canales, `nota` explicativa) cuando no hay `HIL_BENCH_CONFIG`, cuando el archivo no existe, cuando el YAML es inválido (`ConfigError`) o cuando no se puede leer (`OSError`); en ningún caso propaga una excepción al arranque del server. La validación de `config.py` es estricta y con mensajes concretos: etiquetas y GPIO únicos, tipo en {boton, interruptor}, polaridad en {activo_alto, activo_bajo}, `gpio_pico`/`antirrebote_ms`/`baudrate` enteros en rango (con el detalle fino de excluir `bool`, que en Python es subclase de `int`). El spot-check de `py_compile` sobre `pico/main.py`, `config.py` y `server.py` pasó limpio (exit 0), consistente con lo que el coder reportó.

## Validación de canal en las herramientas genéricas

`_requiere_canal(etiqueta, tipo_esperado)` concentra la validación y produce tres errores distinguibles: sin banco cargado ("no hay banco cargado ... Define HIL_BENCH_CONFIG"), canal inexistente (que además enumera los canales disponibles) y tipo incorrecto ("canal 'x' es de tipo 'boton', se esperaba 'interruptor'"). `pulsar_boton` exige tipo `boton` y `fijar_interruptor` exige `interruptor`, así que accionar un canal con la herramienta equivocada falla con un mensaje claro antes de tocar la Pico (**confirmed**). La ventana de correlación se deriva de `antirrebote_ms + MARGEN_VENTANA_MS` (400 ms) con un suelo por defecto, no de constantes de dominio.

## Correcciones de las observaciones previas

Ambas observaciones menores de la revisión anterior están aplicadas (**confirmed**). El bucle principal del firmware ahora hace `time.sleep_ms(50)` antes del `continue` tras un `EOFError`, evitando el busy-loop que quemaba CPU cuando el host no está conectado (`time` está disponible en esa rama porque sólo se importa y sólo se ejecuta `main()` cuando `_HW` es `True` en la Pico). Y `_enviar_pico` captura exclusivamente `PicoError` —la base de `PicoNoDisponible`/`PicoTimeout`— en lugar de una excepción ancha, de modo que un bug de programación en la ruta de envío ya no quedaría enmascarado como "Pico no disponible".

## Observaciones de comportamiento no bloqueantes

Dos detalles menores de comportamiento, ninguno bloqueante. En `_estimular_y_leer` la espera de la ventana (`anyio.sleep(ventana_ms/1000)`) ocurre siempre, incluso cuando el comando a la Pico ya falló por ausencia de hardware; el resultado es correcto (`pico.ok = False` y `dut_nota` explicando la ausencia), pero el LLM espera cientos de ms de más antes de recibir el error (possible, cosmético). Y `_asegurar_safe_inicial` marca `_safe_inicial_hecho = True` cuando el banco está cargado pero no tiene GPIO, sin haber contactado la Pico; es inocuo (no hay pines que asegurar) pero la bandera queda en un estado que sugiere que el SAFE se envió cuando no fue así (confirmed, sin impacto funcional).

## Cobertura de prueba

La verificación software reportada por el coder cubre lo esencial sin hardware: compilación de todos los `.py` (incluida la guarda `_HW` del firmware), import del server en modo degradado con registro de las 6 herramientas, carga y asserts del YAML de ejemplo, parseo del protocolo del firmware en CPython, y prioridad de `PICO_PORT`/`DUT_PORT` sobre el YAML. Repetí un spot-check estrecho (`py_compile` de los tres archivos más delicados) y pasó.

No probado (inevitable sin hardware, y así lo documentan tanto el README §10 como la verificación): el lazo cerrado real estímulo→serial, el flasheo de la Pico, y que el `SAFE` inicial deja GPIO0/D3 en reposo durante el reset del ESP8266. No hay suite de tests automatizados (ni pytest ni un `tests/`); para una herramienta que ya tiene un parser puro (`procesar_comando`) y un validador puro (`_valida_canal`/`cargar_desde_texto`), unos tests unitarios de esas dos piezas serían baratos y protegerían el protocolo y el esquema YAML de regresiones. Es una recomendación, no un bloqueante para esta entrega.

## Estado de git

Todo `hil_bench/` figura como untracked en `git status` sobre la rama `fix/emisor-pir-criticos`; no hay commit ni push de estos cambios (**confirmed**). Coincide con lo pedido: los cambios quedan en el working tree.

</details>

<details>
<summary>Mapa de archivos</summary>

- `hil_bench/__init__.py` — docstring del paquete, reescrito en clave genérica.
- `hil_bench/README.md` — guía operativa genérica; wifi_PIR como ejemplo.
- `hil_bench/docs/DISENO_BANCO_HIL.md` — diseño completo (vocabulario, polaridad, protocolo por GPIO, YAML).
- `hil_bench/pico/main.py` — firmware MicroPython v2.0 genérico por GPIO (PING/PULSE/SET/SAFE); arranque seguro; `MAX_PULSE_MS`; fix del busy-loop EOFError.
- `hil_bench/mcp_server/__init__.py` — docstring del server, genérico.
- `hil_bench/mcp_server/config.py` — NUEVO: `Canal`/`BenchConfig`, carga/validación de `bench.yaml`, modo degradado, merge con env.
- `hil_bench/mcp_server/server.py` — herramientas MCP genéricas; resolutor canal→GPIO; `_requiere_canal`; captura estrecha de `PicoError`.
- `hil_bench/mcp_server/pico_link.py` — enlace serial a la Pico; apertura perezosa; `pulse_gpio`/`set_gpio`/`safe`.
- `hil_bench/mcp_server/dut_serial.py` — RENOMBRADO de `emisor_serial.py`; clase `DutSerial`; lectura en hilo tolerante a desconexión.
- `hil_bench/mcp_server/requirements.txt` — `mcp[cli]<2`, `pyserial`, `pyyaml` (nuevo).
- `hil_bench/examples/wifi_pir/bench.yaml` — NUEVO: ejemplo wifi_PIR (canales timbre/pir).

`emisor_serial.py` eliminado. Todo el directorio está untracked (sin commit).

</details>
