# 🖥️ Placa: ESP32 DevKit v1

## 🧩 Hardware Principal

- **Placa:** ESP32 DevKit v1
- **MCU:** ESP32-D0WDQ6 (dual-core Xtensa LX6, 32-bit)
- **Clock:** 240 MHz
- **RAM:** 520 KB SRAM
- **Flash:** 4 MB
- **Alimentación:** 5V USB → 3.3V regulado
- **WiFi + Bluetooth:** 802.11 b/g/n + BLE 4.2

## 🗺️ Mapeo de Pines (parcial)

| Función | GPIO | Notas |
| --- | --- | --- |
| UART0 TX | GPIO1 | Debug |
| UART0 RX | GPIO3 | Debug |
| I2C SDA | GPIO21 |  |
| I2C SCL | GPIO22 |  |
| SPI SCK | GPIO18 | VSPI |
| Built-in LED | GPIO2 |  |

## ⚡ Niveles de Voltaje

- **GPIO:** 3.3V lógico (tolerantes a 5V en algunos pines — verificar datasheet)
- **ADC:** 12-bit, 0–3.3V (18 canales)
- **DAC:** 2 canales (GPIO25, GPIO26)

## ⚠️ Consideraciones Críticas

- **GPIO 6-11:** Reservados para flash interna — **NO USAR**
- **GPIO 34-39:** Solo entrada (sin pull-up/down)
- **GPIO 36-39:** ADC1 (mejor para WiFi simultáneo)
- **GPIO 25-26:** DAC (solo salida)
- **Strapping pins (0, 2, 12, 15):** Configuran modo de boot (ver tabla de strapping pins más abajo)
- **Consumo:** Mayor que ESP8266 — considerar deep sleep

## 🔧 Strapping Pins (boot)

| Pin | Función boot | LOW | HIGH | Notas |
|-----|-------------|-----|------|-------|
| GPIO0 | Boot mode | Flash boot | Download boot | Pull-up interno. Evitar carga capacitiva alta. |
| GPIO2 | Boot mode | — | Must be LOW for download | Conectado a LED en muchas boards |
| GPIO5 | VDD_SDIO voltage | 1.8V | 3.3V (flash) | Afecta flash externa. Dejar flotar = 3.3V |
| GPIO12 | VDD_SDIO voltage | 1.8V | 3.3V | MTDI strap |
| GPIO15 | Boot log silencing | UART0 log output | Silent boot | Pull-down interno. Evitar pull-up fuerte. |

> **Regla de oro:** Nunca conectar cargas que puedan mantener strapping pins
> en estado incorrecto durante el reset. Usar buffers tri-state o switches
> para aislar periféricos de estos pines si es necesario.

## 📡 `platformio.ini`

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0

build_flags =
    -D CORE_DEBUG_LEVEL=3
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

monitor_filters = esp32_exception_decoder
```
