# SOFTWARE.md — proyecto-esp8266-d1minipro

> Entorno base (VS Code, extensiones) está en `../../shared/SOFTWARE.md`.
> Este archivo cubre solo lo específico de este proyecto.

## `platformio.ini`

```ini
[env:d1_mini_pro]
platform = espressif8266
board = d1_mini_pro
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0       ; Linux/Mac
; monitor_port = COM3             ; Windows

; Descomentar para decodificar crashes — muestra línea exacta del error
; monitor_filters = esp8266_exception_decoder

; IntelliSense: expone defines para que C/C++ Extension Pack resuelva macros
build_flags =
    -D PIO_FRAMEWORK_ARDUINO
    -D ARDUINO_ARCH_ESP8266

; Flash completa — si la board detecta solo 4MB, forzar con:
; board_build.flash_size = 16MB
; board_build.ldscript = eagle.flash.16m.ld

; lib_deps =
;     (agregar librerías del sensor PIR u otras cuando se definan)
```

## Dependencias del proyecto

- *(agregar librerías conforme se integren sensores/módulos)*
