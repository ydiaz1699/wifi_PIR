# SKILL.md — Reglas para la IA (proyecto-reloj-ntp-nodemcu)

> Reglas críticas genéricas de la placa (voltajes, boot pins) están en
> [`../../boards/esp8266-nodemcu-v2.md`](../../boards/esp8266-nodemcu-v2.md).
> Esto es lo específico de este firmware — es la parte más importante de
> este proyecto para que una IA no rompa nada. Léelo completo antes de tocar código.

## La IA NUNCA debe:

1. **Usar el slot CGRAM 0.** Los caracteres personalizados del LCD usan slots
   1–8. El slot 0 no está cargado — si se usa, el display muestra basura
   visual. Usar siempre `display::chars::BLANK` (ASCII 32, espacio) en su lugar.
   *Severidad: crítica — display roto.*
2. **Asumir que `secrets.h` existe.** Es obligatorio para compilar y está en
   `.gitignore` (nunca se sube a Git). Si falta: `cp include/secrets.h.template
   include/secrets.h` y completar SSID/password/zona horaria.
   *Síntoma si falta: `error: 'secrets' was not declared in this scope'`.*
3. **Usar inicialización designada de C++20** (`.campo = valor`). El toolchain
   xtensa gcc 10.3 soporta C++17 pero NO designated initializers. `TimePacked`
   usa constructor posicional a propósito — no lo cambies a `{.hour = 10}`.
   *Síntoma: `error: expected primary-expression before '.' token`.*
4. **Agregar arrays o strings grandes sin PROGMEM.** Flash total 4MB, pero el
   framework + libs ya consume ~1.2MB. Datos constantes grandes van a PROGMEM.
   *Síntoma: `error: section '.text' will not fit in region 'irom0_0_seg'`.*
5. **Introducir `delay()` en `loop()`.** WiFiManager y NtpClient son FSM no
   bloqueantes basadas en `millis()`. Los únicos `delay()` permitidos son los
   ya existentes en `WiFiManager.cpp` (transición CONNECTING→CONNECTED, están
   marcados como excepción deliberada). Un `delay()` nuevo congela el display.

## La IA SIEMPRE debe:

- Revisar `../../boards/esp8266-nodemcu-v2.md` antes de proponer un pinout nuevo.
- Preferir `millis()` sobre `delay()` para cualquier timeout nuevo.
- Usar la macro `F()` para strings literales nuevos en Serial/LCD (ahorra RAM,
  aunque menos crítico aquí que en el Arduino Uno).

## Limitaciones conocidas (no bloquean, pero hay que saberlas)

| Área | Problema | Severidad | Mitigación |
| --- | --- | --- | --- |
| WiFi | Reconexión sin exponential backoff (siempre cada 30s) | Media | Backoff progresivo pendiente — ver `.ai/TASKS.md` |
| Sistema | Sin Watchdog Timer — si `loop()` se cuelga, no reinicia | Alta | Agregar `ESP.wdtEnable(WDTO_8S)` en `setup()` — pendiente |
| Config | Credenciales WiFi y zona horaria fijas en compile-time | Baja | Requiere SPIFFS + portal cautivo para runtime config |
| NTP | Si falla permanentemente, display sigue mostrando hora vieja sin aviso claro | Media | Falta indicador "NO SYNC" visible |
| Memoria | Heap puede fragmentarse tras muchas conexiones WiFi/NTP | Baja | Sin monitoreo implementado — `ESP.getFreeHeap()` disponible si se necesita |

## Antes de cualquier cambio de hardware

1. Leer `../../boards/esp8266-nodemcu-v2.md` (specs genéricas) y `.ai/HARDWARE.md` (wiring actual).
2. Confirmar niveles de voltaje del periférico nuevo (recordar: el LCD acepta
   5V en VCC por su propio datasheet, no por ser regla de la placa).
3. Actualizar la tabla de "Periféricos" en `.ai/HARDWARE.md`.
4. Anotar la decisión en `.ai/DECISIONS.md` si implica un trade-off.
