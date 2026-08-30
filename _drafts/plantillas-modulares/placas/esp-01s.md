# 🖥️ Placa: ESP-01S (ESP8266)

## 🧩 Hardware Principal

- **Módulo:** ESP-01S
- **MCU:** ESP-8266EX
- **Flash:** 1 MB
- **GPIOs disponibles:** 2 (GPIO0, GPIO2)
- **Alimentación:** 3.3V (requiere regulador externo si se usa 5V)

## ⚠️ Consideraciones Críticas

- **Solo 2 GPIOs** útiles (GPIO0 y GPIO2)
- **Requiere adaptador USB-UART** para programación
- **Necesita 500mA** en picos de WiFi — regulador robusto
- **Muy limitado** — solo para proyectos simples o como slave

## 📡 `platformio.ini`

```ini
[env:esp01_1m]
platform = espressif8266
board = esp01_1m
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0
```
