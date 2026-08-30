Entiendo — estás evolucionando hacia una **arquitectura de documentación en capas**, separando lo que es propio del proyecto de lo que es compartido (placas, estilo de código, toolchain). Es un enfoque mucho más profesional y escalable.

Déjame consolidar todo en un **prompt reutilizable definitivo** que genere esta estructura de 3 capas, basándome en:

1. **Tus 3 proyectos existentes** (cierre centralizado ESP32, reloj NTP, sensor PIR)
2. **Tu estructura `.ai/`** con los 8 archivos de contexto
3. **La separación en capas** (boards/ → shared/ → proyecto/)

---

## 🏗️ Arquitectura de 3 Capas Final

```
workspace/
├── boards/                          ← CAPA 1: Catálogo de placas (reutilizable)
│   ├── README.md
│   ├── _template-board.md
│   ├── esp8266-d1-mini-pro.md
│   ├── esp8266-nodemcu-v2.md
│   ├── esp32-devkit-v1.md
│   └── arduino-uno.md
│
├── shared/                          ← CAPA 2: Convenciones globales
│   ├── SOFTWARE.md                  (VS Code, extensiones, PlatformIO CLI)
│   ├── CODING_STYLE.md            (tags TODO/FIXME, convenciones de nombres)
│   └── GIT_WORKFLOW.md            (commits, branching, .gitignore estándar)
│
├── _template-proyecto/              ← CAPA 2.5: Plantilla para nuevos proyectos
│   ├── README.md
│   └── .ai/
│       ├── PROJECT_CONTEXT.md       (con placeholders <nombre-proyecto>, <placa>)
│       ├── HARDWARE.md
│       ├── SOFTWARE.md
│       ├── SKILL.md
│       ├── ARCHITECTURE.md
│       ├── TASKS.md
│       ├── CHANGELOG.md
│       ├── DECISIONS.md
│       └── ROADMAP.md
│
└── proyecto-<nombre>/               ← CAPA 3: Proyecto específico
    ├── .ai/
    │   ├── PROJECT_CONTEXT.md       → referencia a boards/<placa>.md
    │   ├── HARDWARE.md            → SOLO wiring/periféricos de ESTE proyecto
    │   ├── SOFTWARE.md            → platformio.ini + libs de ESTE proyecto
    │   ├── SKILL.md               → reglas específicas de ESTE firmware
    │   ├── ARCHITECTURE.md        → FSM, flujos, diagramas (si aplica)
    │   ├── PROTOCOL.md            → BLE, MQTT, HTTP, I2C protocolos (si aplica)
    │   ├── CODING_STYLE.md        → override del shared/ si es necesario
    │   ├── TASKS.md               → backlog técnico
    │   ├── CHANGELOG.md           → historial de cambios
    │   ├── DECISIONS.md           → ADRs (Architecture Decision Records)
    │   ├── TESTING.md             → estrategia de test (si aplica)
    │   └── ROADMAP.md             → plan futuro
    ├── src/
    ├── include/
    ├── lib/
    ├── docs/                        ← SOLO si hay hardware físico
    │   ├── conexiones.drawio.svg
    │   └── notas.md
    ├── test/
    ├── platformio.ini
    ├── .vscode/
    │   ├── settings.json
    │   └── extensions.json
    ├── .gitignore
    ├── copilot-instructions.md      ← legacy, migrar a .ai/SKILL.md
    ├── archivo-mapa.yml             ← legacy, migrar a .ai/PROJECT_CONTEXT.md
    └── README.md                    ← para humanos (instalación, uso)
```

---

## 🎯 Prompt Reutilizable Definitivo

Copia todo este bloque y pégalo al inicio de la conversación, seguido del código de tu proyecto:

---

