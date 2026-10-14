# Banco de pruebas HIL genérico — "manos y oídos" del LLM

> Objetivo: que el LLM (en Kiro) accione las entradas físicas de **cualquier**
> dispositivo embebido bajo prueba (DUT) y lea su monitor serial, sin que el
> usuario tenga que apretar botones. Hardware real, acople aislado, control vía
> un MCP server.
>
> La herramienta es **genérica y reutilizable** entre proyectos. wifi_PIR es
> solo un ejemplo (ver `examples/wifi_pir/bench.yaml`).

## 1. Arquitectura

```
   LLM (Kiro)
     │  llamadas a herramientas MCP (stdio)
     ▼
   MCP server (Python, FastMCP, en el PC)        ← sistema nervioso
     │   lee bench.yaml: mapa etiqueta -> GPIO, tipo, polaridad
     ├── serial USB 1 ──→ Raspberry Pi Pico (MicroPython)   = MANOS
     │                       GPIO → optoacoplador PC817 → pin del DUT
     └── serial USB 2 ──→ DUT bajo prueba (ESP8266, etc.)   = OÍDOS
                             lee logs: ¿flanco? ¿antirrebote? ¿evento?
```

Dos microcontroladores, dos cables USB al PC, un acople aislado entre ellos. El
MCP server es la única fuente de verdad de la semántica del banco (la lee del
YAML); la Pico es una ejecutora de GPIO "tonta" y 100% genérica.

## 2. Vocabulario genérico

El core razona en **primitivas de actuador**, no en conceptos de dominio:

| Concepto | Qué es |
|----------|--------|
| **DUT** (dispositivo bajo prueba) | el aparato cuyas entradas accionamos y cuyo serial leemos. Sustituye a "emisor"; el emisor D1 mini es el DUT del ejemplo wifi_PIR. |
| **canal** | una entrada física concreta del DUT que el banco puede accionar. Tiene una **etiqueta** libre (p.ej. `timbre`, `pir`, `reset`), un **tipo**, un **GPIO** de la Pico, una **polaridad** y metadatos. |
| **boton** | actuador **momentáneo** (se pulsa y se suelta solo). Un timbre, un reset, cualquier pulsador. |
| **interruptor** | actuador **sostenido** on/off. Un PIR, un switch, un enable. |
| **analogico** | *(diseñado, aún NO implementado)* fijar un nivel vía DAC/pot digital. |

Las etiquetas `timbre`/`pir` son solo nombres que el usuario asigna en su
`bench.yaml`; el código no sabe qué es un "timbre", solo que es un `boton` en
tal GPIO.

### Tipo futuro `analogico` (diseño, no implementado)

Un canal `analogico` fijaría un **nivel** (p.ej. 0–3.3 V o un valor 0–255) en
una entrada analógica del DUT, usando un DAC (MCP4725 por I2C) o un
potenciómetro digital (MCP41010 por SPI) gobernado por la Pico. El protocolo
sumaría un comando tipo `LEVEL <id> <valor>` y el server una herramienta
`fijar_analogico(canal, valor)`. Hoy `config.py` **rechaza** explícitamente el
tipo `analogico` para no dar falsa sensación de soporte.

## 3. Canal: configuración declarativa (bench.yaml)

Cada banco concreto se describe en un archivo YAML. Esquema:

```yaml
nombre: wifi_PIR            # opcional, informativo
pico_port: null            # o COMx; env PICO_PORT tiene prioridad
dut_port: null             # o COMx; env DUT_PORT tiene prioridad
baudrate: 115200           # env HIL_BAUDRATE tiene prioridad
canales:
  - etiqueta: timbre        # nombre libre del canal en este proyecto
    tipo: boton             # boton | interruptor
    gpio_pico: 2            # GP2 de la Pico
    polaridad: activo_bajo  # activo_alto | activo_bajo (semántica del DUT)
    antirrebote_ms: 800     # informativo: cuánto esperar entre estímulos
    descripcion: '...'      # opcional
```

Reglas de validación (`mcp_server/config.py`): etiquetas y GPIO únicos; `tipo`
en {boton, interruptor}; `polaridad` en {activo_alto, activo_bajo};
`gpio_pico`/`antirrebote_ms` enteros ≥ 0. Un YAML inválido no rompe el server:
arranca en modo degradado y lo reporta por `listar_canales`/`estado_banco`.

### Polaridad y estado seguro (importante)

Convención **física** del banco (vale para todo canal, cualquier polaridad):

> **GPIO de la Pico en ALTO = LED del opto encendido = opto activo = entrada
> del DUT ESTIMULADA.** GPIO en BAJO = opto apagado = canal en REPOSO.

