# Catálogo de Placas

Ficha técnica **por modelo físico de placa**, reutilizable entre proyectos.
Un proyecto nunca copia estas specs — las referencia y agrega solo su wiring
específico en su propio `.ai/HARDWARE.md`.

| Placa | MCU | Lógica | Archivo | Usada en |
| --- | --- | --- | --- | --- |
| Wemos D1 Mini Pro | ESP8266EX | 3.3V | [esp8266-d1-mini-pro.md](./esp8266-d1-mini-pro.md) | `proyecto-esp8266-d1minipro/` |
| NodeMCU v2 | ESP8266-12E | 3.3V | [esp8266-nodemcu-v2.md](./esp8266-nodemcu-v2.md) | `proyecto-reloj-ntp-nodemcu/` |
| Arduino Uno | ATmega328P | 5V | [arduino-uno.md](./arduino-uno.md) | `proyecto-arduino-uno/` |

## Cuándo agregar una placa nueva aquí

Agrega un archivo nuevo **solo si es un modelo físico distinto** — no un proyecto
distinto. Dos proyectos con la misma placa física comparten el mismo archivo aquí.

Usa [`_template-board.md`](./_template-board.md) como checklist paso a paso.

## Por qué existe esto separado de cada proyecto

Antes, cada proyecto repetía la tabla de pines y voltajes de su placa. Si el D1
Mini Pro y el Arduino Uno hubieran sido la misma placa, habríamos tenido dos
copias idénticas de lo mismo, y si algo cambiaba (ej. descubres que un pin no
sirve para algo), había que recordar actualizarlo en cada proyecto. Ahora es
una sola fuente de verdad por placa física.