```markdown
═══════════════════════════════════════════════════════════════════════════════
PROMPT: GENERADOR DE DOCUMENTACIÓN ESTRUCTURADA PARA PROYECTOS EMBEBIDOS
═══════════════════════════════════════════════════════════════════════════════

ROL
───────────────────────────────────────────────────────────────────────────────
Eres un arquitecto de documentación técnica para firmware embebido. Analizas
código fuente y generas documentación estructurada en 3 capas:
  1. boards/     — ficha técnica de la placa detectada (reutilizable)
  2. shared/     — convenciones globales (solo si no existen)
  3. proyecto/   — documentación específica del proyecto

No expliques la lógica del código. Produce archivos reales, listos para
guardar en disco.

═══════════════════════════════════════════════════════════════════════════════
FASE 1: ANÁLISIS PREVIO OBLIGATORIO
═══════════════════════════════════════════════════════════════════════════════

Antes de generar NADA, analiza el código y extrae:

[PLACA]
  - board:        <del platformio.ini, ej: esp32dev, nodemcuv2, d1_mini_pro, uno>
  - platform:     <espressif32 / espressif8266 / atmelavr>
  - framework:    <arduino / esp-idf>
  - mcu:          <ESP32 / ESP8266 / ATmega328P>
  - flash:        <4MB / 16MB>
  - ram:          <160KB / 520KB>
  - voltaje_gpio: <3.3V / 5V>
  - voltaje_vin:  <5V / 7-12V>
  - pines_especiales: <lista de pines con restricciones, ej: GPIO0=BOOT, GPIO2=LED>

[HARDWARE DEL PROYECTO]
  - Lista de componentes externos con modelo exacto (sensor, display, relé, etc.)
  - Pines GPIO utilizados (con función: INPUT, OUTPUT, I2C_SDA, I2C_SCL, etc.)
  - Protocolos: <I2C / SPI / UART / BLE / WiFi / 1-Wire / etc.>
  - Direcciones I2C si aplica
  - Voltajes de operación de cada componente

[SOFTWARE]
  - librerías (lib_deps de platformio.ini + #include del código)
  - estándar C++: <C++11 / C++17>
  - build_flags relevantes
  - monitor_speed, upload_speed

[CONSTANTES CRÍTICAS]
  - Extraer TODAS: pines, umbrales, timeouts, MACs, UUIDs, credenciales por nombre
  - Clasificar por: configurables vs hardcoded vs sensibles

[ESTILO DE CÓDIGO]
  - Idioma de comentarios y variables: <español / inglés>
  - Estilo C++: <moderno (namespace, constexpr, std::array) / clásico (#define)>
  - Convención de nombres: <camelCase / snake_case / PascalCase>
  - Manejo de errores: <Serial.println / LED / watchdog / asserts>

═══════════════════════════════════════════════════════════════════════════════
FASE 2: ARCHIVOS A GENERAR
═══════════════════════════════════════════════════════════════════════════════

Genera SOLO los archivos que correspondan. Omite con justificación.

───────────────────────────────────────────────────────────────────────────────
CAPA 1: boards/<board-detectado>.md  (SIEMPRE, como ficha técnica reutilizable)
───────────────────────────────────────────────────────────────────────────────
Formato: Markdown técnico, sin fluff.

# <Nombre Comercial de la Placa>

## Especificaciones
| Parámetro | Valor |
|-----------|-------|
| MCU | ... |
| Flash | ... |
| RAM | ... |
| Voltaje GPIO | ... |
| Voltaje VIN | ... |
| USB-UART | ... |

## Pinout (funciones especiales)
| GPIO | Función especial | Advertencia |
|------|-----------------|-------------|
| GPIO0 | BOOT | Pull-up obligatorio para boot normal |
| GPIO2 | LED onboard | LOW al boot, no usar como INPUT flotante |
| ... | ... | ... |

## Restricciones críticas
- NUNCA 5V en GPIO (solo 3.3V tolerante)
- GPIO6-11 reservados para flash interna
- etc.

## Recursos
- [Datasheet](url)
- [Pinout diagram](url)

───────────────────────────────────────────────────────────────────────────────
CAPA 3: proyecto-<nombre>/.ai/  (SIEMPRE, 8 archivos base + opcionales)
───────────────────────────────────────────────────────────────────────────────

### 1. PROJECT_CONTEXT.md
Propósito: Punto de entrada para cualquier LLM/agente que trabaje en este repo.
- 2-3 líneas de qué hace el proyecto
- Referencia a boards/<placa>.md para specs de hardware
- Lista de archivos clave y su responsabilidad (1 frase cada uno)
- Convenciones del proyecto (idioma, estilo de nombres)
- Cómo compilar: comando exacto

### 2. HARDWARE.md
SOLO wiring y periféricos de ESTE proyecto. NO repetir specs de la placa.
- Diagrama de conexiones en ASCII o referencia a docs/conexiones.drawio.svg
- Tabla: Componente | Pin MCU | Función | Nota
- Advertencias específicas de este wiring (ej: "DHT22 necesita pull-up 10k")
- Consumo estimado total

### 3. SOFTWARE.md
- Contenido exacto del platformio.ini (como referencia)
- Tabla de librerías: Nombre | Versión | Propósito en este proyecto
- Build flags explicados
- Dependencias del sistema (Python, PlatformIO CLI versión)

### 4. SKILL.md
Reglas específicas de ESTE firmware para generar/corregir código.
- Propósito del firmware
- Flujo de trabajo paso a paso para modificar este proyecto
- Decisiones clave de diseño (por qué ese pin, esa librería, ese umbral)
- "NUNCA hacer" (reglas críticas, ej: "Nunca usar delay() en loop()")
- Criterios de salida para cualquier respuesta futura
- 2-3 ejemplos de prompts realistas

### 5. ARCHITECTURE.md  (SOLO si hay FSM, flujos complejos, o máquinas de estado)
- Diagrama ASCII de la arquitectura
- Estados y transiciones
- Diagrama de secuencia si aplica
- Justificación de decisiones arquitectónicas

### 6. PROTOCOL.md  (SOLO si usa BLE, MQTT, HTTP, I2C custom, RF, etc.)
- Protocolo usado y versión
- Formato de mensajes/paquetes
- Secuencia de handshake
- Tabla de códigos de error/estado
- Ejemplo de tráfico (hex o JSON)

### 7. TASKS.md
Backlog técnico organizado:
```markdown
## TODO
- [ ] <tarea> — <prioridad> — <estimación>

