# 🖥️ Placa: Raspberry Pi Pico (RP2040)

## 🧩 Hardware Principal

- **Placa:** Raspberry Pi Pico
- **MCU:** RP2040 (dual-core ARM Cortex-M0+, 32-bit)
- **Clock:** 133 MHz
- **RAM:** 264 KB SRAM
- **Flash:** 2 MB (externa)
- **Alimentación:** 5V USB-C
- **GPIOs:** 26 (20 GPIO + 6 ADC)

## ⚡ Niveles de Voltaje

- **GPIO:** 3.3V lógico (tolerantes a 5V)
- **ADC:** 12-bit, 0–3.3V (3 canales externos + 1 interno)

## ⚠️ Consideraciones

- **USB-C** — conexión moderna y robusta
- **PIO (Programmable I/O):** 8 state machines para protocolos personalizados
- **Dual-core:** Ejecución paralela de tareas
- **Documentación excelente** — Raspberry Pi Foundation

## 📡 `platformio.ini`

```ini
[env:pico]
platform = raspberrypi
board = pico
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

; board = adafruit_feather_rp2040

build_flags =
    -D CFG_DEBUG=2
    -D CFG_TUSB_DEBUG=0
```
