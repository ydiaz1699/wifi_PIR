# 📋 PLANTILLA MEJORADA: ENTORNO VS CODE + PLATFORMIO + CATÁLOGO HARDWARE

---

# PLANTILLA 1: ENTORNO VS CODE + PLATFORMIO

## 🔧 Configuración Base

- **IDE:** VS Code + PlatformIO
- **Monitor Serial:** VS Code Serial Monitor (`ms-vscode.vscode-serial-monitor`)
- **Baudrate por defecto:** `115200` (ajustar por proyecto)
- **Framework:** Arduino / ESP-IDF / Zephyr (según plataforma)
- **Tema VS Code:** Claro/Oscuro (preferencia)

---

## 🧩 Extensiones VS Code Recomendadas

| Extensión | ID | Función |
| --- | --- | --- |
| PlatformIO IDE | `platformio.platformio-ide` | Core del workflow: build, upload, monitor, librerías |
| VS Code Serial Monitor | `ms-vscode.vscode-serial-monitor` | Monitor serial nativo con timestamps y formato hex |
| Better C++ Syntax | `jeff-hykin.better-cpp-syntax` | Syntax highlighting mejorado para C++ moderno |
| IBM Output Colorizer | `IBM.output-colorizer` | Colores en el panel de output de PlatformIO |
| Draw.io Integration | `hediet.vscode-drawio` | Diagramas de conexión hardware y flujos de estado |
| C/C++ Extension Pack | `ms-vscode.cpptools-extension-pack` | IntelliSense, navegación de código, autocompletado |
| Todo Tree | `gruntfuggly.todo-tree` | Tracking de `TODO`, `FIXME`, `HACK`, `BUG`, `NOTE`, `WARN` |
| GitLens | `eamodio.gitlens` | Historial de cambios, blame, comparación de ramas |
| YAML | `redhat.vscode-yaml` | Validación y autocompletado para `platformio.ini` y CI/CD |
| Markdown All in One | `yzhang.markdown-all-in-one` | Edición mejorada de documentación `.md` |

---

## 🛠️ Configuración Global VS Code

### `.vscode/settings.json` (workspace)

```json
{
    "todo-tree.general.tags": ["TODO","FIXME","HACK","BUG","XXX","NOTE","WARN","REVIEW"],
    "todo-tree.highlights.defaultHighlight": {
        "foreground": "#ffffff",
        "background": "#ff6b6b",
        "icon": "alert",
        "iconColour": "#ff6b6b"
    },
    "todo-tree.highlights.customHighlight": {
        "TODO":  { "icon": "check", "background": "#4ecdc4", "iconColour": "#4ecdc4" },
        "FIXME": { "icon": "tools", "background": "#ffe66d", "iconColour": "#ffe66d", "foreground": "#000000" },
        "HACK":  { "icon": "zap",   "background": "#ff6b6b", "iconColour": "#ff6b6b" },
        "BUG":   { "icon": "bug",   "background": "#c0392b", "iconColour": "#c0392b" },
        "NOTE":  { "icon": "note",  "background": "#3498db", "iconColour": "#3498db" },
        "WARN":  { "icon": "alert", "background": "#f39c12", "iconColour": "#f39c12" },
        "REVIEW":{ "icon": "eye",   "background": "#9b59b6", "iconColour": "#9b59b6" }
    },
    "todo-tree.filtering.includeGlobs": [
        "**/*.cpp","**/*.h","**/*.hpp","**/*.c","**/*.ino","**/*.py","**/*.md","**/*.yaml","**/*.yml"
    ],
    "todo-tree.tree.showCountsInTree": true,
    "todo-tree.general.statusBar": "total",

    "C_Cpp.intelliSenseCacheSize": 512,
    "C_Cpp.errorSquiggles": "Enabled",
    "C_Cpp.autocomplete": "Default",
    "C_Cpp.intelliSenseEngine": "Default",
    "C_Cpp.formatting": "vcFormat",
    "C_Cpp.vcFormat.indent.multiLineRelativeTo": "innermostParenthesis",

    "vscode-serial-monitor.default.baudRate": 115200,
    "vscode-serial-monitor.default.lineEnding": "LF",
    "vscode-serial-monitor.default.timestamp": true,
    "vscode-serial-monitor.default.hexFormat": false,

    "editor.formatOnSave": true,
    "editor.codeActionsOnSave": {
        "source.organizeImports": true
    },
    "files.associations": {
        "*.ino": "cpp",
        "platformio.ini": "ini"
    },
    "platformio-ide.useBuiltinPython": true,
    "platformio-ide.autoClearConsole": false,
    "platformio-ide.autoSaveOnBuild": true
}
```

### `.vscode/extensions.json` (recomendaciones)

```json
{
    "recommendations": [
        "platformio.platformio-ide",
        "ms-vscode.vscode-serial-monitor",
        "jeff-hykin.better-cpp-syntax",
        "IBM.output-colorizer",
        "hediet.vscode-drawio",
        "ms-vscode.cpptools-extension-pack",
        "gruntfuggly.todo-tree",
        "eamodio.gitlens",
        "redhat.vscode-yaml",
        "yzhang.markdown-all-in-one"
    ],
    "unwantedRecommendations": []
}
```

