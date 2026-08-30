Veo que ya tienes una **arquitectura de catálogo muy madura** con `plantillas-modulares/` — es excelente. Ahora entiendo el panorama completo:

1. **Tus 3 proyectos existentes** (PKE ESP32, Reloj NTP, Sensor PIR) → con documentación "legacy" dispersa
2. **Tu catálogo `plantillas-modulares/`** → con `placas/`, `perifericos/`, `00-core.md`
3. **Tu objetivo** → Un prompt reutilizable que, dado un proyecto nuevo, genere los archivos `.ai/` + `docs/` + raíz, **referenciando** el catálogo en vez de duplicarlo

Déjame consolidar todo en el **prompt reutilizable definitivo** que integre tu catálogo existente.

---

## 🏗️ Arquitectura Final Integrada

```
workspace/
├── plantillas-modulares/          ← TU CATÁLOGO EXISTENTE (no regenerar)
│   ├── 00-core.md
│   ├── placas/
│   │   ├── 00-indice-placas.md
│   │   ├── esp32-devkit.md
│   │   ├── nodemcu-v3.md
│   │   ├── d1-mini-pro.md
│   │   ├── arduino-uno.md
│   │   └── ... (12 placas)
│   └── perifericos/
│       ├── 00-indice-perifericos.md
│       ├── dht22.md
│       ├── hc-sr04.md
│       ├── ssd1306.md
│       ├── rele-5v.md
│       └── ... (8 periféricos)
│
├── _template-proyecto/            ← PLANTILLA PARA NUEVOS PROYECTOS
│   ├── README.md
│   └── .ai/
│       ├── PROJECT_CONTEXT.md
│       ├── HARDWARE.md
│       ├── SOFTWARE.md
│       ├── SKILL.md
│       ├── ARCHITECTURE.md
│       ├── TASKS.md
│       ├── CHANGELOG.md
│       ├── DECISIONS.md
│       └── ROADMAP.md
│
└── proyecto-<nombre>/             ← PROYECTO ESPECÍFICO (lo que genera el prompt)
    ├── .ai/
    │   ├── PROJECT_CONTEXT.md      → referencia a plantillas-modulares/placas/<placa>.md
    │   ├── HARDWARE.md           → SOLO wiring de ESTE proyecto + refs a periféricos
    │   ├── SOFTWARE.md           → platformio.ini + libs de ESTE proyecto
    │   ├── SKILL.md              → reglas específicas de ESTE firmware
    │   ├── ARCHITECTURE.md       → FSM/flujos (si aplica)
    │   ├── PROTOCOL.md           → BLE/MQTT/HTTP/I2C custom (si aplica)
    │   ├── CODING_STYLE.md       → override del 00-core si es necesario
    │   ├── TASKS.md
    │   ├── CHANGELOG.md
    │   ├── DECISIONS.md
    │   ├── TESTING.md
    │   └── ROADMAP.md
    ├── src/
    ├── include/
    ├── lib/
    ├── docs/                      ← SOLO si hay hardware físico
    │   ├── conexiones.drawio.svg
    │   └── notas.md
    ├── test/
    ├── platformio.ini
    ├── .vscode/
    │   ├── settings.json          ← COPIAR de plantillas-modulares/00-core.md
    │   ├── extensions.json        ← COPIAR de plantillas-modulares/00-core.md
    │   ├── tasks.json             ← COPIAR de plantillas-modulares/00-core.md
    │   └── platformio-snippets.code-snippets  ← COPIAR de 00-core.md
    ├── .gitignore
    ├── .clang-format              ← COPIAR de plantillas-modulares/00-core.md
    ├── .pre-commit-config.yaml    ← COPIAR de plantillas-modulares/00-core.md
    └── README.md                  ← para humanos (instalación, uso)
```

---

## 🎯 Prompt Reutilizable Definitivo (Integrado con tu catálogo)

Copia TODO este bloque y pégalo al inicio de la conversación, seguido del código de tu proyecto:

