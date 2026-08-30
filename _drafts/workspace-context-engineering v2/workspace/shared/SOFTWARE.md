# SOFTWARE.md (Compartido) — Entorno de Desarrollo

> Aplica a **todos** los proyectos del workspace. Cada proyecto individual puede tener
> su propio `SOFTWARE.md` para lo específico (librerías, `platformio.ini`), pero el
> entorno base es este.

## IDE y Herramientas

- **IDE:** VS Code + PlatformIO
- **Monitor Serial:** VS Code Serial Monitor (`ms-vscode.vscode-serial-monitor`)
- **Baudrate por defecto:** `115200` (ajustar según proyecto en su `platformio.ini`)
- **Framework:** Arduino
- **Tema VS Code:** Claro

## Extensiones VS Code Instaladas

| Extensión | ID | Función |
| --- | --- | --- |
| PlatformIO IDE | `platformio.platformio-ide` | Core del workflow: build, upload, monitor, librerías |
| VS Code Serial Monitor | `ms-vscode.vscode-serial-monitor` | Monitor serial nativo con timestamps y formato hex |
| Better C++ Syntax | `jeff-hykin.better-cpp-syntax` | Syntax highlighting mejorado para C++ moderno |
| IBM Output Colorizer | `IBM.output-colorizer` | Colores en el panel de output de PlatformIO |
| Draw.io Integration | `hediet.vscode-drawio` | Diagramas de conexión hardware y flujos de estado |
| C/C++ Extension Pack | `ms-vscode.cpptools-extension-pack` | IntelliSense, navegación de código, autocompletado |
| Todo Tree | `gruntfuggly.todo-tree` | Tracking de `TODO`, `FIXME`, `HACK`, `BUG`, `NOTE`, `WARN` |

## `settings.json` base (colocar en `.vscode/` de cada proyecto)

```json
{
    "todo-tree.general.tags": ["TODO","FIXME","HACK","BUG","XXX","NOTE","WARN"],
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
        "WARN":  { "icon": "alert", "background": "#f39c12", "iconColour": "#f39c12" }
    },
    "todo-tree.filtering.includeGlobs": [
        "**/*.cpp","**/*.h","**/*.hpp","**/*.c","**/*.ino","**/*.py","**/*.md"
    ],
    "todo-tree.tree.showCountsInTree": true,
    "C_Cpp.intelliSenseCacheSize": 512,
    "C_Cpp.errorSquiggles": "Enabled",
    "C_Cpp.autocomplete": "Default",
    "C_Cpp.intelliSenseEngine": "Default",
    "C_Cpp.intelliSenseEngineFallback": "Disabled",
    "vscode-serial-monitor.default.baudRate": 115200,
    "vscode-serial-monitor.default.lineEnding": "LF",
    "vscode-serial-monitor.default.timestamp": true
}
```

## `extensions.json` base (colocar en `.vscode/` de cada proyecto)

```json
{
    "recommendations": [
        "platformio.platformio-ide",
        "ms-vscode.vscode-serial-monitor",
        "jeff-hykin.better-cpp-syntax",
        "IBM.output-colorizer",
        "hediet.vscode-drawio",
        "ms-vscode.cpptools-extension-pack",
        "gruntfuggly.todo-tree"
    ]
}
```

## Nota para la IA

Este archivo describe **herramientas de desarrollo**, no hardware. Para pines, voltajes
o periféricos, consulta el `HARDWARE.md` del proyecto específico. No mezcles ambos
contextos al proponer cambios.
