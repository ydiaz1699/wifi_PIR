# SKILL.md — Reglas para la IA (proyecto-arduino-uno)

> Reglas críticas genéricas de la placa (voltajes, pines especiales) están en
> [`../../boards/arduino-uno.md`](../../boards/arduino-uno.md).
> Esto es solo lo específico de este proyecto/firmware.

## La IA NUNCA debe:

1. Escribir strings literales largos sin envolverlos en `F()` — la SRAM es de
   solo 2KB y se agota rápido con logs de debug o mensajes de UI.
2. Sugerir usar el pin 13 como entrada de señal (tiene LED + resistencia integrados).
3. Reasignar D2 o D3 a otra función sin revisar antes si el RF 433MHz (RCSwitch)
   sigue necesitando la interrupción externa en D2.
4. Asumir GPIO tolerantes a 3.3V sin módulo de nivel lógico — el Uno es 5V lógico,
   distinto al D1 Mini Pro (3.3V). No mezclar reglas entre ambos `HARDWARE.md`.

## La IA SIEMPRE debe:

- Verificar `.ai/HARDWARE.md` antes de proponer un pinout nuevo.
- Recordar que no hay WiFi nativo — cualquier feature de conectividad requiere
  hardware adicional no incluido todavía.
- Vigilar el presupuesto de 200mA total de corriente en GPIO al sugerir varios
  actuadores simultáneos.

## Antes de cualquier cambio de hardware

1. Leer `.ai/HARDWARE.md` completo.
2. Confirmar que no hay conflicto con D2 (RF 433MHz).
3. Actualizar la tabla de "Periféricos / Módulos Conectados" en `.ai/HARDWARE.md`.
4. Anotar la decisión en `.ai/DECISIONS.md` si implica un trade-off.
