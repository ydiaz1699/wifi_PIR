# Banco de pruebas HIL "manos y oídos" del LLM

El banco le da al LLM un par de manos (firmware MicroPython en una Raspberry Pi Pico que acciona dos optoacopladores PC817) y un par de oídos (lectura del monitor serial del emisor D1 mini), coordinados por un MCP server en Python sobre FastMCP/stdio. El objetivo es cerrar el lazo acción → reacción sin que el usuario apriete botones: el LLM pulsa el timbre o dispara el PIR a través de la Pico y lee en el serial del emisor si leyó el flanco, si respetó el antirrebote y si encoló el EVENT. La implementación sigue el documento de diseño autoritativo (`hil_bench/docs/DISENO_BANCO_HIL.md`) en arquitectura, pines, protocolo serial y estructura de carpetas. El diseño prioriza correctamente la seguridad del boot strap GPIO0/D3 del ESP8266: tanto el firmware como el server arrancan y permanecen en estado seguro por defecto.

**Watch for:** no hay hallazgos bloqueantes. Dos observaciones menores de robustez (confianza: possible/likely) que no impiden el funcionamiento: `_enviar_pico` en el server solo atrapa `PicoError` y no excepciones genéricas inesperadas; y el `continue` tras `EOFError` en el bucle de la Pico podría consumir CPU en bucle vivo si stdin quedara cerrado, aunque en el uso real por USB no ocurre.

**Verdict**: APPROVED

## High-level view

El estado seguro al arranque está bien resuelto en los dos extremos. En la Pico, los `Pin` se construyen con `value=0` (opto apagado) en `Manos.__init__` y `main()` reafirma con `manos.safe()` antes de entrar al bucle, de modo que GPIO0/D3 nunca queda en LOW durante un reset del emisor. En el server, nada abre puertos al instanciar; la apertura es perezosa.

La tolerancia a ausencia de hardware es consistente. El server se puede cargar y registrar sin Pico ni emisor conectados: `PicoLink` abre el puerto solo al enviar y traduce los fallos a excepciones propias con mensaje claro, y `EmisorSerial.iniciar()` devuelve `False` en vez de lanzar. Las herramientas devuelven dicts con campos de error en lugar de propagar trazas.

La superficie MCP coincide con el diseño: las seis herramientas (`estado_banco`, `pulsar_timbre`, `disparar_pir`, `mantener_pir`, `leer_serial_emisor`, `reset_banco`) tienen los nombres de la sección 6 y docstrings en español orientados al LLM que explican qué acciona cada una y qué antirrebote verificar.

El event loop no se bloquea: las esperas de ventana usan `anyio.sleep` y la E/S serial puntual se delega a un hilo con `anyio.to_thread.run_sync`. El único bloqueo real es dentro de la Pico durante el pulso (`time.sleep_ms`), acotado por `MAX_PULSE_MS = 10000` como salvaguarda anti-bloqueo de pin, lo cual es el comportamiento deseado por diseño.

El protocolo serial implementado reproduce exactamente la tabla de la sección 5 (`PING`, `TIMBRE PULSE/HOLD/RELEASE`, `PIR HIGH/LOW/PULSE`, `SAFE`) con respuestas `OK <eco>` / `ERR <motivo>`, y la estructura de carpetas coincide con la sección 7 más dos `__init__.py` justificados para ejecutar como paquete.

Las observaciones restantes son de robustez defensiva, no de corrección: el server atrapa `PicoError` pero no excepciones genéricas inesperadas en la ruta a hilo, y el manejo de `EOFError` en el firmware es un bucle cerrado sin pausa.

<details>
<summary>Issues (2)</summary>

1. **Captura estrecha en `_enviar_pico`** (possible) — el envoltorio en `server.py` solo atrapa `PicoError`; una excepción no prevista en `pico.enviar` (p. ej. de codificación o de pyserial no envuelta) escaparía a la capa MCP. Ampliar a un `except Exception` adicional que degrade a `{"ok": False, "error": ...}` cerraría la última vía de traza cruda. No bloqueante: las rutas conocidas de fallo ya están cubiertas.
2. **`continue` tras `EOFError` sin pausa** (likely) — en `main()` de la Pico, si `input()` lanzara `EOFError` de forma persistente (stdin cerrado), el `while True` giraría sin ceder, consumiendo CPU. En el uso real por USB no se da (input bloquea esperando línea), pero una pausa breve (`time.sleep_ms`) en esa rama lo haría robusto frente a un stdin degradado.

</details>

<details>
<summary>Details</summary>

### Estado seguro al arranque (GPIO0/D3) — correcto en ambos extremos

