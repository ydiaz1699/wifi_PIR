# HARDWARE.md — proyecto-reloj-ntp-nodemcu

> Specs genéricas de la placa: ver [`../../boards/esp8266-nodemcu-v2.md`](../../boards/esp8266-nodemcu-v2.md).
> Este archivo es solo el wiring y periféricos específicos de este proyecto.

## Placa base

NodeMCU v2 (ESP8266-12E). Ver el catálogo para MCU, voltajes, mapeo de pines
genérico y consideraciones críticas de la placa.

## Periféricos / Módulos conectados en ESTE proyecto

| Módulo | Protocolo | Nivel Lógico | Pines Usados | Notas |
| --- | --- | --- | --- | --- |
| LCD 16x2 con módulo I2C PCF8574T/A | I2C | VCC 5V, bus lógico 3.3V | SDA=GPIO4(D2), SCL=GPIO5(D1) | Dirección 0x27 (alternativas: 0x26, 0x25, 0x24). Consumo ~50mA @5V |

## Pines reservados por este proyecto

| Pin/GPIO | Uso en este proyecto |
| --- | --- |
| GPIO4 (D2) | I2C SDA — bus hacia el LCD |
| GPIO5 (D1) | I2C SCL — bus hacia el LCD |

No se usan más GPIO. El resto está disponible para expansiones futuras.

## Alimentación y consumo (medido/estimado para este proyecto)

| Estado | Consumo |
| --- | --- |
| WiFi activo | ~150–200 mA |
| Idle (WiFi conectado, sin actividad) | ~50–80 mA |
| Sin WiFi | ~30 mA |

- **Cable I2C:** 4 pines (GND, VCC 5V, SDA, SCL), máximo 1 metro recomendado.
- **Pull-ups I2C:** típicamente incluidas en el módulo del LCD, no se agregaron externas.

## Consideraciones específicas de este proyecto

- El módulo LCD I2C acepta 5V en VCC aunque el bus lógico del NodeMCU corre a
  3.3V — esto **es específico de este módulo PCF8574T/A**, no una regla general
  de la placa. Si cambias de módulo LCD, verificar su datasheet antes de asumir
  que sigue aceptando 5V.
- Dirección I2C configurable en software (`include/hw.h` → `LCD_ADDR`) si el
  LCD físico usa una dirección alternativa (0x26/0x25/0x24).
