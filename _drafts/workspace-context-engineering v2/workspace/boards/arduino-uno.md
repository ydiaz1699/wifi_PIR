# Board: Arduino Uno

> Ficha técnica reutilizable. NO poner aquí periféricos de un proyecto concreto
> (eso va en el `HARDWARE.md` del proyecto, en la sección "Wiring de este proyecto").
> Si algo de esto cambia, cambia para **todos** los proyectos que usan esta placa.

## Identidad

- **MCU:** ATmega328P (AVR, 8-bit)
- **Clock:** 16 MHz
- **RAM:** 2 KB SRAM
- **Flash:** 32 KB (0.5 KB usado por bootloader)
- **EEPROM:** 1 KB
- **Alimentación:** 5V USB → regulado interno / 7–12V barrel jack

## Conectividad

- **USB-UART:** ATmega16U2
- **Sin WiFi** — comunicación solo serial/SPI/I2C/UART

## Niveles de Voltaje

- **Lógico:** 5V — GPIO tolerantes a 5V
- **ADC:** 10-bit, 0–5V (6 canales: A0–A5)
- **Corriente por pin:** máximo 40 mA, total 200 mA
- **Interfaz con 3.3V:** usar módulo de nivel lógico bidireccional

## Mapeo de Pines (genérico de la placa)

| Función | Pin | Uso / Notas |
| --- | --- | --- |
| UART TX | 1 | Monitor Serial (USB) |
| UART RX | 0 | Monitor Serial (USB) — no usar mientras programa |
| INT0 | 2 | Interrupción externa 0 |
| INT1 | 3 | Interrupción externa 1 / PWM |
| PWM | 3,5,6,9,10,11 | 8-bit PWM |
| SPI SCK | 13 | También LED integrado (active HIGH) |
| SPI MISO | 12 | |
| SPI MOSI | 11 | |
| SPI CS | 10 | |
| I2C SDA | A4 | |
| I2C SCL | A5 | |
| ADC | A0–A5 | 0–5V, 10-bit |

## Consideraciones críticas de esta placa (aplican a todo proyecto que la use)

- **RAM limitada (2KB):** usar macro `F()` para strings — crítico, se agota rápido
- **Pin 0 (RX):** no conectar nada mientras se programa vía USB
- **Pin 13:** tiene LED + resistencia integrados — no usar como entrada de señal
- **INT0 (D2) e INT1 (D3):** únicos pines con interrupción externa
- **Corriente total:** no superar 200 mA en todos los GPIO juntos
- **Sin WiFi:** requiere módulo externo (ESP8266 AT commands o NRF24L01)

## `platformio.ini` — bloque `[env:]` de referencia

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
monitor_speed = 115200
```

## Proyectos que usan esta placa

- `../proyecto-arduino-uno/`
