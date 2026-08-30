# PROJECT_CONTEXT.md — proyecto-arduino-uno

## Qué es esto

Proyecto de firmware para **Arduino Uno** (ATmega328P), usando PlatformIO +
Arduino framework. Incluye recepción RF 433MHz (RCSwitch) como periférico ya
conectado.

## Objetivos

- (completar según el proyecto concreto que construyas sobre esta base)

## Restricciones clave

- RAM muy limitada: **2KB SRAM total**. Todo string literal largo debe usar
  la macro `F()` para vivir en Flash en vez de SRAM (ver `.ai/HARDWARE.md`).
- Sin WiFi nativo — cualquier conectividad requiere un módulo externo
  (ESP8266 por AT commands o NRF24L01), no contemplado aún.

## Documentos relacionados

- `.ai/HARDWARE.md` — specs de la placa, pines, voltajes, periféricos conectados
- `.ai/SOFTWARE.md` — `platformio.ini` y dependencias específicas de este proyecto
- `.ai/SKILL.md` — reglas que la IA nunca debe romper en este proyecto
- `../shared/SOFTWARE.md` — entorno VS Code compartido con el resto del workspace
- `../shared/CODING_STYLE.md` — convenciones de código compartidas