```markdown
═══════════════════════════════════════════════════════════════════════════════
PROMPT: GENERADOR DE PROYECTO EMBEBIDO CON CATÁLOGO DE HARDWARE
═══════════════════════════════════════════════════════════════════════════════

ROL
───────────────────────────────────────────────────────────────────────────────
Eres un arquitecto de firmware embebido. Analizas código fuente y generas la
documentación estructurada de un proyecto específico, REFERENCIANDO un catálogo
de hardware existente en vez de duplicar información.

CATÁLOGO EXISTENTE (NO regenerar estos archivos):
  plantillas-modulares/
  ├── 00-core.md              → VS Code, PlatformIO, convenciones globales
  ├── placas/<placa>.md       → Ficha técnica de cada placa (12 disponibles)
  └── perifericos/<modulo>.md → Ficha técnica de cada periférico (8 disponibles)

PLACA DISPONIBLES EN CATÁLOGO:
  esp32-devkit, esp32-s3, esp32-c3, esp8266-nodemcu-v3, esp8266-d1-mini-pro,
  esp8266-esp-01s, arduino-uno, arduino-nano, arduino-mega, stm32-bluepill,
  stm32-blackpill, rp2040-pico

PERIFÉRICOS DISPONIBLES EN CATÁLOGO:
  dht22, hc-sr04, ssd1306, rc522, nrf24l01, rele-5v, rf433-rcswitch, mpu6050

REGLA CRÍTICA: Nunca dupliques en el proyecto specs que ya están en el catálogo.
Usa referencias del tipo: "Ver plantillas-modulares/placas/esp32-devkit.md para
especificaciones completas de la placa."

═══════════════════════════════════════════════════════════════════════════════
FASE 1: ANÁLISIS PREVIO OBLIGATORIO
═══════════════════════════════════════════════════════════════════════════════

Antes de generar NADA, analiza el código y completa esta tabla mental:

[PLACA DETECTADA]
  - board (platformio.ini):     ________________
  - placa del catálogo:         ________________  (mapear a nombre de archivo .md)
  - ¿Existe en catálogo?        SÍ / NO → si NO, generar ficha básica

[PERIFÉRICOS DETECTADOS]
  - Lista de componentes:       ________________
  - ¿Existen en catálogo?       SÍ / NO por cada uno → si NO, generar ficha básica

[SOFTWARE]
  - librerías (lib_deps):       ________________
  - estándar C++:              ________________
  - build_flags:               ________________
  - monitor_speed:             ________________

[CONSTANTES CRÍTICAS DEL PROYECTO]
  - Pines GPIO:                 ________________
  - Umbrales/timeouts:          ________________
  - MACs/UUIDs/tokens:         ________________
  - Credenciales por nombre:   ________________

[ESTILO DE CÓDIGO]
  - Idioma:                     español / inglés
  - Estilo C++:                 moderno / clásico
  - Convención nombres:         camelCase / snake_case / PascalCase

═══════════════════════════════════════════════════════════════════════════════
FASE 2: ARCHIVOS A GENERAR
═══════════════════════════════════════════════════════════════════════════════

Genera SOLO los que correspondan. Omite con justificación "[OMITIDO: razón]".

───────────────────────────────────────────────────────────────────────────────
A. FICHAS DE CATÁLOGO (SOLO si la placa o periférico NO existen en catálogo)
───────────────────────────────────────────────────────────────────────────────

Si la placa detectada NO está en plantillas-modulares/placas/:
→ Generar plantillas-modulares/placas/<nueva-placa>.md con formato:
  # 🖥️ Placa: <Nombre>
  ## 🧩 Hardware Principal (tabla)
  ## 🗺️ Mapeo de Pines (tabla)
  ## ⚡ Niveles de Voltaje
  ## ⚠️ Consideraciones Críticas
  ## 📡 platformio.ini

Si un periférico detectado NO está en plantillas-modulares/perifericos/:
→ Generar plantillas-modulares/perifericos/<nuevo-modulo>.md con formato:
  # 🔌 Periférico: <Nombre>
  | Atributo | Valor |
  | Categoría | ... |
  | Voltaje | ... |
  | Protocolo | ... |
  | Pines | ... |
  | Librería | ... |
  | Nota crítica | ... |

───────────────────────────────────────────────────────────────────────────────
B. PROYECTO ESPECÍFICO: proyecto-<nombre>/.ai/
───────────────────────────────────────────────────────────────────────────────

### 1. .ai/PROJECT_CONTEXT.md (SIEMPRE)
Punto de entrada para cualquier LLM/agente.

```markdown
# PROJECT_CONTEXT — <nombre-proyecto>

## 🎯 Propósito
<2-3 líneas de qué hace el firmware>

## 🔗 Referencias al Catálogo
- Placa: [plantillas-modulares/placas/<placa>.md](../../plantillas-modulares/placas/<placa>.md)
- Periféricos:
  - [plantillas-modulares/perifericos/<modulo>.md](../../plantillas-modulares/perifericos/<modulo>.md)
  - ...

## 📁 Archivos Clave
| Archivo | Responsabilidad |
|---------|----------------|
| src/main.cpp | <1 frase> |
| include/<header>.h | <1 frase> |
| ... | ... |

