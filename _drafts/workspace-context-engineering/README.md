# Workspace Multi-Plataforma — Context Engineering + Hardware

Este workspace fusiona tu **plantilla de hardware** (D1 Mini Pro + Arduino Uno) con la
metodología de **[Context Engineering V2](https://github.com/ydiaz1699/Context_Engineering_V2)**.

## Qué cambió respecto a tu plantilla original

Antes tenías un único documento gigante con todo mezclado: hardware, entorno de VS Code,
convenciones de código y estructura de carpetas. Ahora cada tipo de información vive en
su propio archivo, siguiendo la convención `.ai/` del framework:

| Antes (un solo doc) | Ahora (separado) |
|---|---|
| Todo en un README | `PROJECT_CONTEXT.md` → qué es el proyecto |
| Specs de la placa mezcladas con VS Code | `HARDWARE.md` → **solo** hardware (MCU, pines, voltajes, periféricos) |
| Extensiones y settings.json | `SOFTWARE.md` → entorno, IDE, dependencias |
| Reglas del LED, boot pins, etc. | `SKILL.md` → qué NO debe hacer la IA nunca |
| — | `TASKS.md`, `CHANGELOG.md`, `DECISIONS.md`, `ROADMAP.md` → gestión viva del proyecto |

**Por qué importa:** cuando le pides a una IA "lee `.ai/HARDWARE.md`", no le haces tragar
también el JSON de `settings.json` de VS Code. Cada archivo es contexto quirúrgico —
la IA carga solo lo que necesita para la tarea, y tú puedes actualizar el hardware sin
tocar la config del editor (o viceversa).

## Estructura

```
workspace/
├── shared/                              ← Todo lo que NO es específico de una placa
│   ├── SOFTWARE.md                      ← Entorno VS Code + extensiones (compartido)
│   └── CODING_STYLE.md                  ← Convención de tags (TODO/FIXME/etc.)
│
├── proyecto-esp8266-d1minipro/
│   ├── .ai/
│   │   ├── PROJECT_CONTEXT.md
│   │   ├── HARDWARE.md                  ← SOLO specs del D1 Mini Pro
│   │   ├── SOFTWARE.md                  ← platformio.ini de este proyecto
│   │   ├── SKILL.md                     ← reglas críticas (nunca 5V en GPIO, etc.)
│   │   ├── TASKS.md
│   │   ├── CHANGELOG.md
│   │   ├── DECISIONS.md
│   │   └── ROADMAP.md
│   ├── src/main.cpp
│   ├── platformio.ini
│   └── .vscode/
│       ├── settings.json
│       └── extensions.json
│
└── proyecto-arduino-uno/
    ├── .ai/
    │   ├── PROJECT_CONTEXT.md
    │   ├── HARDWARE.md                  ← SOLO specs del Arduino Uno + RF 433MHz
    │   ├── SOFTWARE.md
    │   ├── SKILL.md
    │   ├── TASKS.md
    │   ├── CHANGELOG.md
    │   ├── DECISIONS.md
    │   └── ROADMAP.md
    ├── src/main.cpp
    ├── platformio.ini
    └── .vscode/
        ├── settings.json
        └── extensions.json
```

## Cómo usarlo con una IA

Al empezar una conversación sobre un proyecto específico:

```
Lee proyecto-esp8266-d1minipro/.ai/PROJECT_CONTEXT.md, HARDWARE.md y SKILL.md
antes de proponer cualquier cambio de código o pines.
```

Esto le da a la IA justo lo que necesita — sin la config de VS Code, sin las specs del
Arduino Uno, sin ruido — igual que recomienda `synapse_prompt.txt` del repo original.

## Si agregas una tercera plataforma (ej. ESP32)

1. Copia una de las carpetas `proyecto-*/` como base.
2. Reescribe `HARDWARE.md` desde cero para la placa nueva (no reutilices pines de otra).
3. `SOFTWARE.md` de ese proyecto solo referencia `../shared/SOFTWARE.md` para lo común
   y agrega su propio `platformio.ini`.
4. Actualiza `SKILL.md` con los riesgos eléctricos/de boot específicos de esa placa.
