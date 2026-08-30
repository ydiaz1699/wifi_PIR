# PROJECT_CONTEXT.md — proyecto-esp8266-d1minipro

## Qué es esto

Proyecto de firmware para **Wemos D1 Mini Pro** (ESP8266EX), usando PlatformIO +
Arduino framework. Próximo objetivo declarado: integración de un sensor **PIR**.

## Objetivos

- (completar según el proyecto concreto que construyas sobre esta base)

## Restricciones clave

- El D1 Mini Pro es **3.3V lógico, no tolerante a 5V** en ningún GPIO ni en A0.
  Cualquier periférico de 5V requiere el módulo de nivel lógico bidireccional
  ya documentado en `.ai/HARDWARE.md`.
- Flash de 16MB — por defecto PlatformIO puede detectar solo 4MB; ver
  `platformio.ini` para el override.

## Documentos relacionados

- `.ai/HARDWARE.md` — specs de la placa, pines, voltajes, periféricos conectados
- `.ai/SOFTWARE.md` — `platformio.ini` y dependencias específicas de este proyecto
- `.ai/SKILL.md` — reglas que la IA nunca debe romper en este proyecto
- `../shared/SOFTWARE.md` — entorno VS Code compartido con el resto del workspace
- `../shared/CODING_STYLE.md` — convenciones de código compartidas
