# Verificación del banco HIL (`hil_bench`) — sin hardware presente

Esta es la **primera iteración**: no existía
`.agents/tasks/review-hil-bench.json`, así que el banco se implementó desde
cero siguiendo `hil_bench/docs/DISENO_BANCO_HIL.md`.

Entorno: Windows, PowerShell, **Python 3.10.2**. CWD de todos los comandos:
`c:\Users\Alex\Desktop\1-Github\wifi_PIR` (raíz del repo).

## Dependencias

`pyserial 3.5` ya estaba instalado. `mcp` no lo estaba.

- `pip install "mcp>=1.2.0"` → instaló **mcp 2.3.0**, donde `FastMCP` se
  renombró a `MCPServer` y la API cambió (`import mcp.server.fastmcp` falla con
  un mensaje de migración). La tarea exige la API **FastMCP** (`@mcp.tool`,
  `mcp.run(transport="stdio")`), así que se corrigió el pin.
- `pip install "mcp<2"` → instaló **mcp 1.30.0**. Resultado: ✅
- Verificación de la API: `python -c "from mcp.server.fastmcp import FastMCP;
  m=FastMCP('x'); print(hasattr(m,'tool'), hasattr(m,'run'))"` → `tool: True
  run: True`. ✅

Por eso `requirements.txt` fija `mcp[cli]>=1.2,<2` (documentado en el propio
archivo). Nota: pip avisó de un conflicto de `starlette` con `platformio` (que
no forma parte de este banco); no afecta a `hil_bench`.

## Comandos de verificación y resultados

| Comando | Resultado |
|---------|-----------|
| `python -c "import hil_bench.mcp_server.server; print('import server OK')"` | ✅ `import server OK` |
| `python -c "import hil_bench.mcp_server.pico_link, hil_bench.mcp_server.emisor_serial"` | ✅ `imports OK` |
| `python -m py_compile hil_bench\pico\main.py hil_bench\mcp_server\server.py hil_bench\mcp_server\pico_link.py hil_bench\mcp_server\emisor_serial.py hil_bench\__init__.py hil_bench\mcp_server\__init__.py` | ✅ exit 0 |
| `python -c "import asyncio, hil_bench.mcp_server.server as s; print([t.name for t in asyncio.run(s.mcp.list_tools())])"` | ✅ `['estado_banco', 'pulsar_timbre', 'disparar_pir', 'mantener_pir', 'leer_serial_emisor', 'reset_banco']` |
| Carga de `pico/main.py` en CPython + parseo | ✅ `_HW=False`; `procesar_comando('PING') -> 'PONG v1.0'`; `procesar_comando('FOO') -> 'ERR comando desconocido'` |

Observaciones:

- El server **se importa e instancia sin abrir ningún puerto** (apertura
  perezosa). Las herramientas registran bien y el banco se puede cargar sin
  Pico ni emisor conectados.
- `pico/main.py` **compila y se carga en CPython** porque `from machine import
  Pin` está aislado en `try/except ImportError`; con `_HW=False` el bucle
  `main()` no se ejecuta y no se toca GPIO. En la Pico el import tiene éxito
  (`_HW=True`) y arranca el bucle. El archivo sigue siendo válido en MicroPython.

No se abrió ningún puerto serie real (no hay hardware en el entorno de build),
tal como pide la tarea.

## Estructura final de `hil_bench/`

```
hil_bench/
├── __init__.py                 docstring del paquete raíz
├── README.md                   cableado, flasheo, deps, puertos COM, mcp.json, sesión de prueba
├── docs/
│   └── DISENO_BANCO_HIL.md      (especificación autoritativa, ya existía)
├── pico/
│   └── main.py                  firmware MicroPython de las "manos"
└── mcp_server/
    ├── __init__.py              docstring del paquete del server
    ├── server.py                FastMCP (stdio): define las herramientas
    ├── pico_link.py             enlace serial hacia la Pico (pyserial)
    ├── emisor_serial.py         lectura en hilo del serial del emisor
    └── requirements.txt         mcp[cli]<2, pyserial
```

Confrontada con la sección 7 del documento de diseño: **coincide**. Añadidos
respecto al diagrama del doc: los dos `__init__.py` (necesarios para que
`python -m hil_bench.mcp_server.server` funcione como paquete, tal como pide la
tarea).

## Qué queda por probar con hardware conectado

- Puertos COM reales (`PICO_PORT` / `EMISOR_PORT`) y apertura efectiva del serial.
- Flasheo de la Pico (`pico/main.py` como `main.py`) y respuesta real al protocolo.
- Lazo cerrado completo estímulo → lectura del serial del emisor.
- Antirrebotes en hardware real: 800 ms (timbre) y 200 ms (PIR).
