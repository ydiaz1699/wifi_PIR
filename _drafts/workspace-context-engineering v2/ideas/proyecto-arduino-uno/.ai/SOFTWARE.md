# SOFTWARE.md — proyecto-arduino-uno

> Entorno base (VS Code, extensiones) está en `../../shared/SOFTWARE.md`.
> Este archivo cubre solo lo específico de este proyecto.

## `platformio.ini`

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0       ; Linux/Mac
; monitor_port = COM3             ; Windows

; Librerías del proyecto actual
lib_deps =
    sui77/rc-switch@^2.6.4     ; RF 433MHz
```

## Dependencias del proyecto

- **RCSwitch** (`sui77/rc-switch@^2.6.4`) — decodificación de códigos 24-bit
  del receptor RF 433MHz conectado en D2 (INT0).
