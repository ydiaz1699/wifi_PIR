# 🖥️ Placa: NodeMCU v3 (ESP8266)

## 🧩 Hardware Principal

- **Placa:** NodeMCU v3 (LOLIN)
- **MCU:** ESP-8266EX
- **Clock:** 80 MHz
- **RAM:** ~80 KB
- **Flash:** 4 MB
- **Alimentación:** 5V USB (Micro-USB)
- **Dimensiones:** 54 mm × 31 mm

## 🗺️ Mapeo de Pines

| Función | GPIO | Pin Label |
| --- | --- | --- |
| UART0 TX | GPIO1 | D10/TX |
| UART0 RX | GPIO3 | D9/RX |
| I2C SDA | GPIO4 | D2/SDA |
| I2C SCL | GPIO5 | D1/SCL |
| SPI CS | GPIO15 | D8/CS |
| Built-in LED | GPIO16 | D0/LED |

## ⚠️ Consideraciones

- GPIOs **NO tolerantes a 5V**
- USB Micro-B (menos robusto que USB-C)
- Flash 4 MB (suficiente para la mayoría de proyectos)

## 📡 `platformio.ini`

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0
```
