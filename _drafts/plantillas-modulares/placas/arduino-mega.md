# 🖥️ Placa: Arduino Mega 2560

## 🧩 Hardware Principal

- **Placa:** Arduino Mega 2560
- **MCU:** ATmega2560
- **Clock:** 16 MHz
- **RAM:** 8 KB SRAM
- **Flash:** 256 KB
- **GPIOs:** 54 digitales (15 PWM) + 16 analógicos

## ⚠️ Consideraciones

- **Más memoria** que Uno/Nano — ideal para proyectos grandes
- **Más GPIOs** — 54 pines digitales
- **4 UARTs hardware** — Serial, Serial1, Serial2, Serial3
- **Mayor consumo** — considerar alimentación externa

## 📡 `platformio.ini`

```ini
[env:megaatmega2560]
platform = atmelavr
board = megaatmega2560
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0
```
