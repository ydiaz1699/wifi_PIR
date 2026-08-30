# PROJECT_CONTEXT.md — proyecto-reloj-ntp-nodemcu

## Qué es esto

Reloj digital con LCD 16x2 I2C que sincroniza la hora por NTP vía WiFi y la
muestra en dígitos grandes (grilla 3x2 de caracteres personalizados). Corre
sobre NodeMCU v2 (ESP8266-12E), compilado con PlatformIO. Estado: **producción**.

- **Versión:** 1.0.0
- **Última actualización:** 2026-06-21
- **Autor:** A.A.D.M
- **Licencia:** MIT (recomendado agregar archivo `LICENSE`, aún no existe)

## Objetivo principal

Mostrar la hora sincronizada por NTP en un LCD 16x2 con dígitos grandes,
incluyendo indicadores de estado WiFi/NTP, sin bloqueos en el loop principal.

## Restricciones clave

- Ver [`../../boards/esp8266-nodemcu-v2.md`](../../boards/esp8266-nodemcu-v2.md)
  para voltajes y pines críticos de la placa.
- **No bloqueante es obligatorio:** WiFiManager y NtpClient son FSM. No
  introducir `delay()` en `loop()` salvo los ya marcados como excepción en
  los `.cpp` existentes — ver `.ai/SKILL.md`.
- `secrets.h` debe existir para compilar y **nunca se sube a Git** — ver
  `secrets.h.template` en la raíz del proyecto.

## Puntos de entrada

- `src/main.cpp`

## Documentos relacionados

- `../../boards/esp8266-nodemcu-v2.md` — ficha técnica de la placa (fuente de verdad)
- `.ai/HARDWARE.md` — wiring específico de este proyecto (LCD I2C)
- `.ai/SOFTWARE.md` — `platformio.ini`, dependencias, comandos de build
- `.ai/SKILL.md` — reglas críticas para la IA (CGRAM, C++17, secrets.h, etc.)
- `.ai/ARCHITECTURE.md` — máquinas de estado WiFi/NTP/Display y sus transiciones
- `.ai/TASKS.md` / `.ai/ROADMAP.md` — pendientes conocidos
- `../shared/SOFTWARE.md` — entorno VS Code compartido con el resto del workspace
