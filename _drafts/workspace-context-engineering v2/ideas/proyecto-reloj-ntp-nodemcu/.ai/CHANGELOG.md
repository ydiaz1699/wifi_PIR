# CHANGELOG.md — proyecto-reloj-ntp-nodemcu

## v1.0.0 (2026-06-21)

- Arquitectura refactorizada: WiFiManager, NtpClient, Display separados en
  módulos independientes.
- FSM no bloqueante para WiFi y NTP (sin `delay()` en loop, salvo excepciones marcadas).
- Dígitos grandes con caracteres personalizados (CGRAM, patrón 3x2 por dígito).
- Indicadores de estado W/T/* en esquina del display.
- Soporte para zona horaria configurable (vía `secrets.h`, compile-time).

## [Sin versionar]

- Incorporado al workspace de Context Engineering: separación de hardware
  (`../../boards/esp8266-nodemcu-v2.md`) y descomposición del antiguo
  `archivo-mapa.yml` monolítico en los archivos `.ai/` estándar.
