# HARDWARE.md — proyecto-arduino-uno

> Specs genéricas de la placa: ver [`../../boards/arduino-uno.md`](../../boards/arduino-uno.md).
> Este archivo es solo el wiring y periféricos específicos de este proyecto.

## Placa base

Arduino Uno. Ver el catálogo para MCU, voltajes, mapeo de pines genérico
y consideraciones críticas de la placa.

## Periféricos / Módulos conectados en ESTE proyecto

| Módulo | Protocolo | Nivel Lógico | Pines Usados | Notas |
| --- | --- | --- | --- | --- |
| Receptor RF 433MHz | Digital/INT | 5V | D2 (INT0) | RCSwitch, códigos 24-bit |
| *(agregar)* | | | | |

## Pines reservados por este proyecto

| Pin/GPIO | Uso en este proyecto |
| --- | --- |
| D2 (INT0) | Receptor RF 433MHz (RCSwitch) — no reasignar |

## Consideraciones específicas de este proyecto

- D2 está comprometido por RCSwitch — cualquier feature nueva que necesite
  interrupción externa debe usar D3 (INT1) en su lugar.
