# SKILL.md — Reglas para la IA (proyecto-esp8266-d1minipro)

> Reglas críticas genéricas de la placa (voltajes, boot pins) están en
> [`../../boards/esp8266-d1-mini-pro.md`](../../boards/esp8266-d1-mini-pro.md).
> Esto es solo lo específico de este proyecto/firmware.

## La IA NUNCA debe:

1. Sugerir conectar un periférico de 5V directamente a un GPIO o a A0 sin pasar
   por el módulo de nivel lógico bidireccional.
2. Usar GPIO0, GPIO2 o GPIO15 como salida activa durante el arranque (boot) sin
   advertir sobre el modo de arranque que eso puede forzar.
3. Asumir que hay 4MB de flash sin revisar el `board_build.flash_size` en
   `platformio.ini` — la placa real trae 16MB pero PlatformIO puede detectar menos.
4. Reescribir `.ai/HARDWARE.md` de este proyecto usando pines o voltajes del
   Arduino Uno (son placas distintas con lógica distinta: 3.3V vs 5V).

## La IA SIEMPRE debe:

- Verificar `.ai/HARDWARE.md` antes de proponer un pinout nuevo.
- Recordar que GPIO2 (D4) tiene el LED integrado activo en LOW — evitar
  parpadeos no intencionados al usarlo para otra cosa.
- Preferir GPIO16 (D0) solo para wake de deep sleep, nunca para PWM.

## Antes de cualquier cambio de hardware

1. Leer `.ai/HARDWARE.md` completo.
2. Confirmar niveles de voltaje del periférico nuevo.
3. Actualizar la tabla de "Periféricos / Módulos Conectados" en `.ai/HARDWARE.md`.
4. Anotar la decisión en `.ai/DECISIONS.md` si implica un trade-off.
