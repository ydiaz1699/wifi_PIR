# 🖥️ Placa: STM32F401/F411 "Black Pill"

## 🧩 Hardware Principal

- **Placa:** STM32F401CC/F411CE "Black Pill"
- **MCU:** ARM Cortex-M4 (32-bit, FPU)
- **Clock:** 84 MHz (F401) / 100 MHz (F411)
- **RAM:** 96 KB (F401) / 128 KB (F411)
- **Flash:** 256 KB (F401) / 512 KB (F411)
- **Alimentación:** 3.3V / 5V USB-C
- **ADC:** 12-bit, 16 canales
- **USB:** USB nativo (CDC) — no requiere UART bridge

## 🗺️ Mapeo de Pines (parcial)

| Función | Pin | Notas |
| --- | --- | --- |
| UART1 TX | PA9 |  |
| UART1 RX | PA10 |  |
| I2C1 SDA | PB7 |  |
| I2C1 SCL | PB6 |  |
| SPI1 SCK | PA5 |  |
| USB D+ | PA12 | USB nativo |
| USB D- | PA11 | USB nativo |
| Built-in LED | PC13 | Active LOW |

## ⚡ Niveles de Voltaje

- **GPIO:** 3.3V lógico — **NO tolerante a 5V**
- **ADC:** 12-bit, 0–3.3V
- **USB:** 5V en VBUS, 3.3V en lógica

## ⚠️ Consideraciones Críticas

- **USB nativo:** CDC funciona sin UART bridge, pero requiere resistencia de 1.5k en D+ (ya onboard)
- **FPU:** Cortex-M4 tiene unidad de punto flotante — ideal para algoritmos DSP
- **Más RAM/Flash** que Blue Pill — proyectos complejos sin problemas
- **Bootloader:** Puede cargar STM32duino bootloader para programación USB
- **Consumo:** ~40 mA activo, deep sleep ~1 µA

## 📡 `platformio.ini`

```ini
[env:blackpill_f401cc]
platform = ststm32
board = blackpill_f401cc
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyACM0

build_flags =
    -D USBCON
    -D USBD_USE_CDC
    -D HAL_PCD_MODULE_ENABLED

; Para F411:
; board = blackpill_f411ce
; 128 KB RAM, 512 KB Flash, 100 MHz
```
