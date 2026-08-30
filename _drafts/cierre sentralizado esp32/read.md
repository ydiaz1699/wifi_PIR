Ahí te referís a otra categoría de documentación: no es "hardware docs" sino **documentación para agentes de IA** (instrucciones de contexto para que un LLM/copiloto trabaje bien en el proyecto).

## Cómo se llaman

- **`copilot-instructions.md`** → se llama **"repository custom instructions"** o **"AI agent instructions file"**. Es una convención que popularizó GitHub Copilot (se coloca en `.github/copilot-instructions.md`) pero que ya usan también Cursor (`.cursorrules`), Continue, etc. Define reglas de estilo, contexto del dominio, convenciones de nombres, y restricciones que el asistente debe respetar automáticamente en cada sugerencia dentro de ese repo.

- **`SKILL.md`** → es el formato de **"Agent Skill"** de Anthropic (el mismo mecanismo que usa Claude Code y Claude.ai — de hecho es literalmente la misma estructura que tenés en `/mnt/skills/` en este entorno). Define: cuándo se debe "activar" la skill (trigger/descripción), un flujo de trabajo paso a paso, decisiones clave del dominio, y criterios de salida esperados. Es más estructurado y "accionable" que las instrucciones de Copilot — piensa en él como un mini-procedimiento operativo para una tarea recurrente.

Juntos, esto se conoce genéricamente como **"AI agent context files"** o **"project-level AI configuration"** — documentación que no es para humanos sino para que el propio asistente de código se auto-configure con el contexto y las reglas del proyecto.

## Diferencia clave entre ambos

| Archivo | Para qué sirve | Alcance |
|---|---|---|
| `copilot-instructions.md` | Reglas generales de estilo/comportamiento para *cualquier* tarea en el repo | Todo el repositorio |
| `SKILL.md` | Procedimiento específico para *una* tarea recurrente concreta (ej. "generar un PKE con estos parámetros") | Una tarea/dominio puntual |

## Prompt reutilizable para generarlosCon esto ya tenés dos plantillas complementarias para reusar en cualquier proyecto nuevo:

1. **`prompt-docs-hardware.md`** — genera `docs/notas.md` + `docs/conexiones.drawio.svg` (documentación de hardware, para humanos).
2. **`prompt-contexto-agentes-ia.md`** — genera `copilot-instructions.md` + `SKILL.md` (documentación de contexto, para que la IA se autoconfigure con las reglas del proyecto).

Ambas siguen la misma filosofía: "leé el código real, no inventes, y si algo es ambiguo preguntame antes de asumir".