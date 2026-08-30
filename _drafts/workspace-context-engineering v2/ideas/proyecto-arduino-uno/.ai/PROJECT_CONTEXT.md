# PROJECT_CONTEXT.md — proyecto-arduino-uno

## Qué es esto

Proyecto de firmware para **Arduino Uno** (ATmega328P), usando PlatformIO +
Arduino framework. Incluye recepción RF 433MHz (RCSwitch) como periférico ya
conectado.

## Objetivos

- (completar según el proyecto concreto que construyas sobre esta base)

## Restricciones clave

- Ver [`../../boards/arduino-uno.md`](../../boards/arduino-uno.md) para RAM,
  voltajes y pines críticos de la placa.
- Sin WiFi nativo — cualquier conectividad requiere un módulo externo
  (ESP8266 por AT commands o NRF24L01), no contemplado aún.

## Documentos relacionados

- `../../boards/arduino-uno.md` — ficha técnica de la placa (fuente de verdad)
- `.ai/HARDWARE.md` — wiring y periféricos específicos de ESTE proyecto
- `.ai/SOFTWARE.md` — `platformio.ini` y dependencias específicas de este proyecto
- `.ai/SKILL.md` — reglas que la IA nunca debe romper en este proyecto
- `../shared/SOFTWARE.md` — entorno VS Code compartido con el resto del workspace
- `../shared/CODING_STYLE.md` — convenciones de código compartidas
