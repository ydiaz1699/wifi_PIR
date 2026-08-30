# Workspace Multi-Placa — Context Engineering + Hardware

Fusiona tu plantilla de hardware con la metodología de
**[Context Engineering V2](https://github.com/ydiaz1699/Context_Engineering_V2)**,
más una capa para que esto **escale** cuando tu biblioteca de placas crezca.

## El problema que esto resuelve

La primera versión de este workspace tenía un `HARDWARE.md` completo (specs de
la placa + wiring del proyecto, todo junto) dentro de cada carpeta de proyecto.
Con dos placas ya se veía la repetición; con una tercera placa (o con dos
proyectos sobre la misma placa) sería puro copy-paste desincronizándose.

**La causa:** las specs de una placa (MCU, voltajes, pinout genérico) son
*dato de la placa*, no *dato del proyecto*. Estabas guardando lo mismo en dos
lugares distintos.

## La solución: 3 capas

```
workspace/
├── boards/                     ← CAPA 1: catálogo, una ficha por placa física
│   ├── README.md
│   ├── _template-board.md      ← checklist para agregar una placa nueva
│   ├── esp8266-d1-mini-pro.md
│   ├── esp8266-nodemcu-v2.md
│   └── arduino-uno.md
│
├── shared/                     ← CAPA 2: lo que NO depende de la placa
│   ├── SOFTWARE.md             (VS Code, extensiones — igual para todos)
│   └── CODING_STYLE.md         (convención de tags TODO/FIXME/etc.)
│
├── _template-proyecto/         ← plantilla para no reescribir la forma de cada proyecto
│   ├── README.md               (cómo usarla)
│   └── .ai/                    (PROJECT_CONTEXT, HARDWARE, SOFTWARE, SKILL,
│                                 TASKS, CHANGELOG, DECISIONS, ROADMAP — con
│                                 placeholders `<nombre-proyecto>` / `<placa>`)
│
└── proyecto-<nombre>/          ← CAPA 3: un proyecto = wiring + firmware + gestión
    ├── .ai/
    │   ├── PROJECT_CONTEXT.md  → referencia a boards/<placa>.md
    │   ├── HARDWARE.md         → SOLO wiring/periféricos de ESTE proyecto
    │   ├── SOFTWARE.md         → platformio.ini + libs de este proyecto
    │   ├── SKILL.md            → reglas específicas de este firmware
    │   ├── ARCHITECTURE.md     → (opcional) si hay FSM o flujos complejos
    │   ├── TASKS.md / ROADMAP.md / CHANGELOG.md / DECISIONS.md
    ├── src/, include/, platformio.ini, .vscode/
```

**Regla simple:** si la información es verdad para la placa sin importar qué
proyecto la use → va en `boards/`. Si depende de qué conectaste y cómo →
va en el `HARDWARE.md` del proyecto.

## Proyectos actuales

| Proyecto | Placa | Estado |
| --- | --- | --- |
| [`proyecto-esp8266-d1minipro/`](./proyecto-esp8266-d1minipro/) | [D1 Mini Pro](./boards/esp8266-d1-mini-pro.md) | En desarrollo (sensor PIR pendiente) |
| [`proyecto-arduino-uno/`](./proyecto-arduino-uno/) | [Arduino Uno](./boards/arduino-uno.md) | En desarrollo (RF 433MHz) |
| [`proyecto-reloj-ntp-nodemcu/`](./proyecto-reloj-ntp-nodemcu/) | [NodeMCU v2](./boards/esp8266-nodemcu-v2.md) | **Producción v1.0.0** — reloj LCD + NTP |

## Cuándo tu biblioteca de placas crezca

1. **¿Placa nueva o proyecto nuevo sobre placa existente?**
   - Placa existente → solo copia `_template-proyecto/`, no toques `boards/`.
   - Placa nueva → sigue `boards/_template-board.md` primero.
2. **Nunca copies una tabla de pines/voltajes dentro de un proyecto.** Si te
   encuentras copiando algo de `boards/*.md` dentro de `proyecto-*/.ai/HARDWARE.md`,
   es señal de que deberías estar linkeando en vez de copiando.
3. **Un `HARDWARE.md` de proyecto sano es corto** — dos tablas (pines que usa,
   periféricos conectados) y un link. Si crece mucho, probablemente metiste
   ahí specs genéricas que pertenecen a `boards/`.

## Cómo usarlo con una IA

```
Lee boards/esp8266-nodemcu-v2.md, proyecto-reloj-ntp-nodemcu/.ai/PROJECT_CONTEXT.md,
HARDWARE.md, SKILL.md y ARCHITECTURE.md antes de proponer cualquier cambio.
```

Contexto quirúrgico: la IA carga la placa + el proyecto puntual, nunca los otros
dos proyectos ni sus placas.