Este es el punto crítico del banco y está bien resuelto. En `pico/main.py`, `Manos.__init__` construye los dos pines con `Pin(PIN_TIMBRE, Pin.OUT, value=0)` y `Pin(PIN_PIR, Pin.OUT, value=0)`: el `value=0` fija el nivel BAJO en el mismo acto de construcción, antes de que ninguna otra lógica corra. Con el opto apagado, el pull-up interno del emisor mantiene D3 (GPIO0) en ALTO, que es el nivel que el ESP8266 necesita para arrancar en modo normal. `main()` además llama `manos.safe()` nada más instanciar, reafirmando el estado seguro antes de entrar al bucle de lectura. El flujo es: crear pines en BAJO → `safe()` → bucle. No hay ninguna escritura a GPIO entre el encendido y el estado seguro.

La guarda final `if _HW: main()` impide que el bucle (y por tanto cualquier toque de GPIO) se ejecute cuando el módulo se importa en CPython, donde `from machine import Pin` falla y `_HW` queda en `False`. Esto es lo que permite compilar y cargar el archivo sin hardware, sin efectos colaterales sobre pines inexistentes.

En el server, `reset_banco()` manda `SAFE` y su docstring recuerda explícitamente dejar el banco en estado seguro para no tener el timbre accionado durante un reset del emisor. La cadena de defensa (arranque Pico + `safe()` + `reset_banco()` + `MAX_PULSE_MS`) cubre el riesgo de GPIO0 por los dos lados.

### Tolerancia a ausencia de hardware — apertura perezosa en ambos enlaces

`PicoLink.__init__` solo guarda configuración y crea un lock; no toca el puerto. La apertura ocurre en `abrir()`, invocada perezosamente desde `enviar()`. Si pyserial no está, si no hay puerto configurado o si el sistema no puede abrirlo, lanza `PicoNoDisponible` con un mensaje contextual en vez de propagar una `SerialException` cruda; un timeout de lectura se traduce a `PicoTimeout`. Un fallo de E/S a mitad de un comando cierra el puerto y lo marca como no disponible, de modo que la siguiente llamada reintenta la apertura limpia.

`EmisorSerial.iniciar()` sigue la misma filosofía pero devolviendo `bool` en lugar de lanzar: ante falta de pyserial, de puerto o de apertura, deja el motivo en `ultimo_error` y devuelve `False`. El hilo lector (`_bucle_lectura`) tolera una desconexión en caliente: ante un error de lectura marca el puerto caído, lo cierra y termina el hilo sin tumbar el proceso, dejando que un `iniciar()` posterior lo reintente.

En la capa MCP, `_enviar_pico` envuelve la llamada y traduce `PicoError` a `{"ok": False, "error": ...}`. Aquí está la única grieta de robustez: la captura es `except PicoError`, no `except Exception`. Las rutas de fallo conocidas (`PicoNoDisponible`, `PicoTimeout`) heredan de `PicoError` y quedan cubiertas, pero una excepción inesperada que `pico.enviar` no envuelva (por ejemplo un error de codificación, o un fallo de pyserial que no caiga en el `try` interno) escaparía a través de `anyio.to_thread.run_sync` hasta la capa de herramienta MCP. El impacto es bajo porque `enviar()` ya envuelve su E/S, pero ampliar la captura a un `except Exception` final cerraría la última vía de traza cruda y mantendría la promesa de "mensaje claro, no excepción". Confianza: possible.

### Superficie MCP y docstrings — coherente con el diseño

Las seis herramientas registradas coinciden una a una con la sección 6 del documento: `estado_banco`, `pulsar_timbre`, `disparar_pir`, `mantener_pir`, `leer_serial_emisor`, `reset_banco`. La evidencia del coder confirma que `list_tools()` devuelve exactamente esos seis nombres. Los docstrings están en español y escritos para que el LLM sepa cuándo usar cada herramienta y qué buscar en la respuesta: `pulsar_timbre` menciona el antirrebote de 800 ms, `disparar_pir` el de 200 ms, `estado_banco` aclara que no acciona nada y sirve de primer paso. Las firmas con valores por defecto (`ms=50`, `ventana_ms=2000`, `ms=1000`) son razonables para los flujos de prueba descritos en la sección 8.

### Concurrencia — el event loop no se bloquea

Las herramientas son `async`. La ventana de correlación se espera con `anyio.sleep(ventana_ms/1000.0)`, que cede el loop. La E/S serial puntual (write + readline con timeout) se ejecuta fuera del loop vía `anyio.to_thread.run_sync(pico.enviar, comando)`. El `EmisorSerial` lee en su propio hilo de fondo (daemon) y solo expone consultas sobre un buffer con lock, sin bloquear. El único `sleep` bloqueante del sistema vive dentro de la Pico (`time.sleep_ms` durante el pulso), que es intencional: el firmware es secuencial y los pulsos son cortos. `MAX_PULSE_MS = 10000` acota esa duración.

