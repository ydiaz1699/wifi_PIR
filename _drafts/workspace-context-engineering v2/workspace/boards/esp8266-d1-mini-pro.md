# Board: Wemos D1 Mini Pro

> Ficha técnica reutilizable. NO poner aquí periféricos de un proyecto concreto
> (eso va en el `HARDWARE.md` del proyecto, en la sección "Wiring de este proyecto").
> Si algo de esto cambia, cambia para **todos** los proyectos que usan esta placa.

## Identidad

- **MCU:** ESP-8266EX (Tensilica L106, 32-bit)
- **Clock:** 80 MHz / 160 MHz (configurable)
- **RAM:** ~80 KB user RAM
- **Flash:** 16 MB (128 Mbit)
- **Alimentación:** 5V USB → 3.3V regulado interno (LDO)
- **Dimensiones:** 34.2 mm × 25.6 mm — 2.5 g

## Conectividad

- **WiFi:** 802.11 b/g/n (2.4 GHz), antena cerámica + conector U.FL
- **USB-UART:** CP2104

## Niveles de Voltaje

- **Lógico:** 3.3V — **GPIO NO tolerantes a 5V** (máx 3.6V absoluto)
- **ADC (A0):** máximo 3.2V (1 canal, 10-bit)
- **Regla de oro:** cualquier periférico de 5V pasa por módulo de nivel lógico bidireccional

## Mapeo de Pines (genérico de la placa)

| Función | GPIO | D# | Uso / Notas |
| --- | --- | --- | --- |
| UART0 TX | GPIO1 | D10 | Debug/Monitor Serial (USB) |
| UART0 RX | GPIO3 | D9 | Debug/Monitor Serial (USB) |
| SPI SCK | GPIO14 | D5 | HSCLK |
| SPI MISO | GPIO12 | D6 | HMISO |
| SPI MOSI | GPIO13 | D7 | HMOSI |
| SPI CS | GPIO15 | D8 | HCS (boot: pull-down required) |
| I2C SDA (sugerido) | GPIO4 | D2 | Libre para I2C |
| I2C SCL (sugerido) | GPIO5 | D1 | Libre para I2C |
| PWM / IO | GPIO0 | D3 | Boot: pull-up required |
| PWM / IO | GPIO2 | D4 | Built-in LED (active LOW), boot: pull-up required |
| PWM / IO | GPIO16 | D0 | Deep sleep wake, no PWM |
| ADC | A0 | A0 | 0–3.2V, 10-bit |

## Consideraciones críticas de esta placa (aplican a todo proyecto que la use)

- **NUNCA** conectar 5V a GPIO o A0 — daño irreversible al ESP8266EX
- **GPIO 0, 2, 15:** niveles en boot determinan modo de arranque (flash vs. programación)
- **GPIO 16 (D0):** sin PWM, único pin para wake desde deep sleep
- **GPIO 2 (D4):** LED integrado (LOW = encendido), activo en boot
- **Flash 16 MB:** PlatformIO puede detectar solo 4MB por defecto — forzar con
  `board_build.flash_size = 16MB` y `board_build.ldscript = eagle.flash.16m.ld`
- **Pines SPI (D5-D8):** evitar como GPIO genérico si se usa SPI simultáneamente

## `platformio.ini` — bloque `[env:]` de referencia

```ini
[env:d1_mini_pro]
platform = espressif8266
board = d1_mini_pro
framework = arduino
monitor_speed = 115200
```

## Proyectos que usan esta placa

- `../proyecto-esp8266-d1minipro/`
