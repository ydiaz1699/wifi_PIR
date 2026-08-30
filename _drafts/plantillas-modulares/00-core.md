# 📋 00-CORE — Entorno VS Code + PlatformIO

> **Pegar SIEMPRE este archivo al inicio de cualquier chat nuevo.**
> No cambia seguido — es tu base fija. Lo que sí varía por proyecto
> (placa, periféricos) va en archivos separados de `placas/` y `perifericos/`.

---

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

> 📌 **Archivo estático — no regenerar, copiar desde tu plantilla local:**
> `ENTORNO_VS-CODE/.vscode/extensions.json` → `<mi_proyecto>/.vscode/extensions.json`

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

### `.vscode/tasks.json` (Task Runner)

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

### `.vscode/platformio-snippets.code-snippets`

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

> 📌 **Estos 4 archivos de `.vscode/` son estáticos entre proyectos.**
> Copialos con el script/comando de tu plantilla local en vez de pedirle
> a Claude que los regenere. Ver sección "Comandos de copia" al final.

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

## 🔧 Pre-commit Hooks

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

## 🐳 Docker para CI Local

### `Dockerfile`

```dockerfile
FROM python:3.11-slim

RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir platformio

WORKDIR /workspace

COPY platformio.ini .
RUN pio pkg install

COPY . .

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

## 🔧 Comandos Frecuentes de PlatformIO

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

## 🎯 Workflow Recomendado

1. **Inicializar proyecto:**
   ```bash
   pio init --board d1_mini_pro --ide vscode
   ```

2. **Configurar VS Code:**
   - Copiar `.vscode/settings.json`, `extensions.json`, `tasks.json`, `platformio-snippets.code-snippets` desde tu plantilla local
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

## 📋 Comandos de Copia (Windows PowerShell)

Para plantar archivos estáticos (`.vscode/`, `.clang-format`, etc.) desde tu
carpeta de plantilla local hacia un proyecto nuevo, sin que el LLM los regenere:

```powershell
# Definir ruta de la plantilla una vez
$plantilla = "C:\ruta\a\ENTORNO_VS-CODE"

# Copiar toda la carpeta .vscode
Copy-Item "$plantilla\.vscode" -Destination ".\.vscode" -Recurse -Force

# Copiar archivos sueltos adicionales
Copy-Item "$plantilla\.clang-format" -Destination ".\.clang-format" -Force
Copy-Item "$plantilla\.pre-commit-config.yaml" -Destination ".\.pre-commit-config.yaml" -Force
```

---

## 📌 Qué pegar en un chat nuevo

1. **Este archivo (`00-core.md`)** — siempre.
2. **El archivo de la placa** que estés usando, desde `placas/` (ver índice abajo).
3. **Los archivos de periféricos** conectados a ese proyecto puntual, desde `perifericos/`.

No pegues el catálogo completo — cada placa y periférico vive en su propio
archivo para que solo cargues lo relevante al proyecto del día.