Esto se cumple para ambas polaridades porque el **cableado de cada canal**
(pull-up o pull-down en el pin del DUT, y a qué conmuta el fototransistor) se
diseña para que "opto activo" signifique "entrada estimulada" en ese canal
concreto. Por eso la abstracción mapea siempre `estimular -> GPIO ALTO` y
`reposo -> GPIO BAJO`, y la `polaridad` describe la semántica del DUT y guía el
cableado; **no** invierte el GPIO de la Pico.

El estado **SEGURO/REPOSO** de todo canal es siempre "no estimulado" = GPIO en
BAJO. Esto respeta la precaución de pines boot-strap: un canal `activo_bajo`
sobre un pin de arranque (p.ej. GPIO0/D3 del ESP8266) nunca queda activo durante
el reset del DUT, porque su opto apagado deja el pin en alto por su pull-up.

## 4. Ejemplo wifi_PIR: pines reales del emisor (device_config.cpp)

| Señal | Pin D1 mini | GPIO | Modo en firmware | Activo | Tipo de canal |
|-------|-------------|------|------------------|--------|---------------|
| PIR    | `D2` | GPIO4 | entrada | ALTO = movimiento | `interruptor` |
| TIMBRE | `D3` | GPIO0 | `INPUT_PULLUP` | **LOW = pulsado** | `boton` |

### Precaución crítica — GPIO0 (D3)
GPIO0 es un **pin de arranque (boot strap)** del ESP8266. Si está en LOW durante
el reset/encendido, el ESP entra en **modo flash** y no ejecuta el firmware.
Regla de diseño: el canal del timbre es `activo_bajo` y su opto en reposo
(apagado) deja D3 en alto por el pull-up, así el **estado seguro no lo activa**.
Las manos solo lo pulsan en ventanas cortas cuando el emisor ya corre. El
firmware de la Pico arranca con todas sus salidas en estado seguro.

## 5. Acople: optoacoplador PC817 (uno por canal)

El PC817 aísla eléctricamente la Pico del DUT. Lado LED lo controla la Pico;
lado fototransistor "cierra el botón" o inyecta nivel en el DUT.

### Canal activo en LOW (ejemplo: timbre; es el caso natural del opto)
```
  Pico GPIO ──[ R 330Ω ]──►|── PC817 ──┐ (LED, ánodo→cátodo)
                            LED        │
  Pico GND ────────────────────────────┘

  DUT pin  ───────── colector PC817
  DUT GND  ───────── emisor   PC817
```
- Pico pone GPIO en ALTO → LED conduce → fototransistor conduce → **pin del DUT
  cae a GND = "estimulado"**. GPIO en BAJO → opto abierto → pin en alto por el
  pull-up = reposo. Perfecto para activo-en-LOW.

### Canal activo en ALTO (ejemplo: PIR; el opto debe entregar 3.3 V, no GND)
El PIR real entrega **ALTO** al detectar. Con un solo PC817 el fototransistor
lleva a GND, lo contrario. Opción recomendada: el fototransistor conmuta 3.3 V
hacia el pin a través de una resistencia, con **pull-down** en el pin.
```
  DUT 3V3 ──[ R 1kΩ ]── colector PC817
  DUT pin ─────┬──────── emisor PC817
               │
             [ R 10kΩ ]  (pull-down)
               │
  DUT GND ─────┘
```
- Opto activo → pin a 3.3 V = "estimulado"; opto inactivo → pin a GND por el
  pull-down = reposo.

> Cada opto tiene su resistencia de LED (lado Pico, ~330Ω para 3.3 V) y su
> resistencia de colector (lado DUT). Las masas de Pico y DUT **no** se unen:
> ese es el punto del aislamiento; solo se "tocan" por la luz del opto.

## 6. Lista de materiales (ejemplo wifi_PIR: PIR + timbre digitales)

| Cant. | Componente | Nota |
|-------|-----------|------|
| 1 | Raspberry Pi Pico (o Pico W) | manos, MicroPython |
| 1 | DUT (en el ejemplo, Wemos D1 mini ESP8266) | dispositivo bajo prueba |
| 2 | Optoacoplador PC817 | uno por canal |
| 2 | Resistencia 330Ω | LED de cada opto (lado Pico) |
| 1 | Resistencia 1kΩ | colector del opto del canal activo-alto (a 3V3 del DUT) |
| 1 | Resistencia 10kΩ | pull-down del pin activo-alto |
| 2 | Cable USB | uno a la Pico, uno al DUT, ambos al PC |
| — | Protoboard + jumpers | montaje |

Escalado futuro (canales `analogico`): + MCP4725 (DAC I2C) o MCP41010 (pot
digital SPI) gobernado por la Pico.

## 7. Protocolo serial Pico ↔ MCP server (genérico por GPIO)

Texto por líneas, 115200 baud, determinista. La Pico opera **por número de
GPIO**; el mapa etiqueta→GPIO vive en el server (desde `bench.yaml`):

