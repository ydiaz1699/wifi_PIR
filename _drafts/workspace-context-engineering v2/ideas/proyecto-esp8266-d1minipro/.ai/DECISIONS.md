# DECISIONS.md — proyecto-esp8266-d1minipro

## Separar hardware del entorno de desarrollo
**Por qué:** el documento original mezclaba specs eléctricas con config de VS Code,
dificultando que una IA (o una persona) actualice uno sin releer el otro.
**Decisión:** HARDWARE.md solo contiene MCU/pines/voltajes/periféricos.
SOFTWARE.md (local y compartido) contiene IDE/extensiones/build config.
