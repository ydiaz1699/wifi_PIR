# Board: NodeMCU v2 (ESP8266-12E)

> Ficha técnica reutilizable. NO poner aquí periféricos de un proyecto concreto
> (eso va en el `HARDWARE.md` del proyecto, en la sección "Wiring de este proyecto").
> Si algo de esto cambia, cambia para **todos** los proyectos que usan esta placa.
>
> **Nota:** comparte familia de MCU (ESP8266) con el D1 Mini Pro, pero es una placa
> física distinta — más grande, breakout diferente, y con más flash/RAM disponible
> reportado. No asumir que los pinouts son intercambiables sin verificar el silkscreen.

## Identidad

- **MCU:** ESP8266-12E
- **Clock:** 80 MHz (ajustable a 160 MHz)
- **RAM:** 160 KB SRAM total (~80KB típicamente disponible para el usuario, igual que
  otras placas ESP8266 — el resto lo usa el SDK/WiFi stack)
- **Flash:** 4 MB
- **Alimentación:** 5V USB → 3.3V regulado interno

## Conectividad

- **WiFi:** 802.11 b/g/n (2.4 GHz)
- **USB-UART:** variante según fabricante (CH340/CP2102 típico en NodeMCU v2)

## Niveles de Voltaje

- **Lógico:** 3.3V — **GPIO NO tolerantes a 5V**
- **Regla de oro:** cualquier periférico de 5V pasa por módulo de nivel lógico
  bidireccional o divisor resistivo. Excepción común: módulos LCD I2C con
  PCF8574 suelen aceptar 5V en VCC aunque el bus lógico corra a 3.3V — **verificar
  siempre la hoja de datos del módulo específico**, no asumir.

## Mapeo de Pines (genérico de la placa, notación NodeMCU)

| Función | GPIO | Notación NodeMCU | Notas |
| --- | --- | --- | --- |
| I2C SDA (uso común) | GPIO4 | D2 | Sin resistencias pull-up internas fuertes — módulos I2C suelen traer las propias |
| I2C SCL (uso común) | GPIO5 | D1 | |
| UART0 TX/RX | GPIO1/GPIO3 | TX/RX | Debug/Monitor Serial (USB) |
| Boot-sensitive | GPIO0, GPIO2, GPIO15 | D3, D4, D8 | Niveles en boot determinan modo de arranque |
| LED integrado | GPIO2 | D4 | Active LOW |
| ADC | A0 | A0 | 1 canal, ver datasheet del breakout para rango exacto |

## Consideraciones críticas de esta placa (aplican a todo proyecto que la use)

- **NUNCA** conectar 5V a un GPIO lógico — daño irreversible
- **GPIO 0, 2, 15:** niveles en boot determinan modo de arranque (flash vs. programación)
- **Flash 4MB:** más ajustado que el D1 Mini Pro (16MB) — vigilar el `.map` tras
  compilar si se agregan librerías grandes o SPIFFS/LittleFS
- **Sin WDT por defecto:** si el `loop()` se cuelga, no hay reinicio automático a
  menos que se habilite `ESP.wdtEnable(WDTO_8S)` explícitamente en `setup()`

## `platformio.ini` — bloque `[env:]` de referencia

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
monitor_speed = 115200
upload_speed = 921600
upload_resetmethod = nodemcu
```

## Proyectos que usan esta placa

- `../proyecto-reloj-ntp-nodemcu/`
