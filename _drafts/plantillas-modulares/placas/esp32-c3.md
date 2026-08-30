# 🖥️ Placa: ESP32-C3-DevKitM-1

## 🧩 Hardware Principal

- **Placa:** ESP32-C3-DevKitM-1
- **MCU:** ESP32-C3 (single-core RISC-V, 32-bit)
- **Clock:** 160 MHz
- **RAM:** 400 KB SRAM
- **Flash:** 4 MB
- **Alimentación:** 5V USB-C → 3.3V regulado
- **WiFi + Bluetooth:** 802.11 b/g/n + BLE 5.0

## 🗺️ Mapeo de Pines (parcial)

| Función | GPIO | Notas |
| --- | --- | --- |
| UART0 TX | GPIO21 | Debug (USB) |
| UART0 RX | GPIO20 | Debug (USB) |
| I2C SDA | GPIO8 |  |
| I2C SCL | GPIO9 |  |
| SPI SCK | GPIO4 |  |
| Built-in LED | GPIO8 | Comparte con I2C SDA en algunas boards |

## ⚡ Niveles de Voltaje

- **GPIO:** 3.3V lógico
- **ADC:** 12-bit, 0–3.3V (5 canales)

## ⚠️ Consideraciones Críticas

- **RISC-V single-core:** Menor throughput que ESP32 dual-core, pero más eficiente energéticamente
- **USB nativo:** CDC integrado, no requiere UART bridge
- **GPIO limitados:** 22 GPIOs vs 34 del ESP32 clásico
- **Consumo:** ~90 mA activo, deep sleep ~5 µA
- **Precio:** Significativamente más barato que ESP32-S3

## 📡 `platformio.ini`

```ini
[env:esp32c3_devkit]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D CORE_DEBUG_LEVEL=3

monitor_filters = esp32_exception_decoder
```
