# Verificación — Generalización del banco HIL

Refactorización del banco de pruebas HIL de `hil_bench/` para convertirlo en una
herramienta GENÉRICA (vocabulario de actuadores `boton`/`interruptor`/`canal`/
`DUT`, configuración declarativa en `bench.yaml`), con wifi_PIR como ejemplo.

Entorno: Windows, PowerShell, Python 3.10.2. Dependencias presentes: `pyyaml`
6.0.2, `mcp` (SDK 1.x), `pyserial` 3.5. No hay hardware conectado (verificación
puramente software, como pedía la tarea). No se hizo commit (cambios en el
working tree).

## Qué se renombró / movió

- `mcp_server/emisor_serial.py` → `mcp_server/dut_serial.py`; clase
  `EmisorSerial` → `DutSerial`. Imports actualizados en `server.py`.
- Variable de entorno `EMISOR_PORT` → `DUT_PORT`.
- Herramienta `leer_serial_emisor` → `leer_serial_dut`.
- Herramientas de dominio `pulsar_timbre`/`disparar_pir`/`mantener_pir`
  eliminadas y sustituidas por genéricas (ver abajo).
- Nuevo `mcp_server/config.py` (modelo `Canal`/`BenchConfig`, carga/validación
  de YAML, merge con variables de entorno).
- Nuevo `examples/wifi_pir/bench.yaml` (el ejemplo wifi_PIR).
- Firmware `pico/main.py` reescrito a protocolo genérico por GPIO (v2.0).
- Docs (`docs/DISENO_BANCO_HIL.md`, `README.md`), `requirements.txt` (+pyyaml) y
  los `__init__.py` reescritos en clave genérica.

## Vocabulario nuevo

- **DUT** (dispositivo bajo prueba) sustituye a "emisor" en código y doc. El
  emisor D1 mini es solo el DUT del ejemplo wifi_PIR.
- **canal**: entrada física del DUT con etiqueta libre, tipo, GPIO de la Pico,
  polaridad y metadatos.
- **boton**: actuador momentáneo. **interruptor**: actuador sostenido on/off.
  **analogico**: diseñado pero NO implementado (`config.py` lo rechaza
  explícitamente).

## Herramientas MCP (genéricas)

`listar_canales()`, `estado_banco()`, `pulsar_boton(canal, ms=50)`,
`fijar_interruptor(canal, estado)`, `leer_serial_dut(ventana_ms=2000)`,
`reset_banco()`.

La ventana de correlación acción→reacción se deriva del `antirrebote_ms` del
canal + margen (400 ms), no de valores hardcodeados.

## Protocolo del firmware (enfoque A: Pico tonta por GPIO)

`PING`, `PULSE <gpio> <ms>`, `SET <gpio> <0|1>`, `SAFE`, `SAFE <g1> <g2> ...`.
El server resuelve etiqueta→GPIO desde el YAML y manda `SAFE <gpios>` al
conectar. Arranque en estado seguro; `MAX_PULSE_MS` acota pulsos; guarda
`try/except ImportError` de `machine` (`_HW`) para compilar en CPython. Se
corrigió el `continue` tras `EOFError` añadiendo `time.sleep_ms(50)` para evitar
busy-loop.

## Esquema YAML final

```yaml
nombre: wifi_PIR
pico_port: null            # o COMx; env PICO_PORT tiene prioridad
dut_port: null             # o COMx; env DUT_PORT tiene prioridad
baudrate: 115200           # env HIL_BAUDRATE tiene prioridad
canales:
  - etiqueta: timbre
    tipo: boton
    gpio_pico: 2
    polaridad: activo_bajo
    antirrebote_ms: 800
    descripcion: '...'
  - etiqueta: pir
    tipo: interruptor
    gpio_pico: 3
    polaridad: activo_alto
    antirrebote_ms: 200
    descripcion: '...'
```

Localización por `HIL_BENCH_CONFIG`. Sin esa env o sin archivo válido: modo
degradado (0 canales, `nota` explicativa), el server no rompe.

## Comandos de verificación ejecutados y resultados

### 1. Dependencias (`python --version` + imports)
```
Python 3.10.2
pyyaml 6.0.2
mcp ok
pyserial 3.5
```