### `.vscode/tasks.json` (Task Runner — NUEVO)

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "PlatformIO: Build",
            "type": "shell",
            "command": "pio run",
            "group": { "kind": "build", "isDefault": true },
            "problemMatcher": ["$platformio"]
        },
        {
            "label": "PlatformIO: Upload",
            "type": "shell",
            "command": "pio run --target upload",
            "problemMatcher": ["$platformio"]
        },
        {
            "label": "PlatformIO: Monitor",
            "type": "shell",
            "command": "pio device monitor",
            "problemMatcher": []
        },
        {
            "label": "PlatformIO: Clean",
            "type": "shell",
            "command": "pio run --target clean",
            "problemMatcher": []
        },
        {
            "label": "PlatformIO: Full Deploy",
            "type": "shell",
            "command": "pio run --target upload --target monitor",
            "group": { "kind": "build" },
            "problemMatcher": ["$platformio"]
        },
        {
            "label": "PlatformIO: Test",
            "type": "shell",
            "command": "pio test",
            "problemMatcher": ["$platformio"]
        },
        {
            "label": "PlatformIO: Check",
            "type": "shell",
            "command": "pio check",
            "problemMatcher": []
        }
    ]
}
```

### `.vscode/platformio-snippets.code-snippets` (Snippets VS Code — NUEVO)

```json
{
    "PlatformIO env section": {
        "prefix": "pioenv",
        "body": [
            "[env:${1:myenv}]",
            "platform = ${2:espressif32}",
            "board = ${3:esp32dev}",
            "framework = ${4:arduino}",
            "monitor_speed = ${5:115200}",
            "build_flags =",
            "    -D ${6:DEBUG}=1"
        ],
        "description": "PlatformIO environment section"
    },
    "PlatformIO build flags": {
        "prefix": "piobf",
        "body": [
            "build_flags =",
            "    -D ${1:FLAG_NAME}=1",
            "    -D ${2:ANOTHER_FLAG}=0"
        ],
        "description": "PlatformIO build flags block"
    },
    "PlatformIO lib deps": {
        "prefix": "piolib",
        "body": [
            "lib_deps =",
            "    ${1:author/library} @ ^${2:1.0.0}"
        ],
        "description": "PlatformIO library dependencies"
    },
    "PlatformIO debug env": {
        "prefix": "piodbg",
        "body": [
            "[env:${1:debug}]",
            "extends = env:${2:base}",
            "build_type = debug",
            "build_flags =",
            "    -D DEBUG=1",
            "    -D CORE_DEBUG_LEVEL=5",
            "monitor_filters = esp32_exception_decoder"
        ],
        "description": "PlatformIO debug environment"
    }
}
```

---

## 📝 Convención de Tags (Todo Tree)

```cpp
#include <Arduino.h>

// TODO: Tarea pendiente de implementar
// FIXME: Algo que funciona mal y necesita corrección urgente
// HACK: Solución temporal — refactorizar antes de producción
// BUG: Error conocido, aún sin solución
// NOTE: Información importante sobre el comportamiento del código
// WARN: Advertencia crítica de hardware, seguridad o lógica
// REVIEW: Código que necesita revisión antes de mergear

void setup() { 
    Serial.begin(115200); 
}

void loop() { 
    // TODO: Implementar lógica principal
}
```

---

## 📁 Estructura de Proyectos PlatformIO

### Proyecto Simple (una plataforma)

```
mi-proyecto/
├── src/
│   └── main.cpp
├── include/
│   ├── config.h
│   └── utils.h
├── lib/
│   └── (librerías locales)
├── docs/
│   ├── diagrama.drawio
│   └── notas.md
├── platformio.ini
├── README.md
└── .vscode/
    ├── settings.json
    ├── extensions.json
    ├── tasks.json
    └── platformio-snippets.code-snippets
```

### Proyecto Multi-plataforma

```
workspace/
├── proyecto-esp8266/
│   ├── src/main.cpp
│   ├── platformio.ini      ; env:d1_mini_pro
│   └── .vscode/
├── proyecto-arduino-uno/
│   ├── src/main.cpp
│   ├── platformio.ini      ; env:uno
│   └── .vscode/
├── proyecto-esp32/
│   ├── src/main.cpp
│   ├── platformio.ini      ; env:esp32dev
│   └── .vscode/
└── README.md               ; Documentación general del workspace
```

### Proyecto con Múltiples Entornos (mismo hardware)

```ini
; platformio.ini
[env]
platform = espressif8266
framework = arduino
monitor_speed = 115200
lib_deps =
    knolleary/PubSubClient @ ^2.8

[env:d1_mini_pro_debug]
board = d1_mini_pro
build_type = debug
monitor_filters = esp8266_exception_decoder
build_flags = -D DEBUG=1

[env:d1_mini_pro_release]
board = d1_mini_pro
build_type = release
build_flags = -D DEBUG=0
board_build.flash_mode = dout
```

---

## 🔧 Pre-commit Hooks (NUEVO)

### `.pre-commit-config.yaml`

```yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.5.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-json

  - repo: https://github.com/pre-commit/mirrors-clang-format
    rev: v17.0.6
    hooks:
      - id: clang-format
        types: [c, c++]
        args: [--style=file]

  - repo: local
    hooks:
      - id: platformio-check
        name: PlatformIO check
        entry: pio check
        language: system
        pass_filenames: false
        always_run: true
```

### `.clang-format` (en raíz del proyecto)

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: Empty
BreakBeforeBraces: Attach
SortIncludes: true
```

---

## 🐳 Docker para CI Local (NUEVO)

### `Dockerfile`

```dockerfile
FROM python:3.11-slim

# Instalar dependencias del sistema
RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Instalar PlatformIO
RUN pip install --no-cache-dir platformio

WORKDIR /workspace

# Copiar y resolver dependencias primero (cache layer)
COPY platformio.ini .
RUN pio pkg install

# Copiar el resto del código
COPY . .

# Build por defecto
CMD ["pio", "run"]
```

### Uso:

```bash
# Build
 docker build -t pio-build . && docker run --rm pio-build

# Con bind mount para desarrollo
 docker run --rm -v $(pwd):/workspace pio-build

# Upload (requiere acceso a puerto USB — Linux)
 docker run --rm --device=/dev/ttyUSB0 -v $(pwd):/workspace pio-build pio run --target upload
```

---

## 🔧 Snippets PlatformIO Útiles

### Comandos frecuentes

```bash
# Build
pio run

# Build y upload
pio run --target upload

# Build, upload y monitor
pio run --target upload --target monitor

# Solo monitor
pio device monitor

# Listar puertos
pio device list

# Limpiar build
pio run --target clean

# Instalar librería
pio lib install "nombre@version"

# Actualizar librerías
pio lib update

# Ver información de board
pio boards --filter esp8266

# Check estático
pio check

# Test unitarios
pio test
```