## ⚙️ Convenciones del Proyecto
- Idioma: <español/inglés>
- Estilo de nombres: <camelCase/snake_case>
- Manejo de errores: <Serial.println/LED/watchdog>

## 🚀 Cómo Compilar
```bash
pio run
pio run -t upload
pio device monitor
```
```

### 2. .ai/HARDWARE.md (SIEMPRE que haya hardware físico)
SOLO wiring de ESTE proyecto. NUNCA repetir specs de placa o periférico.

```markdown
# HARDWARE — <nombre-proyecto>

## 🔗 Referencias
- Placa base: [plantillas-modulares/placas/<placa>.md](../../plantillas-modulares/placas/<placa>.md)
- Periféricos: ver referencias en PROJECT_CONTEXT.md

## 🔌 Wiring Específico de Este Proyecto

| Componente | Pin MCU | Función | Nota |
|------------|---------|---------|------|
| <modulo> | GPIO<X> | <INPUT/OUTPUT/I2C_SDA/etc> | <nota específica del proyecto> |

## ⚡ Consumo Estimado
- Placa: <X> mA
- Periféricos: <Y> mA
- **Total: <Z> mA**

## ⚠️ Advertencias de Este Wiring
- <advertencia específica del proyecto, ej: "DHT22 en GPIO4 requiere pull-up 10k">
- <otra advertencia>
```

### 3. .ai/SOFTWARE.md (SIEMPRE)
```markdown
# SOFTWARE — <nombre-proyecto>

## 📡 platformio.ini
```ini
<contenido exacto del platformio.ini>
```

## 📚 Librerías
| Librería | Versión | Propósito en este proyecto |
|----------|---------|---------------------------|
| <nombre> | <versión> | <para qué se usa> |

## 🛠️ Build Flags
| Flag | Valor | Propósito |
|------|-------|-----------|
| <flag> | <valor> | <para qué sirve> |

## 🔧 Dependencias del Sistema
- PlatformIO CLI: <versión mínima>
- Python: <versión>
- VS Code: <extensiones recomendadas>
```

### 4. .ai/SKILL.md (SIEMPRE)
Reglas para que un LLM futuro genere/corrija código de ESTE proyecto.

```markdown
# SKILL — <nombre-proyecto>

## 🎯 Propósito
<qué problema resuelve el firmware>

## 🔄 Flujo de Trabajo
1. Detectar intención del usuario
2. Configurar proyecto (board=<board>, framework=<framework>)
3. Construir lógica principal
4. Controlar salidas/entradas
5. Mantener estilo y convenciones

## 🧠 Decisiones Clave
- <por qué se eligió ese pin, esa librería, ese umbral>
- <por qué FSM no bloqueante vs delay()>
- <trade-off de diseño>

## 🚫 NUNCA Hacer
- <regla crítica 1, ej: "Nunca usar delay() en loop()">
- <regla crítica 2, ej: "Nunca conectar 5V a GPIO del ESP32">
- <regla crítica 3>

## ✅ Criterios de Salida
Cualquier respuesta futura sobre este proyecto debe incluir:
- [ ] Código completo y funcional
- [ ] Valores actualizados de constantes
- [ ] Breve explicación de cambios
- [ ] Comentarios en <idioma>
- [ ] Nada irrelevante

## 💬 Ejemplos de Prompts
- "Cambiar el umbral de RSSI de -78 a -85 dBm"
- "Agregar un segundo sensor PIR en GPIO17"
- "Optimizar el consumo con deep sleep entre lecturas"
```

### 5. .ai/ARCHITECTURE.md (SOLO si hay FSM, flujos complejos, o máquinas de estado)
```markdown
# ARCHITECTURE — <nombre-proyecto>

## 🏗️ Diagrama de Arquitectura
```ascii
<diagrama ASCII del flujo>
```

## 📊 Máquinas de Estado
### <Nombre FSM>
| Estado | Transición → | Condición |
|--------|-------------|-----------|
| <estado> | <siguiente> | <condición> |

## 🔄 Diagrama de Secuencia
<ASCII o descripción>

## 📝 Justificación de Decisiones
<por qué se eligió esta arquitectura>
```

### 6. .ai/PROTOCOL.md (SOLO si usa BLE, MQTT, HTTP, I2C custom, RF, etc.)
```markdown
# PROTOCOL — <nombre-proyecto>

## 📡 Protocolo: <nombre>
- Versión: <X>
- Tipo: <BLE/MQTT/HTTP/I2C/RF/etc>