### 2. py_compile de todos los .py (incluye pico/main.py con la guarda _HW)
```
python -m py_compile hil_bench/__init__.py hil_bench/pico/main.py \
  hil_bench/mcp_server/__init__.py hil_bench/mcp_server/config.py \
  hil_bench/mcp_server/server.py hil_bench/mcp_server/pico_link.py \
  hil_bench/mcp_server/dut_serial.py
=> py_compile exit: 0   (todos compilan)
```

### 3. Importar el server sin hardware ni YAML + listar herramientas MCP
```
python -c "import hil_bench.mcp_server.server as s; import anyio; \
  tools = anyio.run(s.mcp.list_tools); print(...)"
=>
CONFIG.cargado (sin env): False
nota: no hay banco cargado: define la variable de entorno HIL_BENCH_CONFIG ...
herramientas MCP: ['estado_banco', 'fijar_interruptor', 'leer_serial_dut',
                   'listar_canales', 'pulsar_boton', 'reset_banco']
```
El server se instancia en modo degradado (sin romper) y registra las 6
herramientas genéricas.

### 4. Carga del YAML de ejemplo con el loader de config.py
```
python -c "from hil_bench.mcp_server.config import cargar_config; \
  c = cargar_config('hil_bench/examples/wifi_pir/bench.yaml'); ... asserts"
=>
cargado: True | nombre: wifi_PIR | nota: None
num canales: 2
  - timbre boton GP2 activo_bajo 800
  - pir interruptor GP3 activo_alto 200
ASSERTS OK: 2 canales con tipos/polaridades correctos
```
(Check hecho con `python -c`, sin crear ni dejar scripts temporales.)

### 5. Server con HIL_BENCH_CONFIG definido (canales cargados)
```
$env:HIL_BENCH_CONFIG="hil_bench/examples/wifi_pir/bench.yaml"
python -c "import hil_bench.mcp_server.server as s; print(...)"
=>
banco: wifi_PIR | cargado: True | canales: ['timbre', 'pir'] | gpios: [2, 3]
```

### 6. Parser del firmware en CPython (_HW=False)
```
python -c "import hil_bench.pico.main as m; ..."
=>
_HW: False
PING -> PONG v2.0
parse PULSE sin args -> ERR PULSE requiere <gpio> <ms>
parse SET nivel malo  -> ERR SET nivel debe ser 0 o 1
desconocido           -> ERR comando desconocido
```

### 7. Prioridad de env sobre YAML en los puertos
```
$env:PICO_PORT="COM9"; $env:DUT_PORT="COM10"
python -c "...cargar_config('.../bench.yaml')..."
=> pico_port: COM9 | dut_port: COM10
```
(El YAML tiene null; las env ganan.)

### 8. Sin vocabulario de dominio residual en el código
```
grep (emisor|EMISOR_PORT|leer_serial_emisor|EmisorSerial|pulsar_timbre|
      disparar_pir|mantener_pir) en hil_bench/**/*.py
=> No matches found.
```

## Estructura final

```
hil_bench/
├── README.md                       (genérico; wifi_PIR como ejemplo)
├── __init__.py
├── docs/
│   └── DISENO_BANCO_HIL.md          (vocabulario genérico, esquema YAML)
├── pico/
│   └── main.py                      (firmware genérico por GPIO, v2.0)
├── mcp_server/
│   ├── __init__.py
│   ├── config.py                    (NUEVO: carga/valida bench.yaml, Canal)
│   ├── server.py                    (herramientas genéricas; canal->gpio)
│   ├── pico_link.py                 (+ pulse_gpio/set_gpio/safe)
│   ├── dut_serial.py                (RENOMBRADO de emisor_serial.py; DutSerial)
│   └── requirements.txt             (+ pyyaml)
└── examples/
    └── wifi_pir/
        └── bench.yaml               (NUEVO: ejemplo wifi_PIR)
```
(`emisor_serial.py` eliminado.)

## Qué queda por probar con hardware conectado

- Puertos COM reales de la Pico y el DUT (`PICO_PORT` / `DUT_PORT`).
- Flasheo de `pico/main.py` como `main.py` en la Pico y lazo cerrado completo
  (estímulo → lectura del serial del DUT).
- Que el `SAFE <gpios>` inicial del server deja los canales en reposo y respeta
  GPIO0/D3 durante el reset del emisor.
- Verificación en hardware de los antirrebotes del ejemplo wifi_PIR (800 ms
  timbre vía `pulsar_boton("timbre")`, 200 ms PIR vía `fijar_interruptor`).
```
