# 🖥️ Placa: STM32F103C8T6 "Blue Pill"

## 🧩 Hardware Principal

- **Placa:** STM32F103C8T6 "Blue Pill"
- **MCU:** ARM Cortex-M3 (32-bit)
- **Clock:** 72 MHz
- **RAM:** 20 KB
- **Flash:** 64 KB
- **Alimentación:** 3.3V / 5V USB
- **ADC:** 12-bit, 16 canales

## ⚠️ Consideraciones

- **Programación:** Requiere ST-Link o bootloader UART
- **3.3V lógico** — NO tolerante a 5V
- **Más potente** que AVR — ideal para DSP o control avanzado
- **Precio bajo** — excelente relación calidad/precio

## 📡 `platformio.ini`

```ini
[env:bluepill_f103c8]
platform = ststm32
board = bluepill_f103c8
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

build_flags =
    -D DEBUG=1
    -D SERIAL_USB
```