---

## 📄 Templates de `platformio.ini`

### Template ESP8266

```ini
[env:esp8266_base]
platform = espressif8266
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0

build_flags =
    -D PIO_FRAMEWORK_ARDUINO
    -D ARDUINO_ARCH_ESP8266

; Descomentar para debug de excepciones
; monitor_filters = esp8266_exception_decoder

; Optimización de tamaño
; build_flags = -Os
```

### Template ESP32

```ini
[env:esp32_base]
platform = espressif32
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0

build_flags =
    -D CORE_DEBUG_LEVEL=3
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

; Monitor con decodificador de backtrace
monitor_filters = esp32_exception_decoder
```

### Template ESP32-S3 (NUEVO)

```ini
[env:esp32s3_base]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D CORE_DEBUG_LEVEL=3
    -D CONFIG_SPIRAM_USE=1

; PSRAM 8MB habilitada
board_build.arduino.memory_type = qio_opi

; Monitor con decodificador
monitor_filters = esp32_exception_decoder
```

### Template ESP32-C3 (NUEVO)

```ini
[env:esp32c3_base]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D CORE_DEBUG_LEVEL=3

; RISC-V single-core, WiFi + BLE 5.0
; Consumo más bajo que ESP32 clásico
monitor_filters = esp32_exception_decoder
```

### Template AVR (Arduino Uno/Nano)

```ini
[env:avr_base]
platform = atmelavr
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0

; Optimización para memoria limitada
build_flags =
    -D F_CPU=16000000L
    -Os

; Librerías comunes
; lib_deps =
;     Wire
;     SPI
```

### Template STM32

```ini
[env:stm32_base]
platform = ststm32
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

; Board específica (ej: Blue Pill)
; board = bluepill_f103c8

build_flags =
    -D DEBUG=1
    -D SERIAL_USB
```

### Template STM32 Black Pill F401/F411 (NUEVO)

```ini
[env:blackpill_f401cc]
platform = ststm32
board = blackpill_f401cc
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

; Cortex-M4 @ 84 MHz, 96 KB RAM, 256 KB Flash
; USB nativo (CDC), más RAM que Blue Pill
build_flags =
    -D USBCON
    -D USBD_USE_CDC
    -D HAL_PCD_MODULE_ENABLED

; Para F411:
; board = blackpill_f411ce
; 128 KB RAM, 512 KB Flash, Cortex-M4 @ 100 MHz
```

### Template RP2040 (Raspberry Pi Pico)

```ini
[env:rp2040_base]
platform = raspberrypi
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

; board = pico
; board = adafruit_feather_rp2040

build_flags =
    -D CFG_DEBUG=2
    -D CFG_TUSB_DEBUG=0
```

---

## 🎯 Workflow Recomendado

1. **Inicializar proyecto:**
   ```bash
   pio init --board d1_mini_pro --ide vscode
   ```

2. **Configurar VS Code:**
   - Copiar `.vscode/settings.json`, `extensions.json`, `tasks.json`, `platformio-snippets.code-snippets`
   - Instalar extensiones recomendadas
   - Ajustar `monitor_port` según sistema operativo
   - Configurar pre-commit hooks: `pre-commit install`

3. **Desarrollo:**
   - Usar tags `TODO`/`FIXME` para tracking
   - Documentar en `docs/` con Draw.io
   - Commit frecuente con Git
   - Ejecutar `pre-commit run --all-files` antes de push

4. **Debug:**
   - Usar `Serial.println()` con niveles de log
   - Activar `monitor_filters` para decodificar crashes
   - Usar `platformio.ini` con entornos debug/release
   - Ejecutar `pio check` para análisis estático

5. **Deploy:**
   - Build release con optimización `-Os`
   - Verificar partition table si es necesario
   - Upload y monitor en un solo comando (`pio run -t upload -t monitor`)
   - Opcional: build en Docker para CI

---

# PLANTILLA 2: CATÁLOGO DE HARDWARE

## 📚 Índice de Placas

