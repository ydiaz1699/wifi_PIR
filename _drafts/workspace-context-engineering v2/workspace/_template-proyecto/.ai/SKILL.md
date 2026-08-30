# SKILL.md — Reglas para la IA (<nombre-proyecto>)

> Reglas críticas de la PLACA ya están en `../../boards/<placa>.md` —
> no las dupliques. Esto es solo lo específico de este proyecto/firmware.

## La IA NUNCA debe:

1. <regla crítica específica de este firmware>

## La IA SIEMPRE debe:

- Revisar `../../boards/<placa>.md` antes de proponer un pinout nuevo.
- Revisar `.ai/HARDWARE.md` de este proyecto antes de reasignar un pin ya en uso.

## Antes de cualquier cambio de hardware

1. Leer `../../boards/<placa>.md` (specs genéricas) y `.ai/HARDWARE.md` (wiring actual).
2. Confirmar niveles de voltaje del periférico nuevo.
3. Actualizar la tabla de "Periféricos" en `.ai/HARDWARE.md`.
4. Anotar la decisión en `.ai/DECISIONS.md` si implica un trade-off.