### Protocolo serial — coincide con la sección 5

`procesar_comando` implementa el protocolo tal cual: `PING` → `PONG v1.0`; `TIMBRE PULSE <ms>`, `TIMBRE HOLD`, `TIMBRE RELEASE`; `PIR HIGH`, `PIR LOW`, `PIR PULSE <ms>`; `SAFE`. Las respuestas son `OK <eco>` o `ERR <motivo>` sin el salto final, y hay mensajes de error específicos para subcomando ausente, ms ausente y ms inválido. El parseo es case-insensitive en el comando y acota el ms con `_clamp_ms` tras validarlo con `_parse_ms`. El lado del server construye los comandos con el mismo formato (`"TIMBRE PULSE %d"`, `"PIR PULSE %d"`, `"SAFE"`, `"PING"`), por lo que ambos extremos hablan el mismo dialecto.

### Límites de seguridad del pulso

`MAX_PULSE_MS = 10000` y `_clamp_ms` acotan toda duración de pulso al rango [0, 10000] ms, aplicándose en ambos caminos de pulso (`TIMBRE PULSE` y `PIR PULSE`) a través de `Manos._pulse`. La respuesta devuelve los ms efectivamente aplicados (`OK TIMBRE PULSE <ms>`), de modo que el LLM ve si su petición fue recortada. Esto cumple la salvaguarda anti-bloqueo de pin de la sección 9 y protege específicamente a GPIO0 de quedar en LOW indefinidamente por un pulso excesivo.

### Robustez del bucle de la Pico — `EOFError`

En `main()`, el bucle atrapa `EOFError` de `input()` y hace `continue`. En el uso real por USB, `input()` bloquea esperando una línea completa, así que esta rama rara vez se toma. Pero si stdin quedara en estado de EOF persistente, el `while True` giraría sin ceder y consumiría CPU. Una pausa breve (`time.sleep_ms(10)`) en esa rama haría el firmware robusto frente a un stdin degradado sin afectar la latencia normal. Confianza: likely. No bloqueante.

### Estructura de carpetas — coincide con la sección 7

El árbol real (`__init__.py` raíz, `docs/DISENO_BANCO_HIL.md`, `pico/main.py`, `mcp_server/{server,pico_link,emisor_serial,__init__,requirements}.py`, `README.md`) reproduce la sección 7. Los dos `__init__.py` que no aparecen en el diagrama del doc están justificados: habilitan `python -m hil_bench.mcp_server.server` como paquete, que es la invocación registrada en el `mcp.json` del README. `requirements.txt` fija `mcp[cli]>=1.2,<2` con un comentario que explica por qué (en 2.x FastMCP se renombró), decisión respaldada por la evidencia del coder, y `pyserial>=3.5,<4`.

### Idioma y evidencia de verificación

Todo el código, docstrings, comentarios y mensajes de protocolo están en español, incluidos los `ERR <motivo>`. La evidencia del coder en `.agents/tasks/verificacion-hil-bench.md` documenta py_compile de los seis archivos, imports de los tres módulos del server, `list_tools()` con los seis nombres, y la carga de `pico/main.py` en CPython con `_HW=False` probando `procesar_comando('PING') -> 'PONG v1.0'` y `procesar_comando('FOO') -> 'ERR comando desconocido'`. La evidencia es concreta y suficiente; no se reejecutaron las suites conforme a la instrucción.

</details>

<details>
<summary>Mapa de archivos</summary>

- `hil_bench/__init__.py` — docstring del paquete raíz.
- `hil_bench/pico/main.py` — firmware MicroPython de las "manos": estado seguro al arranque, parseo del protocolo, pulsos acotados.
- `hil_bench/mcp_server/server.py` — FastMCP/stdio: define las seis herramientas, patrón acción→reacción sin bloquear el loop.
- `hil_bench/mcp_server/pico_link.py` — enlace serial a la Pico (apertura perezosa, excepciones propias).
- `hil_bench/mcp_server/emisor_serial.py` — lectura en hilo del serial del emisor con buffer circular por timestamp.
- `hil_bench/mcp_server/__init__.py` — docstring del paquete del server.
- `hil_bench/mcp_server/requirements.txt` — `mcp[cli]<2`, `pyserial`.
- `hil_bench/README.md` — cableado, flasheo, deps, puertos COM, `mcp.json`, sesión de prueba.
- `hil_bench/docs/DISENO_BANCO_HIL.md` — especificación autoritativa (preexistente).

</details>