- [ESP8266](#esp8266)
  - [Wemos D1 Mini Pro](#wemos-d1-mini-pro)
  - [NodeMCU v3](#nodemcu-v3)
  - [ESP-01S](#esp-01s)

- [ESP32](#esp32)
  - [ESP32 DevKit v1](#esp32-devkit-v1)
  - [ESP32-S3 DevKit](#esp32-s3-devkit)
  - [ESP32-C3 DevKit](#esp32-c3-devkit)

- [AVR](#avr)
  - [Arduino Uno](#arduino-uno)
  - [Arduino Nano](#arduino-nano)
  - [Arduino Mega 2560](#arduino-mega-2560)

- [ARM](#arm)
  - [STM32 Blue Pill](#stm32-blue-pill)
  - [STM32 Black Pill](#stm32-black-pill)
  - [Arduino Due](#arduino-due)

- [RP2040](#rp2040)
  - [Raspberry Pi Pico](#raspberry-pi-pico)

---

## ESP8266

### Wemos D1 Mini Pro

#### 🧩 Hardware Principal

- **Placa:** Wemos D1 Mini Pro
- **MCU:** ESP-8266EX (Tensilica L106, 32-bit RISC)
- **Clock:** 80 MHz / 160 MHz (configurable)
- **RAM:** ~80 KB user RAM
- **Flash:** 16 MB (128 Mbit)
- **Alimentación:** 5V USB → 3.3V regulado interno (LDO)
- **Dimensiones:** 34.2 mm × 25.6 mm
- **Peso:** 2.5 g

#### 📡 Conectividad

- **WiFi:** 802.11 b/g/n (2.4 GHz)
- **Antena:** Cerámica integrada + conector U.FL para antena externa
- **USB-UART:** CP2104

#### 🔌 Periféricos / Módulos Conectados

| Módulo | Protocolo | Nivel Lógico | Pines Usados | Notas |
| --- | --- | --- | --- | --- |
| Módulo de nivel lógico bidireccional | — | 3.3V ↔ 5V | — | 4 canales, basado en MOSFETs |
| *(agregar)* |  |  |  |  |

#### ⚡ Niveles de Voltaje

- **D1 Mini Pro:** 3.3V lógico (GPIO) / 5V solo en VCC (alimentación)
- **GPIO:** **NO tolerantes a 5V** — máximo 3.3V (3.6V absoluto, operar siempre a 3.3V)
- **ADC (A0):** Máximo 3.2V (1 canal, 10-bit, 0–3.2V)
- **Conversión:** Usar módulo de nivel lógico para cualquier interfaz con dispositivos 5V

#### 🗺️ Mapeo de Pines

| Función | GPIO | D# | Uso / Notas |
| --- | --- | --- | --- |
| UART0 TX | GPIO1 | D10 | Debug/Monitor Serial (USB) |
| UART0 RX | GPIO3 | D9 | Debug/Monitor Serial (USB) |
| SPI SCK | GPIO14 | D5 | HSCLK |
| SPI MISO | GPIO12 | D6 | HMISO |
| SPI MOSI | GPIO13 | D7 | HMOSI |
| SPI CS | GPIO15 | D8 | HCS (boot: pull-down required) |
| I2C SDA | GPIO4 | D2 |  |
| I2C SCL | GPIO5 | D1 |  |
| PWM / IO | GPIO0 | D3 | Boot: pull-up required |
| PWM / IO | GPIO2 | D4 | Built-in LED (active LOW), boot: pull-up required |
| PWM / IO | GPIO16 | D0 | Deep sleep wake, no PWM |
| ADC | A0 | A0 | 0–3.2V, 10-bit |

#### ⚠️ Consideraciones Críticas

- **NUNCA** conectar 5V a GPIO o A0 — daño irreversible al ESP8266EX
- **GPIO 0, 2, 15:** Niveles en boot determinan modo de arranque (flash vs. programación)
- **GPIO 16 (D0):** Sin PWM, único pin para wake desde deep sleep
- **GPIO 2 (D4):** LED integrado (LOW = encendido), activo en boot
- Usar módulo de nivel lógico para UART/I2C/SPI con dispositivos 5V
- Monitor serial requiere puerto libre (cerrar `pio device monitor` si está ocupado)
- **Flash 16 MB:** Verificar partition table en PlatformIO (por defecto puede usar solo 4 MB)
- **Pines SPI (D5-D8):** Evitar para GPIO genérico si se usa SPI simultáneamente
- **D3 y D4:** Evitar como salidas en boot — pueden causar comportamiento inesperado

#### 📡 `platformio.ini`

```ini
[env:d1_mini_pro]
platform = espressif8266
board = d1_mini_pro
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0       ; Linux/Mac
; monitor_port = COM3             ; Windows

; Descomentar para decodificar crashes
; monitor_filters = esp8266_exception_decoder

build_flags =
    -D PIO_FRAMEWORK_ARDUINO
    -D ARDUINO_ARCH_ESP8266

; Flash completa — si la board detecta solo 4MB, forzar con:
; board_build.flash_size = 16MB
; board_build.ldscript = eagle.flash.16m.ld
```

---

### NodeMCU v3

#### 🧩 Hardware Principal

- **Placa:** NodeMCU v3 (LOLIN)
- **MCU:** ESP-8266EX
- **Clock:** 80 MHz
- **RAM:** ~80 KB
- **Flash:** 4 MB
- **Alimentación:** 5V USB (Micro-USB)
- **Dimensiones:** 54 mm × 31 mm

#### 🗺️ Mapeo de Pines

| Función | GPIO | Pin Label |
| --- | --- | --- |
| UART0 TX | GPIO1 | D10/TX |
| UART0 RX | GPIO3 | D9/RX |
| I2C SDA | GPIO4 | D2/SDA |
| I2C SCL | GPIO5 | D1/SCL |
| SPI CS | GPIO15 | D8/CS |
| Built-in LED | GPIO16 | D0/LED |

#### ⚠️ Consideraciones

- GPIOs **NO tolerantes a 5V**
- USB Micro-B (menos robusto que USB-C)
- Flash 4 MB (suficiente para la mayoría de proyectos)

---

### ESP-01S

#### 🧩 Hardware Principal

- **Módulo:** ESP-01S
- **MCU:** ESP-8266EX
- **Flash:** 1 MB
- **GPIOs disponibles:** 2 (GPIO0, GPIO2)
- **Alimentación:** 3.3V (requiere regulador externo si se usa 5V)

#### ⚠️ Consideraciones Críticas

- **Solo 2 GPIOs** útiles (GPIO0 y GPIO2)
- **Requiere adaptador USB-UART** para programación
- **Necesita 500mA** en picos de WiFi — regulador robusto
- **Muy limitado** — solo para proyectos simples o como slave

---

## ESP32

### ESP32 DevKit v1

#### 🧩 Hardware Principal

- **Placa:** ESP32 DevKit v1
- **MCU:** ESP32-D0WDQ6 (dual-core Xtensa LX6, 32-bit)
- **Clock:** 240 MHz
- **RAM:** 520 KB SRAM
- **Flash:** 4 MB
- **Alimentación:** 5V USB → 3.3V regulado
- **WiFi + Bluetooth:** 802.11 b/g/n + BLE 4.2

#### 🗺️ Mapeo de Pines (parcial)

| Función | GPIO | Notas |
| --- | --- | --- |
| UART0 TX | GPIO1 | Debug |
| UART0 RX | GPIO3 | Debug |
| I2C SDA | GPIO21 |  |
| I2C SCL | GPIO22 |  |
| SPI SCK | GPIO18 | VSPI |
| Built-in LED | GPIO2 |  |

#### ⚡ Niveles de Voltaje

- **GPIO:** 3.3V lógico (tolerantes a 5V en algunos pines — verificar datasheet)
- **ADC:** 12-bit, 0–3.3V (18 canales)
- **DAC:** 2 canales (GPIO25, GPIO26)

#### ⚠️ Consideraciones Críticas

- **GPIO 6-11:** Reservados para flash interna — **NO USAR**
- **GPIO 34-39:** Solo entrada (sin pull-up/down)
- **GPIO 36-39:** ADC1 (mejor para WiFi simultáneo)
- **GPIO 25-26:** DAC (solo salida)
- **Strapping pins (0, 2, 12, 15):** Configuran modo de boot
- **Consumo:** Mayor que ESP8266 — considerar deep sleep

---

### ESP32-S3 DevKit (NUEVO)

#### 🧩 Hardware Principal

- **Placa:** ESP32-S3-DevKitC-1
- **MCU:** ESP32-S3 (dual-core Xtensa LX7, 32-bit)
- **Clock:** 240 MHz
- **RAM:** 512 KB SRAM + 8 MB PSRAM (QSPI/OPI)
- **Flash:** 8 MB
- **Alimentación:** 5V USB-C → 3.3V regulado
- **WiFi + Bluetooth:** 802.11 b/g/n + BLE 5.0
- **Vectorial:** SIMD/vector instructions (AI/ML edge)

#### 🗺️ Mapeo de Pines (parcial)

| Función | GPIO | Notas |
| --- | --- | --- |
| UART0 TX | GPIO43 | Debug (USB-JTAG) |
| UART0 RX | GPIO44 | Debug (USB-JTAG) |
| I2C SDA | GPIO8 |  |
| I2C SCL | GPIO9 |  |
| SPI SCK | GPIO12 |  |
| Built-in LED | GPIO2 | RGB LED en algunas versiones |

#### ⚡ Niveles de Voltaje

- **GPIO:** 3.3V lógico
- **ADC:** 12-bit, 0–3.3V (20 canales)
- **USB:** USB-OTG nativo (USB Serial/JTAG)

#### ⚠️ Consideraciones Críticas

- **USB Serial/JTAG integrado:** No requiere UART externo para debug
- **PSRAM 8MB:** Ideal para buffers grandes, cámara, o modelos TinyML
- **GPIO 26-32:** Reservados para flash/PSRAM en modo OPI — **NO USAR**
- **Consumo:** ~150 mA activo, deep sleep ~7 µA
- **AI/Vectorial:** Instrucciones SIMD aceleran operaciones de señal

#### 📡 `platformio.ini`

```ini
[env:esp32s3_devkit]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D CORE_DEBUG_LEVEL=3
    -D CONFIG_SPIRAM_USE=1

; PSRAM habilitada
board_build.arduino.memory_type = qio_opi

monitor_filters = esp32_exception_decoder
```

---

### ESP32-C3 DevKit (NUEVO)

#### 🧩 Hardware Principal

- **Placa:** ESP32-C3-DevKitM-1
- **MCU:** ESP32-C3 (single-core RISC-V, 32-bit)
- **Clock:** 160 MHz
- **RAM:** 400 KB SRAM
- **Flash:** 4 MB
- **Alimentación:** 5V USB-C → 3.3V regulado
- **WiFi + Bluetooth:** 802.11 b/g/n + BLE 5.0

#### 🗺️ Mapeo de Pines (parcial)

| Función | GPIO | Notas |
| --- | --- | --- |
| UART0 TX | GPIO21 | Debug (USB) |
| UART0 RX | GPIO20 | Debug (USB) |
| I2C SDA | GPIO8 |  |
| I2C SCL | GPIO9 |  |
| SPI SCK | GPIO4 |  |
| Built-in LED | GPIO8 | Comparte con I2C SDA en algunas boards |

#### ⚡ Niveles de Voltaje

- **GPIO:** 3.3V lógico
- **ADC:** 12-bit, 0–3.3V (5 canales)

#### ⚠️ Consideraciones Críticas

- **RISC-V single-core:** Menor throughput que ESP32 dual-core, pero más eficiente energéticamente
- **USB nativo:** CDC integrado, no requiere UART bridge
- **GPIO limitados:** 22 GPIOs vs 34 del ESP32 clásico
- **Consumo:** ~90 mA activo, deep sleep ~5 µA
- **Precio:** Significativamente más barato que ESP32-S3

#### 📡 `platformio.ini`

```ini
[env:esp32c3_devkit]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D CORE_DEBUG_LEVEL=3

monitor_filters = esp32_exception_decoder
```

---

## AVR

### Arduino Uno

#### 🧩 Hardware Principal

- **Placa:** Arduino Uno
- **MCU:** ATmega328P (AVR, 8-bit)
- **Clock:** 16 MHz
- **RAM:** 2 KB SRAM
- **Flash:** 32 KB (0.5 KB bootloader)
- **EEPROM:** 1 KB
- **Alimentación:** 5V USB / 7–12V barrel jack
- **Lógica:** 5V (GPIO tolerantes a 5V)

#### 🔌 Periféricos / Módulos Conectados

| Módulo | Protocolo | Nivel Lógico | Pines Usados | Notas |
| --- | --- | --- | --- | --- |
| Receptor RF 433MHz | Digital/INT | 5V | D2 (INT0) | RCSwitch, códigos 24-bit |
| *(agregar)* |  |  |  |  |

#### ⚡ Niveles de Voltaje

- **GPIO:** 5V lógico — tolerantes a 5V
- **ADC:** 10-bit, 0–5V (6 canales: A0–A5)
- **Corriente por pin:** máximo 40 mA, total 200 mA
- **Interfaz con 3.3V:** usar módulo de nivel lógico bidireccional

#### 🗺️ Mapeo de Pines

| Función | Pin | Uso / Notas |
| --- | --- | --- |
| UART TX | 1 | Monitor Serial (USB) |
| UART RX | 0 | Monitor Serial (USB) — no usar mientras programa |
| INT0 | 2 | Interrupción externa 0 — RF 433MHz (RCSwitch) |
| INT1 | 3 | Interrupción externa 1 / PWM |
| PWM | 3,5,6,9,10,11 | 8-bit PWM |
| SPI SCK | 13 | También LED integrado (active HIGH) |
| SPI MISO | 12 |  |
| SPI MOSI | 11 |  |
| SPI CS | 10 |  |
| I2C SDA | A4 |  |
| I2C SCL | A5 |  |
| ADC | A0–A5 | 0–5V, 10-bit |

#### ⚠️ Consideraciones Críticas

- **RAM limitada (2KB):** Usar macro `F()` para strings en Flash — libera SRAM
- **Pin 0 (RX):** No conectar nada mientras se programa via USB
- **Pin 13:** Tiene LED integrado y resistencia — no usar como entrada de señal
- **INT0 (D2) e INT1 (D3):** Únicos pines con interrupción externa en Uno
- **Corriente total:** No superar 200 mA en todos los GPIO juntos
- **Sin WiFi:** Para conectividad agregar módulo ESP8266 (AT commands) o NRF24L01
- **`F()` macro:** Crítica en proyectos con muchos strings

#### 📡 `platformio.ini`

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0

; Librerías del proyecto
; lib_deps =
;     sui77/rc-switch@^2.6.4     ; RF 433MHz
```

---

### Arduino Nano

#### 🧩 Hardware Principal

- **Placa:** Arduino Nano (ATmega328P)
- **MCU:** ATmega328P
- **Clock:** 16 MHz
- **RAM:** 2 KB
- **Flash:** 32 KB
- **Alimentación:** 5V USB (Mini-B) / 7–12V VIN
- **Dimensiones:** 45 mm × 18 mm (más compacta que Uno)

#### ⚠️ Consideraciones

- Mismo MCU que Uno — misma programación
- **USB Mini-B** (obsoleto, considerar Nano V3.3 con USB-C)
- **Sin barrel jack** — alimentar por VIN o USB
- Ideal para proyectos compactos

---

### Arduino Mega 2560

#### 🧩 Hardware Principal

- **Placa:** Arduino Mega 2560
- **MCU:** ATmega2560
- **Clock:** 16 MHz
- **RAM:** 8 KB SRAM
- **Flash:** 256 KB
- **GPIOs:** 54 digitales (15 PWM) + 16 analógicos

#### ⚠️ Consideraciones

- **Más memoria** que Uno/Nano — ideal para proyectos grandes
- **Más GPIOs** — 54 pines digitales
- **4 UARTs hardware** — Serial, Serial1, Serial2, Serial3
- **Mayor consumo** — considerar alimentación externa

---

## ARM

### STM32 Blue Pill

#### 🧩 Hardware Principal

- **Placa:** STM32F103C8T6 "Blue Pill"
- **MCU:** ARM Cortex-M3 (32-bit)
- **Clock:** 72 MHz
- **RAM:** 20 KB
- **Flash:** 64 KB
- **Alimentación:** 3.3V / 5V USB
- **ADC:** 12-bit, 16 canales

#### ⚠️ Consideraciones

- **Programación:** Requiere ST-Link o bootloader UART
- **3.3V lógico** — NO tolerante a 5V
- **Más potente** que AVR — ideal para DSP o control avanzado
- **Precio bajo** — excelente relación calidad/precio

---

### STM32 Black Pill (NUEVO)

#### 🧩 Hardware Principal

- **Placa:** STM32F401CC/F411CE "Black Pill"
- **MCU:** ARM Cortex-M4 (32-bit, FPU)
- **Clock:** 84 MHz (F401) / 100 MHz (F411)
- **RAM:** 96 KB (F401) / 128 KB (F411)
- **Flash:** 256 KB (F401) / 512 KB (F411)
- **Alimentación:** 3.3V / 5V USB-C
- **ADC:** 12-bit, 16 canales
- **USB:** USB nativo (CDC) — no requiere UART bridge

#### 🗺️ Mapeo de Pines (parcial)

| Función | Pin | Notas |
| --- | --- | --- |
| UART1 TX | PA9 |  |
| UART1 RX | PA10 |  |
| I2C1 SDA | PB7 |  |
| I2C1 SCL | PB6 |  |
| SPI1 SCK | PA5 |  |
| USB D+ | PA12 | USB nativo |
| USB D- | PA11 | USB nativo |
| Built-in LED | PC13 | Active LOW |

#### ⚡ Niveles de Voltaje

- **GPIO:** 3.3V lógico — **NO tolerante a 5V**
- **ADC:** 12-bit, 0–3.3V
- **USB:** 5V en VBUS, 3.3V en lógica

#### ⚠️ Consideraciones Críticas

- **USB nativo:** CDC funciona sin UART bridge, pero requiere resistencia de 1.5k en D+ (ya onboard)
- **FPU:** Cortex-M4 tiene unidad de punto flotante — ideal para algoritmos DSP
- **Más RAM/Flash** que Blue Pill — proyectos complejos sin problemas
- **Bootloader:** Puede cargar STM32duino bootloader para programación USB
- **Consumo:** ~40 mA activo, deep sleep ~1 µA

#### 📡 `platformio.ini`

```ini
[env:blackpill_f401cc]
platform = ststm32
board = blackpill_f401cc
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

build_flags =
    -D USBCON
    -D USBD_USE_CDC
    -D HAL_PCD_MODULE_ENABLED

; Para F411:
; board = blackpill_f411ce
; 128 KB RAM, 512 KB Flash, 100 MHz
```

---

## RP2040

### Raspberry Pi Pico

#### 🧩 Hardware Principal

- **Placa:** Raspberry Pi Pico
- **MCU:** RP2040 (dual-core ARM Cortex-M0+, 32-bit)
- **Clock:** 133 MHz
- **RAM:** 264 KB SRAM
- **Flash:** 2 MB (externa)
- **Alimentación:** 5V USB-C
- **GPIOs:** 26 (20 GPIO + 6 ADC)

#### ⚡ Niveles de Voltaje

- **GPIO:** 3.3V lógico (tolerantes a 5V)
- **ADC:** 12-bit, 0–3.3V (3 canales externos + 1 interno)

#### ⚠️ Consideraciones

- **USB-C** — conexión moderna y robusta
- **PIO (Programmable I/O):** 8 state machines para protocolos personalizados
- **Dual-core:** Ejecución paralela de tareas
- **Documentación excelente** — Raspberry Pi Foundation

---

## 📊 Tabla Comparativa Rápida (MEJORADA — con power budget)

| Placa | MCU | Clock | RAM | Flash | Vcc | Consumo Activo | Deep Sleep | WiFi/BT | Precio |
|-------|-----|-------|-----|-------|-----|----------------|------------|---------|--------|
| D1 Mini Pro | ESP8266 | 160 MHz | 80 KB | 16 MB | 3.3V | ~80 mA | ~20 µA | WiFi | $ |
| NodeMCU v3 | ESP8266 | 80 MHz | 80 KB | 4 MB | 3.3V | ~80 mA | ~20 µA | WiFi | $ |
| ESP32 DevKit | ESP32 | 240 MHz | 520 KB | 4 MB | 3.3V | ~120 mA | ~5 µA | WiFi+BT | $$ |
| ESP32-S3 | ESP32-S3 | 240 MHz | 512 KB+8MB | 8 MB | 3.3V | ~150 mA | ~7 µA | WiFi+BT5 | $$$ |
| ESP32-C3 | RISC-V | 160 MHz | 400 KB | 4 MB | 3.3V | ~90 mA | ~5 µA | WiFi+BT5 | $ |
| Arduino Uno | ATmega328P | 16 MHz | 2 KB | 32 KB | 5V | ~45 mA | N/A | — | $ |
| Arduino Nano | ATmega328P | 16 MHz | 2 KB | 32 KB | 5V | ~45 mA | N/A | — | $ |
| Arduino Mega | ATmega2560 | 16 MHz | 8 KB | 256 KB | 5V | ~50 mA | N/A | — | $$ |
| Blue Pill | STM32F103 | 72 MHz | 20 KB | 64 KB | 3.3V | ~35 mA | ~1 µA | — | $ |
| Black Pill | STM32F401 | 84 MHz | 96 KB | 256 KB | 3.3V | ~40 mA | ~1 µA | — | $$ |
| Pi Pico | RP2040 | 133 MHz | 264 KB | 2 MB | 3.3V | ~90 mA | ~1.9 µA | — | $ |

---

## 🔧 Strapping Pins: ESP32 / ESP32-S3 / ESP32-C3 (NUEVO)

| Pin | Función boot | LOW | HIGH | Notas |
|-----|-------------|-----|------|-------|
| GPIO0 | Boot mode | Flash boot | Download boot | Pull-up interno. Evitar carga capacitiva alta. |
| GPIO2 | Boot mode (ESP32) | — | Must be LOW for download | Conectado a LED en muchas boards |
| GPIO5 | VDD_SDIO voltage | 1.8V | 3.3V (flash) | Afecta flash externa. Dejar flotar = 3.3V |
| GPIO12 | VDD_SDIO voltage | 1.8V | 3.3V | ESP32 only. MTDI strap. |
| GPIO15 | Boot log silencing | UART0 log output | Silent boot | Pull-down interno. Evitar pull-up fuerte. |

> **Regla de oro:** Nunca conectar cargas que puedan mantener strapping pins en estado incorrecto durante el reset. Usar buffers tri-state o switches para aislar periféricos de estos pines si es necesario.

---

## 🔧 Partition Table: ESP32 con 16MB Flash (NUEVO)

### `partitions_16MB.csv`

```csv
# Name,   Type, SubType, Offset,  Size,    Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x200000,
app0,     app,  ota_0,   0x210000,0x200000,
app1,     app,  ota_1,   0x410000,0x200000,
spiffs,   data, spiffs,  0x610000,0x9F0000,
```

### `platformio.ini`

```ini
board_build.partitions = partitions_16MB.csv
board_build.flash_size = 16MB
```

---

## 🔧 Criterios de Selección

### ¿Cuándo usar cada plataforma?

**ESP8266 (D1 Mini, NodeMCU):**
- ✅ Necesitas WiFi barato
- ✅ Proyecto simple con IoT básico
- ✅ Consumo moderado aceptable
- ❌ No usar si necesitas Bluetooth o mucho procesamiento

**ESP32:**
- ✅ WiFi + Bluetooth necesarios
- ✅ Procesamiento dual-core requerido
- ✅ Más RAM/Flash necesarios
- ✅ ADC de mayor resolución (12-bit)
- ❌ Consumo más alto que ESP8266

**ESP32-S3:**
- ✅ AI/ML edge (instrucciones vectoriales)
- ✅ PSRAM grande (8MB) para buffers/cámara
- ✅ BLE 5.0 con mayor throughput
- ✅ USB nativo (JTAG/CDC)
- ❌ Consumo más alto, precio superior

**ESP32-C3:**
- ✅ WiFi+BLE 5.0 a bajo costo
- ✅ RISC-V (ecosistema abierto)
- ✅ Consumo eficiente
- ❌ Single-core, menos GPIOs

**AVR (Uno, Nano, Mega):**
- ✅ Proyectos educativos o simples
- ✅ Necesitas 5V lógico (sensores antiguos)
- ✅ Baja velocidad aceptable
- ✅ Compatibilidad con shields
- ❌ Sin conectividad inalámbrica nativa
- ❌ RAM muy limitada

**STM32 Blue Pill:**
- ✅ Procesamiento ARM 32-bit necesario
- ✅ ADC/DAC de alta resolución
- ✅ Control en tiempo real crítico
- ✅ Precio mínimo
- ❌ Curva de aprendizaje más pronunciada
- ❌ Programación requiere ST-Link

**STM32 Black Pill:**
- ✅ FPU (punto flotante) para DSP
- ✅ USB nativo sin UART bridge
- ✅ Más RAM/Flash que Blue Pill
- ✅ USB-C moderno
- ❌ Ligeramente más caro que Blue Pill

**RP2040:**
- ✅ PIO para protocolos personalizados
- ✅ Dual-core a bajo costo
- ✅ USB-C nativo
- ✅ Excelente documentación
- ❌ Sin WiFi/Bluetooth nativo (agregar módulo)

---

# PLANTILLA 3: MÓDULOS PERIFÉRICOS (NUEVO)

## 📚 Índice de Módulos

- [Comunicación RF](#comunicación-rf)
  - [NRF24L01+](#nrf24l01)
- [Sensores Ambientales](#sensores-ambientales)
  - [DHT22 / AM2302](#dht22--am2302)
- [Sensores de Distancia](#sensores-de-distancia)
  - [HC-SR04](#hc-sr04)
- [Displays](#displays)
  - [SSD1306 OLED](#ssd1306-oled)
- [RFID](#rfid)
  - [RC-522](#rc-522)
- [Actuadores](#actuadores)
  - [Módulo Relé 5V](#módulo-relé-5v)
- [IMU](#imu)
  - [MPU6050](#mpu6050)
- [Comunicación Inalámbrica](#comunicación-inalámbrica)
  - [RF 433MHz (RCSwitch)](#rf-433mhz-rcswitch)

---

## Comunicación RF

### NRF24L01+

| Atributo | Valor |
|----------|-------|
| **Voltaje** | 3.3V (¡NO 5V en lógica!) |
| **Pines** | SPI (SCK, MISO, MOSI) + CE + CSN |
| **Alcance** | ~100m (PA+LNA: ~1km) |
| **Frecuencia** | 2.4 GHz (125 canales) |
| **Librería** | `RF24` |
| **Notas** | Pin IRQ opcional pero recomendado para interrupciones. Alimentación requiere 3.3V regulado limpio (capacitor 10µF cerca del módulo). |

---

## Sensores Ambientales

### DHT22 / AM2302

| Atributo | Valor |
|----------|-------|
| **Protocolo** | Single-wire digital |
| **Voltaje** | 3.3V–5.5V (tolerante) |
| **Precisión** | ±0.5°C, ±2% RH |
| **Rango temp** | -40°C a +80°C |
| **Librería** | `DHT sensor library` (Adafruit) |
| **Nota crítica** | No usar en interrupciones. Timing crítico ~20-40ms por lectura. Intervalo mínimo entre lecturas: 2 segundos. |

---

## Sensores de Distancia

### HC-SR04

| Atributo | Valor |
|----------|-------|
| **Voltaje señal** | 5V (Trig input, Echo output) |
| **Voltaje alimentación** | 5V |
| **Pines** | Trig (input), Echo (output 5V) |
| **Rango** | 2cm – 400cm |
| **Resolución** | ~3mm |
| **Nota** | Echo es 5V — usar divisor resistivo o level shifter con ESP32/ESP8266 (3.3V). El pin Echo puede dañar GPIO 3.3V si se conecta directamente. |

---

## Displays

### SSD1306 OLED

| Atributo | Valor |
|----------|-------|
| **Voltaje** | 3.3V–5V (I2C, tolerante) |
| **Protocolo** | I2C (SDA/SCL) o SPI |
| **Resolución** | 128×64 px |
| **Dirección I2C** | 0x3C o 0x3D (verificar con scanner) |
| **Librería** | `Adafruit SSD1306` |
| **Notas** | Consumo muy bajo (~10 mA). Ideal para proyectos a batería. SPI es más rápido que I2C para refresco. |

---

## RFID

### RC-522

| Atributo | Valor |
|----------|-------|
| **Voltaje** | 3.3V (¡NO 5V en lógica!) |
| **Protocolo** | SPI (recomendado) o I2C/UART |
| **Frecuencia** | 13.56 MHz |
| **Rango** | ~3cm |
| **Librería** | `MFRC522` |
| **Notas** | Algunos módulos chinos tienen regulador 3.3V onboard pero la lógica SPI sigue siendo 3.3V. No conectar a 5V lógico. |

---

## Actuadores

### Módulo Relé 5V

| Atributo | Valor |
|----------|-------|
| **Voltaje señal** | 3.3V–5V (optocoplado, aislado) |
| **Voltaje carga** | hasta 250V AC / 30V DC |
| **Corriente** | hasta 10A (según modelo) |
| **Nota** | Algunos módulos son active-LOW. Verificar con LED onboard antes de conectar carga. |
| **Seguridad** | Nunca tocar terminales con carga conectada. Usar fusible en línea de carga. |

---

## IMU

### MPU6050

| Atributo | Valor |
|----------|-------|
| **Voltaje** | 3.3V–5V (regulador onboard) |
| **Protocolo** | I2C (SDA/SCL) |
| **Dirección** | 0x68 (AD0=GND) o 0x69 (AD0=VCC) |
| **Sensores** | Acelerómetro 3-axis ±2g/±4g/±8g/±16g + Giroscopio 3-axis ±250°/s a ±2000°/s |
| **Librería** | `Adafruit MPU6050` |
| **Notas** | Incluye DMP (Digital Motion Processor) para fusión de sensores. Requiere calibración inicial. |

---

## Comunicación Inalámbrica

### RF 433MHz (RCSwitch)

| Atributo | Valor |
|----------|-------|
| **Voltaje** | 5V (para módulos típicos) |
| **Protocolo** | Digital / INT (interrupción) |
| **Pines** | D2 (INT0) en Arduino Uno para receptor |
| **Librería** | `RCSwitch` |
| **Notas** | Transmisor: ~3V-12V (mayor voltaje = mayor alcance). Receptor: superregenerativo (ruidoso) o superheterodino (mejor). No encriptado — solo para proyectos domésticos. |

---

## 📝 Notas de Uso

### Agregar nueva placa al catálogo

1. Copiar template de sección
2. Completar especificaciones técnicas
3. Agregar mapeo de pines completo
4. Documentar consideraciones críticas
5. Incluir `platformio.ini` base
6. Actualizar tabla comparativa
7. Agregar al índice

### Agregar nuevo módulo periférico

1. Copiar template de tabla
2. Completar: voltaje, protocolo, pines, librería, notas críticas
3. Documentar niveles lógicos y conversiones necesarias
4. Agregar advertencias de seguridad si aplica
5. Actualizar índice de módulos

---

*Plantilla generada para workflow profesional de firmware embebido.*
