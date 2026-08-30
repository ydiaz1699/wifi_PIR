# ROADMAP.md — proyecto-reloj-ntp-nodemcu

## Fase actual

Producción — v1.0.0. Funcionalidad core estable (WiFi + NTP + display).

## Próximas fases (por prioridad sugerida)

1. **Robustez:** Watchdog Timer + indicador NO SYNC (ver `.ai/TASKS.md`)
2. **Configurabilidad:** zona horaria y credenciales sin recompilar
3. **Calidad:** tests unitarios básicos para `TimePacked` y las FSM
