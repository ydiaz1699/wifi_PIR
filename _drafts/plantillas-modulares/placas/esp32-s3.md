# 🖥️ Placa: ESP32-S3-DevKitC-1

## 🧩 Hardware Principal

- **Placa:** ESP32-S3-DevKitC-1
- **MCU:** ESP32-S3 (dual-core Xtensa LX7, 32-bit)
- **Clock:** 240 MHz
- **RAM:** 512 KB SRAM + 8 MB PSRAM (QSPI/OPI)
- **Flash:** 8 MB
- **Alimentación:** 5V USB-C → 3.3V regulado
- **WiFi + Bluetooth:** 802.11 b/g/n + BLE 5.0
- **Vectorial:** SIMD/vector instructions (AI/ML edge)

## 🗺️ Mapeo de Pines (parcial)

| Función | GPIO | Notas |
| --- | --- | --- |
| UART0 TX | GPIO43 | Debug (USB-JTAG) |
| UART0 RX | GPIO44 | Debug (USB-JTAG) |
| I2C SDA | GPIO8 |  |
| I2C SCL | GPIO9 |  |
| SPI SCK | GPIO12 |  |
| Built-in LED | GPIO2 | RGB LED en algunas versiones |

## ⚡ Niveles de Voltaje

- **GPIO:** 3.3V lógico
- **ADC:** 12-bit, 0–3.3V (20 canales)
- **USB:** USB-OTG nativo (USB Serial/JTAG)

## ⚠️ Consideraciones Críticas

- **USB Serial/JTAG integrado:** No requiere UART externo para debug
- **PSRAM 8MB:** Ideal para buffers grandes, cámara, o modelos TinyML
- **GPIO 26-32:** Reservados para flash/PSRAM en modo OPI — **NO USAR**
- **Consumo:** ~150 mA activo, deep sleep ~7 µA
- **AI/Vectorial:** Instrucciones SIMD aceleran operaciones de señal

## 🔧 Partition Table (16MB Flash)

### `partitions_16MB.csv`

```csv
# Name,   Type, SubType, Offset,  Size,    Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x200000,
app0,     app,  ota_0,   0x210000,0x200000,
app1,     app,  ota_1,   0x410000,0x200000,
spiffs,   data, spiffs,  0x610000,0x9F0000,
```

## 📡 `platformio.ini`

```ini
[env:esp32s3_devkit]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D CORE_DEBUG_LEVEL=3
    -D CONFIG_SPIRAM_USE=1

; PSRAM habilitada
board_build.arduino.memory_type = qio_opi

; Si usás la partition table de 16MB:
; board_build.partitions = partitions_16MB.csv
; board_build.flash_size = 16MB

monitor_filters = esp32_exception_decoder
```