| Comando (PC→Pico) | Acción |
|-------------------|--------|
| `PING` | responde `PONG vX.Y` (comprobar vida) |
| `PULSE <gpio> <ms>` | pone `<gpio>` ALTO durante `<ms>` (acotado) y lo baja |
| `SET <gpio> <0\|1>` | fija `<gpio>` en BAJO (0) o ALTO (1), sostenido |
| `SAFE` | todos los GPIO ya tocados a BAJO (reposo) |
| `SAFE <g1> <g2> …` | registra esos GPIO y los pone a BAJO |

Respuestas: `OK <eco>` o `ERR <motivo>`. La Pico aplica `MAX_PULSE_MS` como
salvaguarda. Al conectar, el server manda `SAFE <gpios de sus canales>` para
dejar en reposo exactamente los pines declarados (respetando pines boot-strap).

Se eligió el **enfoque A** (Pico tonta por GPIO, YAML como única fuente de
verdad) frente a enviar `CONFIG` por canal: menos estado en la Pico, firmware
genérico sin recompilar entre proyectos, y una sola fuente de verdad.

## 8. Herramientas que expone el MCP server (lo que el LLM llama)

| Herramienta | Qué hace |
|-------------|----------|
| `listar_canales()` | canales del banco: etiqueta, tipo, gpio, polaridad, antirrebote_ms, descripción. Primer paso. |
| `estado_banco()` | PING a la Pico, ¿DUT emitiendo serial?, puertos, nombre del banco, nº de canales |
| `pulsar_boton(canal, ms=50)` | valida canal tipo `boton`; pulso; devuelve la reacción del DUT |
| `fijar_interruptor(canal, estado)` | valida canal tipo `interruptor`; sostiene `alto`/`bajo` (`activo`/`reposo`); reacción |
| `leer_serial_dut(ventana_ms=2000)` | últimas líneas del serial del DUT |
| `reset_banco()` | SAFE: todos los canales a reposo |

Las herramientas de estímulo correlacionan **acción → reacción**: accionan,
esperan una ventana corta y devuelven lo que imprimió el DUT. La ventana se
**deriva del `antirrebote_ms` del canal + un margen**, no de valores
hardcodeados.

## 9. Estructura del proyecto

```
hil_bench/
├── docs/
│   └── DISENO_BANCO_HIL.md       (este documento)
├── pico/
│   └── main.py                    firmware MicroPython genérico (por GPIO)
├── mcp_server/
│   ├── server.py                  FastMCP: herramientas genéricas; resuelve canal->gpio
│   ├── config.py                  carga/valida bench.yaml; modelo Canal; merge con env
│   ├── pico_link.py               serial hacia la Pico (pulse_gpio/set_gpio/safe)
│   ├── dut_serial.py              lectura del serial del DUT (clase DutSerial)
│   └── requirements.txt           mcp[cli], pyserial, pyyaml
├── examples/
│   └── wifi_pir/
│       └── bench.yaml             el ejemplo wifi_PIR (timbre + pir)
└── README.md                      cómo describir, cablear y registrar un banco
```

Registro en Kiro (ejemplo, `.kiro/settings/mcp.json`):
```json
{
  "mcpServers": {
    "hil-bench": {
      "command": "python",
      "args": ["-m", "hil_bench.mcp_server.server"],
      "env": {
        "HIL_BENCH_CONFIG": "hil_bench/examples/wifi_pir/bench.yaml",
        "PICO_PORT": "COM5",
        "DUT_PORT": "COM6"
      },
      "disabled": false
    }
  }
}
```

## 10. Flujo de una prueba (ejemplo wifi_PIR)

Para verificar el antirrebote del timbre (800 ms), con el canal `timbre`:
1. `pulsar_boton("timbre", 50)` → en `dut_logs` debe aparecer "TIMBRE detectado / EVENT encolado".
2. `pulsar_boton("timbre", 50)` inmediato → **no** debe emitir otro EVENT (dentro de los 800 ms).
3. esperar ~900 ms, `pulsar_boton("timbre", 50)` → vuelve a emitir. Antirrebote confirmado en hardware real.

Para el PIR (`interruptor`): `fijar_interruptor("pir", "alto")` para simular
movimiento, leer la reacción, y `fijar_interruptor("pir", "bajo")` (o
`reset_banco()`) para volver a reposo.

## 11. Seguridad y límites

- Masas aisladas (ese es el punto del opto): no unir GND de Pico y DUT.
- Pulsos acotados en la Pico (`MAX_PULSE_MS`, salvaguarda anti-bloqueo de pin).
- Estado seguro al arranque de la Pico (crítico por pines boot-strap del DUT).
- El server manda `SAFE <gpios>` al conectar: solo gestiona pines declarados.
- El MCP server es local (stdio), no abre puertos de red.
- Nada de esto toca credenciales ni el repo versionado de firmware.
