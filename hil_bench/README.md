# Banco de pruebas HIL genérico — "manos y oídos" del LLM

Esta herramienta le da al LLM (en Kiro) **manos** para accionar las entradas
físicas de un dispositivo embebido bajo prueba (DUT) y **oídos** para leer su
monitor serial, sin que tú tengas que apretar botones. Es hardware real con
acople aislado, controlado por un servidor MCP.

Es **genérica y reutilizable en cualquier proyecto**: el banco razona en
primitivas de actuador (`boton`, `interruptor`), y cada proyecto describe sus
canales en un archivo `bench.yaml`. El proyecto `wifi_PIR` es solo un ejemplo
(ver [`examples/wifi_pir/bench.yaml`](examples/wifi_pir/bench.yaml)).

```
   LLM (Kiro)
     │  herramientas MCP (stdio)
     ▼
   MCP server (Python, FastMCP)              ← sistema nervioso
     │   lee bench.yaml: etiqueta -> GPIO, tipo, polaridad
     ├── USB → Raspberry Pi Pico (MicroPython)   = MANOS
     │            GPIO → optoacoplador PC817 → pin del DUT
     └── USB → DUT bajo prueba (115200)          = OÍDOS (lee los logs)
```

Documento de diseño completo (arquitectura, vocabulario, pines, protocolo,
materiales): [`docs/DISENO_BANCO_HIL.md`](docs/DISENO_BANCO_HIL.md). Esto es un
resumen operativo.

## 1. Conceptos

- **DUT**: el dispositivo bajo prueba (cuyas entradas accionas y cuyo serial
  lees). En el ejemplo wifi_PIR es un D1 mini (ESP8266).
- **canal**: una entrada física del DUT que el banco puede accionar. Tiene una
  **etiqueta** libre (`timbre`, `pir`, `reset`…), un **tipo**, un **GPIO** de la
  Pico, una **polaridad** y metadatos.
- **tipos de canal**:
  - `boton`: actuador momentáneo (se pulsa y se suelta). Un timbre, un reset.
  - `interruptor`: actuador sostenido on/off. Un PIR, un switch, un enable.
  - `analogico`: diseñado pero **aún no implementado** (fijar un nivel vía
    DAC/pot digital).

## 2. Describir tu banco con `bench.yaml`

Crea un YAML con los canales de tu proyecto. Ejemplo (el de wifi_PIR):

```yaml
nombre: wifi_PIR
pico_port: null            # o COMx; env PICO_PORT tiene prioridad
dut_port: null             # o COMx; env DUT_PORT tiene prioridad
baudrate: 115200
canales:
  - etiqueta: timbre
    tipo: boton
    gpio_pico: 2            # GP2 de la Pico
    polaridad: activo_bajo  # el DUT lee 'activo' cuando el pin va a su nivel activo
    antirrebote_ms: 800     # informativo: cuánto esperar entre estímulos
    descripcion: 'Pulsador de timbre, D3/GPIO0 del emisor, INPUT_PULLUP activo en LOW'
  - etiqueta: pir
    tipo: interruptor
    gpio_pico: 3
    polaridad: activo_alto
    antirrebote_ms: 200
    descripcion: 'Entrada PIR, D2/GPIO4 del emisor, pull-down, activo en ALTO'
```

Reglas: etiquetas y GPIO únicos; `tipo` ∈ {boton, interruptor}; `polaridad` ∈
{activo_alto, activo_bajo}. Si el YAML falta o es inválido, el server arranca en
**modo degradado** y lo reporta por `listar_canales`/`estado_banco` (no rompe).

### Polaridad y estado seguro

Convención física (para todo canal): **GPIO de la Pico en ALTO = opto activo =
entrada ESTIMULADA**; GPIO en BAJO = reposo. El cableado de cada canal (pull-up
o pull-down en el pin del DUT) se diseña para que esto se cumpla, así que la
`polaridad` describe la semántica del DUT y guía el cableado; no invierte el
GPIO. El estado seguro de todo canal es "no estimulado" (GPIO en BAJO), lo que
respeta pines boot-strap como GPIO0/D3. Ver el documento de diseño, sección 3.

## 3. Cablear cualquier DUT (resumen)

Un optoacoplador PC817 por canal, con las masas de la Pico y el DUT **aisladas**
(no se unen). Detalle y esquemas en [`docs/DISENO_BANCO_HIL.md`](docs/DISENO_BANCO_HIL.md)
sección 5:

- **Canal activo en LOW**: `GPIO Pico --[330Ω]--> LED del PC817`; el
  fototransistor lleva el pin del DUT a GND cuando el opto conduce = "estimulado".
- **Canal activo en ALTO**: el fototransistor conmuta 3.3 V hacia el pin del DUT
  (con pull-down de 10 kΩ en el pin y ~1 kΩ de colector) = "estimulado".

En ambos casos, GPIO de la Pico en ALTO = opto activo = entrada estimulada.

### ⚠️ Precaución — pines boot-strap del DUT (ejemplo: GPIO0/D3 del ESP8266)

