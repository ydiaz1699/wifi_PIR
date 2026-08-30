# 🖥️ Placa: Wemos D1 Mini Pro (ESP8266)

## 🧩 Hardware Principal

- **Placa:** Wemos D1 Mini Pro
- **MCU:** ESP-8266EX (Tensilica L106, 32-bit RISC)
- **Clock:** 80 MHz / 160 MHz (configurable)
- **RAM:** ~80 KB user RAM
- **Flash:** 16 MB (128 Mbit)
- **Alimentación:** 5V USB → 3.3V regulado interno (LDO)
- **Dimensiones:** 34.2 mm × 25.6 mm
- **Peso:** 2.5 g

## 📡 Conectividad

- **WiFi:** 802.11 b/g/n (2.4 GHz)
- **Antena:** Cerámica integrada + conector U.FL para antena externa
- **USB-UART:** CP2104

## 🔌 Periféricos / Módulos Conectados

| Módulo | Protocolo | Nivel Lógico | Pines Usados | Notas |
| --- | --- | --- | --- | --- |
| Módulo de nivel lógico bidireccional | — | 3.3V ↔ 5V | — | 4 canales, basado en MOSFETs |
| *(agregar)* |  |  |  |  |

## ⚡ Niveles de Voltaje

- **D1 Mini Pro:** 3.3V lógico (GPIO) / 5V solo en VCC (alimentación)
- **GPIO:** **NO tolerantes a 5V** — máximo 3.3V (3.6V absoluto, operar siempre a 3.3V)
- **ADC (A0):** Máximo 3.2V (1 canal, 10-bit, 0–3.2V)
- **Conversión:** Usar módulo de nivel lógico para cualquier interfaz con dispositivos 5V

## 🗺️ Mapeo de Pines

| Función | GPIO | D# | Uso / Notas |
| --- | --- | --- | --- |
| UART0 TX | GPIO1 | D10 | Debug/Monitor Serial (USB) |
| UART0 RX | GPIO3 | D9 | Debug/Monitor Serial (USB) |
| SPI SCK | GPIO14 | D5 | HSCLK |
| SPI MISO | GPIO12 | D6 | HMISO |
| SPI MOSI | GPIO13 | D7 | HMOSI |
| SPI CS | GPIO15 | D8 | HCS (boot: pull-down required) |
| I2C SDA | GPIO4 | D2 |  |
| I2C SCL | GPIO5 | D1 |  |
| PWM / IO | GPIO0 | D3 | Boot: pull-up required |
| PWM / IO | GPIO2 | D4 | Built-in LED (active LOW), boot: pull-up required |
| PWM / IO | GPIO16 | D0 | Deep sleep wake, no PWM |
| ADC | A0 | A0 | 0–3.2V, 10-bit |

## ⚠️ Consideraciones Críticas

- **NUNCA** conectar 5V a GPIO o A0 — daño irreversible al ESP8266EX
- **GPIO 0, 2, 15:** Niveles en boot determinan modo de arranque (flash vs. programación)
- **GPIO 16 (D0):** Sin PWM, único pin para wake desde deep sleep
- **GPIO 2 (D4):** LED integrado (LOW = encendido), activo en boot
- Usar módulo de nivel lógico para UART/I2C/SPI con dispositivos 5V
- Monitor serial requiere puerto libre (cerrar `pio device monitor` si está ocupado)
- **Flash 16 MB:** Verificar partition table en PlatformIO (por defecto puede usar solo 4 MB)
- **Pines SPI (D5-D8):** Evitar para GPIO genérico si se usa SPI simultáneamente
- **D3 y D4:** Evitar como salidas en boot — pueden causar comportamiento inesperado

## 📡 `platformio.ini`

```ini
[env:d1_mini_pro]
platform = espressif8266
board = d1_mini_pro
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0       ; Linux/Mac
; monitor_port = COM3             ; Windows

; Descomentar para decodificar crashes
; monitor_filters = esp8266_exception_decoder

build_flags =
    -D PIO_FRAMEWORK_ARDUINO
    -D ARDUINO_ARCH_ESP8266

; Flash completa — si la board detecta solo 4MB, forzar con:
; board_build.flash_size = 16MB
; board_build.ldscript = eagle.flash.16m.ld
```