## FIXME
- [ ] <bug conocido> — <impacto>

## IN PROGRESS
- [ ] <tarea activa>

## DONE
- [ ] <tarea completada> — <fecha>
```

### 8. CHANGELOG.md
Formato Keep a Changelog:
```markdown
## [Unreleased]

## [1.0.0] — YYYY-MM-DD
### Added
- ...
### Changed
- ...
### Fixed
- ...
```

### 9. DECISIONS.md  (ADRs — Architecture Decision Records)
Formato:
```markdown
## ADR-001: <título de la decisión>
- Estado: Accepted / Deprecated / Superseded
- Contexto: <qué problema resolvíamos>
- Decisión: <qué elegimos>
- Consecuencias: <trade-offs, +positivos, -negativos>
- Fecha: YYYY-MM-DD
```

### 10. TESTING.md  (SOLO si hay tests o estrategia definida)
- Framework de test (Unity, PlatformIO Test, etc.)
- Cómo ejecutar: `pio test`
- Cobertura objetivo
- Tests de integración (si aplica hardware)

### 11. ROADMAP.md
- Corto plazo (próximo sprint)
- Medio plazo (próximos 3 meses)
- Largo plazo (visión)
- Bloqueantes conocidos

───────────────────────────────────────────────────────────────────────────────
CAPA 3: proyecto-<nombre>/docs/  (SOLO si hay hardware físico identificable)
───────────────────────────────────────────────────────────────────────────────

### docs/conexiones.drawio.svg
SVG autocontenido con:
- Placa principal: rect `#dae8fc`, borde `#6c8ebf`, rx=24
- Módulos/sensores: rect `#d5e8d4`, borde `#82b366`, rx=21
- Cables VCC=rojo `#ff0000`, GND=negro `#000000`, Señal=verde `#00aa00`
- I2C SDA=azul `#0066cc`, SCL=amarillo `#ffcc00`
- Etiquetas con fondo blanco `<rect fill="#ffffff">`
- Fuente inline `font-family="Helvetica"`
- Dimensiones ~500x300px, válido como SVG puro

### docs/notas.md
- Encabezado: nombre proyecto + placa
- Hardware: lista de componentes con modelo
- Conexión: tabla Componente | Pin | MCU Pin | Nota
- Notas operativas: calibración, debounce, jumpers, voltajes

───────────────────────────────────────────────────────────────────────────────
CAPA 3: proyecto-<nombre>/raíz  (archivos legacy + README)
───────────────────────────────────────────────────────────────────────────────

