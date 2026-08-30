# DECISIONS.md — proyecto-reloj-ntp-nodemcu

## FSM no bloqueante en vez de delay()
**Por qué:** un reloj con display debe seguir actualizándose (parpadeo de dos
puntos, renderizado) mientras espera WiFi o NTP, que pueden tardar segundos.
**Decisión:** WiFiManager y NtpClient son máquinas de estado basadas en
`millis()`, actualizadas cada `loop()`. Único uso de `delay()` permitido: las
transiciones ya marcadas en `WiFiManager.cpp` como excepción deliberada.

## secrets.h separado de secrets.h.template
**Por qué:** las credenciales WiFi no deben subirse a control de versiones,
pero un colaborador nuevo necesita saber qué campos llenar.
**Decisión:** `secrets.h.template` se versiona (sin credenciales reales),
`secrets.h` real está en `.gitignore`. Ver `.ai/SOFTWARE.md` para el setup.

## Descomponer archivo-mapa.yml en archivos .ai/ separados
**Por qué:** el YAML original mezclaba hardware, software, arquitectura,
problemas conocidos y changelog en un solo archivo de ~500 líneas — costoso
de mantener y de darle a una IA solo el contexto que necesita para una tarea puntual.
**Decisión:** separado en `HARDWARE.md`, `SOFTWARE.md`, `SKILL.md`,
`ARCHITECTURE.md`, `TASKS.md`, `ROADMAP.md`, `CHANGELOG.md`, siguiendo el
mismo esquema que el resto del workspace. El hardware genérico de la placa
además se movió a `../../boards/esp8266-nodemcu-v2.md` para no duplicarlo
si en el futuro hay otro proyecto con la misma placa.
