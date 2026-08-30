# 🖥️ Placa: Arduino Uno (ATmega328P)

## 🧩 Hardware Principal

- **Placa:** Arduino Uno
- **MCU:** ATmega328P (AVR, 8-bit)
- **Clock:** 16 MHz
- **RAM:** 2 KB SRAM
- **Flash:** 32 KB (0.5 KB bootloader)
- **EEPROM:** 1 KB
- **Alimentación:** 5V USB / 7–12V barrel jack
- **Lógica:** 5V (GPIO tolerantes a 5V)

## 📡 Conectividad

- **USB-UART:** ATmega16U2
- **Sin WiFi** — comunicación solo serial/SPI/I2C/UART

## 🔌 Periféricos / Módulos Conectados

| Módulo | Protocolo | Nivel Lógico | Pines Usados | Notas |
| --- | --- | --- | --- | --- |
| Receptor RF 433MHz | Digital/INT | 5V | D2 (INT0) | RCSwitch, códigos 24-bit |
| *(agregar)* |  |  |  |  |

## ⚡ Niveles de Voltaje

- **GPIO:** 5V lógico — tolerantes a 5V
- **ADC:** 10-bit, 0–5V (6 canales: A0–A5)
- **Corriente por pin:** máximo 40 mA, total 200 mA
- **Interfaz con 3.3V:** usar módulo de nivel lógico bidireccional

## 🗺️ Mapeo de Pines

| Función | Pin | Uso / Notas |
| --- | --- | --- |
| UART TX | 1 | Monitor Serial (USB) |
| UART RX | 0 | Monitor Serial (USB) — no usar mientras programa |
| INT0 | 2 | Interrupción externa 0 — RF 433MHz (RCSwitch) |
| INT1 | 3 | Interrupción externa 1 / PWM |
| PWM | 3,5,6,9,10,11 | 8-bit PWM |
| SPI SCK | 13 | También LED integrado (active HIGH) |
| SPI MISO | 12 |  |
| SPI MOSI | 11 |  |
| SPI CS | 10 |  |
| I2C SDA | A4 |  |
| I2C SCL | A5 |  |
| ADC | A0–A5 | 0–5V, 10-bit |

## ⚠️ Consideraciones Críticas

- **RAM limitada (2KB):** Usar macro `F()` para strings en Flash — libera SRAM
- **Pin 0 (RX):** No conectar nada mientras se programa via USB
- **Pin 13:** Tiene LED integrado y resistencia — no usar como entrada de señal
- **INT0 (D2) e INT1 (D3):** Únicos pines con interrupción externa en Uno
- **Corriente total:** No superar 200 mA en todos los GPIO juntos
- **Sin WiFi:** Para conectividad agregar módulo ESP8266 (AT commands) o NRF24L01
- **`F()` macro:** Crítica en proyectos con muchos strings — ATmega328P tiene solo 2KB RAM

## 📡 `platformio.ini`

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0       ; Linux/Mac
; monitor_port = COM3             ; Windows

; Librerías del proyecto
; lib_deps =
;     sui77/rc-switch@^2.6.4     ; RF 433MHz
```
