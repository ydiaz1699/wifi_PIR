# TASKS.md — proyecto-reloj-ntp-nodemcu

## Pendiente (extraído de problemas conocidos)

- [ ] **Alta prioridad:** agregar `ESP.wdtEnable(WDTO_8S)` en `setup()` — no hay
      Watchdog Timer, si `loop()` se cuelga el reloj queda congelado indefinidamente.
- [ ] Implementar exponential backoff en reconexión WiFi (hoy: fijo cada 30s
      sin aumento, puede generar trancas con redes inestables).
- [ ] Agregar indicador "NO SYNC" visible en display cuando NTP esté en estado
      FAILED — hoy el display sigue mostrando la hora vieja sin aviso claro.
- [ ] Agregar tests unitarios en `test/` (actualmente vacía, sin CI/CD).

## Pendiente (mejoras de configurabilidad, baja prioridad)

- [ ] Zona horaria configurable en runtime (hoy hardcoded en `secrets.h`,
      requiere recompilar). Candidato: SPIFFS/LittleFS + menú con botones.
- [ ] Credenciales WiFi persistentes/configurables sin recompilar. Candidato:
      WPS, Smart Config, o portal cautivo (+~10KB flash estimado).

## Bugs conocidos

- Ninguno abierto actualmente. El bug histórico de CGRAM slot 0 (basura visual)
  ya está resuelto — ver `.ai/SKILL.md` regla #1 para no reintroducirlo.