Algunos pines del DUT son de arranque. En wifi_PIR, `D3` = GPIO0: si está en LOW
durante el reset/encendido, el ESP entra en modo flash y no arranca su firmware.
El canal del timbre es `activo_bajo` y su opto en reposo deja D3 en alto por el
pull-up, así el estado seguro no lo activa. La Pico arranca **siempre en estado
seguro**. Enciende/resetea el DUT con el banco en reposo (`reset_banco()` o Pico
recién arrancada).

## 4. Flashear la Pico (MicroPython)

1. Instala MicroPython en la Pico (arrástrale el `.uf2` oficial en modo
   BOOTSEL).
2. Copia [`pico/main.py`](pico/main.py) a la **raíz** de la Pico con el nombre
   `main.py` (p.ej. con Thonny o `mpremote`). Al nombrarse `main.py`, MicroPython
   lo ejecuta solo al encender.

El firmware es **genérico**: opera GPIO por número (`PULSE <gpio> <ms>`,
`SET <gpio> <0|1>`, `SAFE`, `PING`). No necesitas tocarlo para cambiar de
proyecto; el mapa etiqueta→GPIO vive en tu `bench.yaml`. El protocolo completo
está en el documento de diseño (sección 7).

## 5. Instalar las dependencias del MCP server

Desde la raíz del repo:

```powershell
pip install -r hil_bench/mcp_server/requirements.txt
```

Dependencias: `mcp[cli]` (<2, por la API FastMCP), `pyserial` y `pyyaml`.

## 6. Encontrar los puertos COM en Windows

- **Administrador de dispositivos → Puertos (COM y LPT)**: aparecen la Pico y el
  DUT (desconecta y reconecta uno para ver cuál cambia).
- Por línea de comandos:

```powershell
python -m serial.tools.list_ports -v
```

Anota cuál es la Pico (p.ej. `COM5`) y cuál el DUT (p.ej. `COM6`).

## 7. Registrar el server en Kiro (`.kiro/settings/mcp.json`)

```json
{
  "mcpServers": {
    "hil-bench": {
      "command": "python",
      "args": ["-m", "hil_bench.mcp_server.server"],
      "env": {
        "HIL_BENCH_CONFIG": "hil_bench/examples/wifi_pir/bench.yaml",
        "PICO_PORT": "COM5",
        "DUT_PORT": "COM6",
        "HIL_BAUDRATE": "115200"
      },
      "disabled": false,
      "autoApprove": ["listar_canales", "estado_banco", "leer_serial_dut"]
    }
  }
}
```

- `HIL_BENCH_CONFIG` apunta al `bench.yaml` de tu proyecto (para otro proyecto,
  cambia la ruta a su propio YAML).
- `PICO_PORT` / `DUT_PORT` tienen **prioridad** sobre lo que diga el YAML, así
  la misma config sirve en varias máquinas.

Ejecuta `python` desde la raíz del repo para que el paquete `hil_bench` sea
importable (o ajusta `cwd`/`PYTHONPATH`). El server se puede cargar aunque no
haya hardware conectado ni YAML: las herramientas devolverán un mensaje claro en
vez de fallar.

## 8. Herramientas que expone el MCP

| Herramienta | Qué hace |
|-------------|----------|
| `listar_canales()` | canales del banco (etiqueta, tipo, gpio, polaridad, antirrebote_ms, descripción). Primer paso. |
| `estado_banco()` | PING a la Pico, ¿DUT emitiendo serial?, puertos, nombre del banco, nº de canales |
| `pulsar_boton(canal, ms=50)` | pulsa un canal `boton` y devuelve los logs del DUT que aparezcan |
| `fijar_interruptor(canal, estado)` | sostiene un canal `interruptor` en `alto`/`bajo` (o `activo`/`reposo`) y lee la reacción |
| `leer_serial_dut(ventana_ms=2000)` | últimas líneas del serial del DUT |
| `reset_banco()` | lleva todos los canales a estado seguro (`SAFE`) |

Las herramientas de estímulo correlacionan **acción → reacción**: accionan,
esperan una ventana corta (derivada del `antirrebote_ms` del canal + margen) y
devuelven lo que imprimió el DUT.

## 9. Ejemplo de sesión de prueba (wifi_PIR)

### Antirrebote del timbre (800 ms)

1. `pulsar_boton("timbre", 50)` → en `dut_logs` debe aparecer la detección del
   timbre y el EVENT encolado.
2. `pulsar_boton("timbre", 50)` inmediato → dentro de los 800 ms **no** debe
   emitir otro EVENT (antirrebote).
3. espera ~900 ms y `pulsar_boton("timbre", 50)` → vuelve a emitir. Antirrebote
   confirmado en hardware real.

### Movimiento del PIR

1. `fijar_interruptor("pir", "alto")` → debe aparecer la detección de movimiento.
2. `fijar_interruptor("pir", "bajo")` → vuelve a reposo.

Cierra siempre con `reset_banco()` para dejar el banco en estado seguro.

## 10. Qué queda por probar con hardware conectado

- Puertos COM reales de la Pico y el DUT (`PICO_PORT` / `DUT_PORT`).
- Flasheo de la Pico y lazo cerrado completo (estímulo → lectura del serial).
- Verificación en hardware real de los antirrebotes del ejemplo wifi_PIR
  (800 ms timbre / 200 ms PIR).
