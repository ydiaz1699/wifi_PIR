# HARDWARE.md — proyecto-esp8266-d1minipro

> Specs genéricas de la placa: ver [`../../boards/esp8266-d1-mini-pro.md`](../../boards/esp8266-d1-mini-pro.md).
> Este archivo es solo el wiring y periféricos específicos de este proyecto.

## Placa base

Wemos D1 Mini Pro. Ver el catálogo para MCU, voltajes, mapeo de pines
genérico y consideraciones críticas de la placa.

## Periféricos / Módulos conectados en ESTE proyecto

| Módulo | Protocolo | Nivel Lógico | Pines Usados | Notas |
| --- | --- | --- | --- | --- |
| Módulo de nivel lógico bidireccional | — | 3.3V ↔ 5V | — | 4 canales, basado en MOSFETs |
| *(agregar sensor PIR aquí cuando se conecte)* | | | | |

## Pines reservados por este proyecto

| Pin/GPIO | Uso en este proyecto |
| --- | --- |
| GPIO4 (D2) | Reservado para I2C SDA (si se agrega LCD u otro sensor I2C) |
| GPIO5 (D1) | Reservado para I2C SCL |
| *(agregar pin del sensor PIR)* | |

## Consideraciones específicas de este proyecto

- Ninguna adicional a las de la placa por ahora.