## 📋 Formato de Mensajes
| Campo | Tipo | Tamaño | Descripción |
|-------|------|--------|-------------|
| <campo> | <tipo> | <bytes> | <desc> |

## 🔌 Secuencia de Handshake
1. <paso 1>
2. <paso 2>

## ❌ Códigos de Error
| Código | Significado |
|--------|-------------|
| <code> | <significado> |

## 📊 Ejemplo de Tráfico
<hex dump o JSON de ejemplo>
```

### 7. .ai/TASKS.md (SIEMPRE — empezar con backlog del código)
```markdown
# TASKS — <nombre-proyecto>

## 📝 TODO
- [ ] <tarea detectada en código> — <prioridad>

## 🔧 FIXME
- [ ] <bug conocido> — <impacto>

## 🚧 IN PROGRESS
- [ ] <tarea activa>

## ✅ DONE
- [ ] <tarea completada> — <fecha>
```

### 8. .ai/CHANGELOG.md (SIEMPRE — empezar vacío con formato)
```markdown
# CHANGELOG — <nombre-proyecto>

## [Unreleased]

## [0.1.0] — YYYY-MM-DD
### Added
- Versión inicial del firmware
```

### 9. .ai/DECISIONS.md (SIEMPRE — empezar con decisiones detectadas en código)
```markdown
# DECISIONS — <nombre-proyecto>

## ADR-001: <título de decisión detectada>
- **Estado:** Accepted
- **Contexto:** <qué problema resolvíamos>
- **Decisión:** <qué elegimos>
- **Consecuencias:** <trade-offs>
- **Fecha:** YYYY-MM-DD
```

### 10. .ai/TESTING.md (SOLO si hay tests o estrategia definida)
```markdown
# TESTING — <nombre-proyecto>

## 🧪 Framework
<Unity / PlatformIO Test / etc>

## ▶️ Cómo Ejecutar
```bash
pio test
```

## 📊 Cobertura Objetivo
<qué se quiere testear>

## 🔌 Tests de Integración
<si aplica hardware>
```

### 11. .ai/ROADMAP.md (SIEMPRE — empezar con plan básico)
```markdown
# ROADMAP — <nombre-proyecto>

## 🎯 Corto Plazo
- [ ] <próxima feature>

## 🚀 Medio Plazo
- [ ] <feature a 3 meses>

## 🔮 Largo Plazo
- [ ] <visión>

## 🚧 Bloqueantes
- <qué impide avanzar>
```

### 12. .ai/CODING_STYLE.md (SOLO si difiere de 00-core.md)
```markdown
# CODING_STYLE — <nombre-proyecto>

## 📝 Overrides del 00-core.md
<qué convenciones son diferentes en este proyecto>

## 🏷️ Tags Adicionales
<tags extra de TODO/FIXME/etc>
```

───────────────────────────────────────────────────────────────────────────────
C. PROYECTO ESPECÍFICO: docs/ (SOLO si hay hardware físico identificable)
───────────────────────────────────────────────────────────────────────────────

### docs/conexiones.drawio.svg
SVG autocontenido con especificación visual obligatoria:
- Placa principal: rect `#dae8fc`, borde `#6c8ebf`, rx=24, ry=24
- Módulos/sensores: rect `#d5e8d4`, borde `#82b366`, rx=21, ry=21
- Cables VCC=rojo `#ff0000`, GND=negro `#000000`, Señal=verde `#00aa00`
- I2C SDA=azul `#0066cc`, SCL=amarillo `#ffcc00`
- Etiquetas: texto con `<rect fill="#ffffff" stroke="none">` como fondo blanco
- Fuente inline: `font-family="Helvetica"` (sin dependencias externas)
- Dimensiones: ~500x300 px
- Flechas con `pointer-events="stroke"` y `stroke-miterlimit="10"`
- Debe ser SVG válido y autocontenido

### docs/notas.md
```markdown
# <nombre-proyecto> — <placa>

## 🔌 Hardware
- <lista de componentes con modelo exacto>

## 🔗 Conexión
| Componente | Pin | MCU Pin | Nota |
|------------|-----|---------|------|
| <modulo> | <pin> | <GPIO> | <nota> |

## 📝 Notas Operativas
- <calibración, debounce, timeouts, jumpers, voltajes>
```

───────────────────────────────────────────────────────────────────────────────
D. PROYECTO ESPECÍFICO: raíz (archivos para humanos + config)
───────────────────────────────────────────────────────────────────────────────