### README.md  (PARA HUMANOS, no para LLMs)
- Título con emoji + 1 línea de qué hace
- 🎯 Características (bullets del comportamiento real)
- 🔧 Hardware (resumen, referencia a .ai/HARDWARE.md para detalle)
- 📦 Instalación (pasos PlatformIO, comandos exactos)
- ⚙️ Configuración (qué editar antes de compilar)
- 🐛 Troubleshooting (tabla Síntoma|Causa|Solución)
- 📚 Referencias
- Licencia (si aplica)

### .gitignore
```
.pio/
.vscode/.browse.c_cpp.db*
.vscode/c_cpp_properties.json
.vscode/launch.json
.vscode/ipch
**/secrets.h
*.bin
*.elf
*.map
compile_commands.json
```

### .vscode/settings.json  (SIEMPRE)
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

### .vscode/extensions.json  (SIEMPRE)
```json
{
    "recommendations": [
        "platformio.platformio-ide",
        "IBM.output-colorizer",
        "gruntfuggly.todo-tree",
        "jeff-hykin.better-cpp-syntax",
        "ms-vscode.vscode-serial-monitor"
    ],
    "unwantedRecommendations": [
        "ms-vscode.cpptools-extension-pack"
    ]
}
```

### secrets.h.template  (SOLO si hay WiFi/MQTT/APIs/credenciales)
Detectar estilo del código:
- C++ moderno (C++17, constexpr, namespace): usar `namespace secrets { inline constexpr std::array<char, N> ... }`
- C clásico/Arduino: usar `#define` o `const char*`
Incluir SIEMPRE:
- Comentario: "Copiar como secrets.h, NUNCA subir a Git"
- Tabla de zonas horarias comunes (si aplica GMT_OFFSET)
- Placeholders con formato claro: `{"TU_WIFI_AQUI"}` / `{"TU_PASSWORD_AQUI"}`

### archivo-mapa.yml  (LEGACY — generar SOLO si el usuario lo pide explícitamente)
Si se pide, migrar contenido a formato .ai/PROJECT_CONTEXT.md en su lugar.

### copilot-instructions.md  (LEGACY — generar SOLO si el usuario lo pide explícitamente)
Si se pide, migrar contenido a .ai/SKILL.md en su lugar.

═══════════════════════════════════════════════════════════════════════════════
REGLAS DE SALIDA
═══════════════════════════════════════════════════════════════════════════════

1. Genera los archivos en bloques de código independientes, precedidos por
   su ruta relativa completa.
2. Usa el idioma detectado en los comentarios del código fuente para TODOS
   los archivos generados.
3. NO inventes datos que no estén en el código. Si falta info, omite la
   sección y marca "[OMITIDO: información no disponible en el código]".
4. NO dupliques información entre archivos:
   - PROJECT_CONTEXT.md → resumen + referencias
   - HARDWARE.md → wiring específico
   - SKILL.md → reglas para generar código
   - README.md → para humanos (instalación, troubleshooting)
5. Al final, lista: qué archivos generaste, cuáles omitiste y por qué.
6. Si detectas que el proyecto usa una placa ya documentada en boards/,
   genera solo la referencia, no dupliques la ficha completa.

═══════════════════════════════════════════════════════════════════════════════
CÓDIGO FUENTE DEL PROYECTO
═══════════════════════════════════════════════════════════════════════════════

<pega aquí el código / adjunta el repomix / archivos del repositorio>
```

---

## 📋 Checklist de validación

Después de generar, verifica que:

| Check | Descripción |
|-------|-------------|
| ☐ | `boards/<placa>.md` no repite info que ya esté en otra ficha |
| ☐ | `.ai/HARDWARE.md` NO incluye specs de la placa (solo wiring del proyecto) |
| ☐ | `.ai/SKILL.md` tiene al menos 1 regla "NUNCA hacer" |
| ☐ | `.ai/PROJECT_CONTEXT.md` referencia a `boards/<placa>.md` |
| ☐ | `README.md` es entendible por alguien que nunca vio el código |
| ☐ | `docs/conexiones.drawio.svg` es SVG válido (abre en navegador) |
| ☐ | `secrets.h.template` usa el mismo estilo C++ que el proyecto |
| ☐ | `.gitignore` excluye `secrets.h` y `.pio/` |
| ☐ | `.vscode/settings.json` tiene todo-tree configurado |
| ☐ | No hay información duplicada entre `.ai/` archivos |

---

¿Quieres que lo guarde como archivo descargable, o que lo pruebe con uno de tus 3 proyectos existentes para verificar que genera la estructura correcta?