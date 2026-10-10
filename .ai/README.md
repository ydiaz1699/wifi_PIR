# .ai — Contexto para asistentes de IA

Esta carpeta es el **contexto de ingeniería** del repositorio `wifi_PIR`,
siguiendo la metodología de Context Engineering (frameworks
`ydiaz1699/Context_Engineering_V2` y `ydiaz1699/vsCode-AI`). Su objetivo es que
cualquier asistente de IA pueda **leer `.ai/` y entender todo el sistema** sin
que el usuario tenga que repetir el contexto.

## Cómo usarla

1. **Lee `.ai/` completo** antes de proponer o escribir cambios.
2. **Sigue las reglas** de `PROTOCOL.md` y `CODING_STYLE.md`: no romper el wire
   V4.1, firma centralizada, secretos fuera de git, estilo en español para la
   app.
3. Verifica siempre con lo descrito en `TESTING.md` tras cualquier cambio.
4. Trata `docs/` y `.agents/` como fuente detallada: estos archivos **resumen y
   referencian**, no duplican.

## Índice

| Archivo | Para qué |
|---|---|
| `PROJECT_CONTEXT.md` | Qué es el proyecto, componentes y diagrama de alto nivel. |
| `ARCHITECTURE.md` | Arquitectura del sistema y del protocolo `IoTProtocol`. |
| `HARDWARE.md` | Placas, pines reales, IPs estáticas, requisitos del PIR. |
| `SOFTWARE.md` | Toolchain PlatformIO, envs, `lib_deps`, compilar/subir. |
| `PROTOCOL.md` | Reglas de interacción con la IA al trabajar en el repo. |
| `CODING_STYLE.md` | Estilo real observado en el código. |
| `TASKS.md` | Pendientes reales (no bloqueantes + roadmap). |
| `CHANGELOG.md` | Cambios recientes (arreglos B1–B15). |
| `SKILL.md` | Skill de análisis profundo: cuándo y cómo. |
| `TESTING.md` | Cómo verificar (pio run, checks host, tests incrementales). |
| `DECISIONS.md` | Decisiones de diseño y su justificación. |
| `ROADMAP.md` | Rumbo del proyecto (V4.4, V5). |

## Fuentes autoritativas (no duplicar)

- Código: `lib/IoTProtocol/`, `lib/AlarmProfile/`, `emisor_pir_unificado/`,
  `receptor_central_unificado/`.
- Documentación: `docs/` (ARCHITECTURE, OPERATIONS, ROADMAP, CHANGELOG,
  PLAN_EJECUCION_FUTURA, auditorías).
- Informes de análisis: `.agents/tasks/` y `.agents/reviews/`.

> Regla de oro: **nunca** copiar credenciales reales (WiFi/MQTT, `IOT_AUTH_KEY`,
> contraseñas OTA) a esta carpeta ni a ningún otro archivo versionado.