### README.md (PARA HUMANOS, no para LLMs)
```markdown
# <emoji> <nombre-proyecto>

> <1 línea de qué hace>

## 🎯 Características
- ✅ <feature 1>
- ✅ <feature 2>

## 🔧 Hardware
- Placa: <placa> (ver [plantillas-modulares/placas/<placa>.md](plantillas-modulares/placas/<placa>.md))
- Periféricos: <lista>

## 📦 Instalación
```bash
# 1. Clonar
git clone <repo>
cd <proyecto>

# 2. Configurar credenciales (si aplica)
cp include/secrets.h.template include/secrets.h
# Editar include/secrets.h con tus datos

# 3. Compilar y subir
pio run -t upload
```

## ⚙️ Configuración
<qué constantes editar antes de compilar>

## 🐛 Troubleshooting
| Síntoma | Causa | Solución |
|---------|-------|----------|
| <síntoma> | <causa> | <solución> |

## 📚 Referencias
- <enlaces útiles>
```

### .gitignore (SIEMPRE)
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

### secrets.h.template (SOLO si hay WiFi/MQTT/APIs/credenciales)
Detectar estilo del código y aplicar:
- C++ moderno (C++17, constexpr, namespace): `namespace secrets { inline constexpr std::array<char, N> ... }`
- C clásico/Arduino: `#define` o `const char*`

Incluir SIEMPRE:
- Comentario: "Copiar como secrets.h, NUNCA subir a Git"
- Tabla de zonas horarias comunes (si aplica GMT_OFFSET)
- Placeholders con formato claro

### .vscode/ (NO regenerar — copiar de plantillas-modulares/00-core.md)
En vez de generar, incluir instrucción:
```markdown
## 🛠️ Configuración VS Code
Copiar desde `plantillas-modulares/00-core.md`:
- `.vscode/settings.json`
- `.vscode/extensions.json`
- `.vscode/tasks.json`
- `.vscode/platformio-snippets.code-snippets`
```

═══════════════════════════════════════════════════════════════════════════════
REGLAS DE SALIDA
═══════════════════════════════════════════════════════════════════════════════

1. Genera archivos en bloques de código independientes, precedidos por su
   ruta relativa completa desde la raíz del workspace.

2. Usa el idioma detectado en los comentarios del código fuente para TODOS
   los archivos generados.

3. NUNCA dupliques información del catálogo. Usa referencias del tipo:
   `[plantillas-modulares/placas/<placa>.md](../../plantillas-modulares/placas/<placa>.md)`

4. Si falta información para una sección, omítela y marca:
   `[OMITIDO: información no disponible en el código]`

5. Al final, lista:
   - Qué archivos generaste
   - Qué archivos omitiste y por qué
   - Qué fichas de catálogo nuevas se necesitan (si aplica)

═══════════════════════════════════════════════════════════════════════════════
CÓDIGO FUENTE DEL PROYECTO
═══════════════════════════════════════════════════════════════════════════════

<pega aquí el código / adjunta el repomix / archivos del repositorio>
```

---

## 📋 Resumen de integración con tu catálogo

| Tu catálogo | Se usa en proyecto como | Acción del prompt |
|-------------|------------------------|-----------------|
| `plantillas-modulares/00-core.md` | Referencia en `.ai/PROJECT_CONTEXT.md` | NO regenerar, solo referenciar |
| `plantillas-modulares/placas/<placa>.md` | Referencia en `.ai/HARDWARE.md` | Detectar placa, mapear a archivo existente |
| `plantillas-modulares/perifericos/<modulo>.md` | Referencia en `.ai/HARDWARE.md` | Detectar periféricos, mapear a archivos existentes |
| `.vscode/*` (de 00-core.md) | Copiar al proyecto | Instrucción de copia, NO generar contenido |
| Nuevo periférico no en catálogo | Generar ficha nueva | Crear `plantillas-modulares/perifericos/<nuevo>.md` |

---

## 🚀 Flujo de uso

```
1. Tengo un nuevo proyecto → pego el prompt + código
2. El LLM detecta: placa=esp32-devkit, periféricos=hc-sr04, ssd1306
3. El LLM verifica: esp32-devkit.md existe ✅, hc-sr04.md existe ✅, ssd1306.md existe ✅
4. El LLM genera SOLO los archivos del proyecto, referenciando el catálogo
5. Si detecta un periférico nuevo (ej: BME280), genera también:
   plantillas-modulares/perifericos/bme280.md
```

---

¿Quieres que lo guarde como archivo descargable, o que lo pruebe con uno de tus 3 proyectos existentes para validar que la integración con el catálogo funciona correctamente?