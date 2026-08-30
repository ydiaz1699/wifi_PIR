# SOFTWARE.md — proyecto-reloj-ntp-nodemcu

> Entorno base (VS Code, extensiones) está en `../../shared/SOFTWARE.md`.
> Este archivo cubre solo lo específico de este proyecto.

## `platformio.ini`

```ini
[platformio]
default_envs = nodemcuv2

[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino

monitor_speed = 115200
upload_speed = 921600
upload_resetmethod = nodemcu

build_type = release
board_build.filesystem = littlefs

lib_deps =
    marcoschwartz/LiquidCrystal_I2C@^1.1.4

build_flags =
    -std=c++17
    -Os
    -Wall
    -Wextra
    -fno-exceptions
    -DPIO_FRAMEWORK_ARDUINO_LWIP2_LOW_MEMORY

; Para debug detallado, agregar también:
; -DDEBUG_LOG
```

## Dependencias del proyecto

- **LiquidCrystal_I2C** (`marcoschwartz/LiquidCrystal_I2C@^1.1.4`) — control del LCD 16x2
- **ESP8266WiFi** (built-in framework) — conexión WiFi
- **Wire** (built-in framework) — bus I2C

## Comandos

| Acción | Comando |
| --- | --- |
| Build | `platformio run` |
| Upload | `platformio run --target upload` |
| Monitor | `platformio device monitor --baud 115200` |
| Build + upload + monitor | `platformio run --target upload && platformio device monitor` |

## Setup inicial (primera vez)

```bash
git clone <repo> && cd reloj-ntp-nodemcu
cp include/secrets.h.template include/secrets.h
# Editar include/secrets.h: WiFi SSID, password, zona horaria
platformio run --target upload
platformio device monitor --baud 115200
```

Resultado esperado en LCD: `Iniciando reloj` → `Sincronizando hora (NTP)...` → `HH:MM`.

## Build config

- **Estándar:** C++17
- **Optimización:** `-Os` (tamaño)
- **Sistema de archivos:** LittleFS
- **Tamaño binario estimado:** ~200–250 KB (release build)
- **RAM en runtime (estimado):** ~40–60 KB

## Habilitar debug logs

Agregar `-DDEBUG_LOG` a `build_flags` en `platformio.ini`, recompilar y monitorear.
Ver `include/log.h` para las macros `LOG`/`LOGLN`/`LOGF` condicionales.
