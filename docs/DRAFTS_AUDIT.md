# Auditoría canónica de `_drafts/`

**Fecha de consolidación:** 2026-09-02
**Estado:** trazabilidad completa; los diez borradores originales fueron consolidados y retirados; este documento conserva su procedencia, destinos y pendientes.
**Propósito:** conservar la procedencia, las decisiones, los descartes y el backlog de los diez archivos históricos de `_drafts/` sin depender de esos archivos.

Este documento reemplaza a los borradores como registro histórico. No convierte una propuesta en una función implementada: cada entrada conserva por separado su clasificación y su estado de evidencia.

## Estados

- **INTEGRADO:** el principio, fix o método ya tiene destino en código o documentación canónica.
- **MEJORA:** propuesta válida con destino claro, aún no implementada o no verificada.
- **PENDIENTE:** requiere diseño, decisión o prueba antes de aplicarse.
- **RECHAZADO:** no se adopta por contradicción, falta de evidencia o riesgo técnico.
- **FUERA DE ALCANCE:** operación externa, publicación automática o contenido que no pertenece al firmware/protocolo.
- **NO_DECIDIBLE:** falta evidencia externa o una decisión de producto.

La evidencia de implementación sigue esta secuencia:

```text
PROPUESTO → APLICADO → COMPILADO → VERIFICADO → VERIFICADO EN HARDWARE
```

## Inventario completo

| Draft retirado | Contenido principal | Estado dominante | Destino canónico |
|---|---|---|---|
| `1mejoras.md` | Sirena no bloqueante, `isBusy()`, configuración y MQTT | MEJORA/PENDIENTE | `PLAN_EJECUCION_FUTURA.md` Fase 8; `MATRIZ_UNIFICACION_V3_V4.md` F-05 |
| `BUGS_FIXED.md` | BUG-001…BUG-010, causas y reglas preventivas | INTEGRADO con estados individuales | `docs/BUGS_FIXED.md`, plan sección de bugs |
| `META_PROMPT.md` | Método para producir documentación auditable | INTEGRADO | `ANALISIS_INICIAL_HALLAZGOS.md`, `universal-protocol/META_PROMPT.md` |
| `bugs.md` | BUG-011, MQTT/HA retained y topología de red | INTEGRADO/PENDIENTE/NO_DECIDIBLE | `docs/BUGS_FIXED.md`, `OPERATIONS.md`, análisis de hallazgos |
| `ideas.md` | Seguridad, telemetría, event log, estados, zonas, config, OTA, capabilities y V5 | PENDIENTE/MEJORA | `ROADMAP.md`, `CAPABILITY_DISCOVERY.md`, plan y backlog V5 |
| `instrucciones.md` | Aplicación segura de patch, compilación, commit/push | INTEGRADO/FUERA DE ALCANCE | `OPERATIONS.md`, plan; no se publica automáticamente |
| `plantilla de prompt.md` | Plantilla de requisitos, alternativas, pruebas y ADR | INTEGRADO | `universal-protocol/META_PROMPT.md`, análisis |
| `prodoco.md` | Capas aplicación/protocolo/transporte y separación del core | INTEGRADO/PENDIENTE | `ARCHITECTURE.md`, `INFORME_UNIFICACION.md`, backlog V5 |
| `prompt.md` | Auditoría V4, seguridad, migración, API, tests y entrega | INTEGRADO/PENDIENTE | plan, roadmap, informes universales y `OPERATIONS.md` |
| `prompt2.md` | Comparación tecnológica y amenaza/seguridad V5 | PENDIENTE/NO_DECIDIBLE | `INFORME_UNIFICACION.md`, sección V5 de `ROADMAP.md` |

## Trazabilidad por archivo

### `1mejoras.md` — sirena

| Hallazgo | Estado | Decisión/destino |
|---|---|---|
| `Buzzer::sirenOn(onMs, offMs, totalMs)` con patrón aproximado 200/200 ms durante 4 s | PENDIENTE | Fase 8; no copiar el bloque histórico sin pruebas |
| `Buzzer::isBusy()` durante toda la sirena, incluso con el pin momentáneamente apagado | PENDIENTE | Debe acompañar a la máquina de patrón |
| Conservar `timedOn(500)` para TIMBRE | INTEGRADO | El baseline conserva el beep temporizado |
| MQTT debe consultar ocupación del patrón, no solo estado instantáneo del pin | PENDIENTE | Actualizar junto con `isBusy()` |
| Centralizar tiempos `ON`, `OFF` y duración total | PENDIENTE | Configuración del perfil alarma |
| Comparación de `millis()` segura ante rollover | INTEGRADO como regla; implementación de sirena pendiente | Usar diferencia signed y probar rollover |
| Política ante TIMBRE, nueva MOTION y apagado manual | PENDIENTE | Decidir antes de tocar firmware |
| `pio run -t upload` y el archivo de resumen de sesión | FUERA DE ALCANCE/NO_DECIDIBLE | El resumen no existe; los comandos están en `OPERATIONS.md` solo como procedimiento externo |

### `BUGS_FIXED.md` — BUG-001 a BUG-010

Los síntomas, causas, reglas preventivas y pruebas faltantes se conservan en `docs/BUGS_FIXED.md`. La decisión importante es no tratar “fix histórico” como “verificado”.

| Bug | Estado consolidado | Qué falta |
|---|---|---|
| 001 MQTT bloqueando UDP | APLICADO parcialmente | Medición con broker caído y métrica del loop |
| 002 PIR/TIMBRE mutuamente bloqueados | APLICADO en diseño V3 | Prueba simultánea en hardware |
| 003 PIR sostenido/antirebote | APLICADO parcialmente | Medición con HC-SR501 real |
| 004 macro `LOCAL` | APLICADO | Mantener nombres seguros y línea base compilable |
| 005 OTA/firewall | APLICADO documentalmente | Prueba desde PC/red real |
| 006 `connect()` bloqueante | APLICADO como regla | Auditar rutas actuales |
| 007 B622/DMZ/repetidor | NO_DECIDIBLE | Medir gateway, subnet, rutas y UDP; no fijar IPs como arquitectura |
| 008 BOOT_ID | APLICADO en código; persistencia y hardware pendientes | Verificar `IoTStorage`→`IoTNode`, fallos de LittleFS y reinicios |
| 009 secretos | APLICADO como prevención | Auditar historial Git y rotar si hubo exposición |
| 010 autenticación temprana | APLICADO en frontera del nodo según código actual; integración/hardware pendientes | Probar inválido, duplicado, replay y orden de efectos |

### `META_PROMPT.md` — método documental

- Separar problema, objetivo, usuarios, entradas, procesamiento, salidas, restricciones y supuestos: **INTEGRADO**.
- Distinguir hechos, inferencias, decisiones, preguntas abiertas y evidencia: **INTEGRADO** en `ANALISIS_INICIAL_HALLAZGOS.md`.
- Exigir requisitos verificables, casos de uso, edge cases, alternativas, riesgos y criterios de aceptación: **INTEGRADO** en el meta-prompt unificado y el plan.
- Mantener un lector sin contexto y paths/comandos explícitos: **INTEGRADO**.
- Commit/push automático: **RECHAZADO como regla**; publicación requiere revisión humana.

### `bugs.md` — BUG-011 y red/MQTT

- El uso de `doc.to<JsonObject>()` frente a `doc.as<JsonObject>()`: **INTEGRADO EN CÓDIGO**, documentado como BUG-011.
- Inspección de retained con `mosquitto_sub`: **INTEGRADO como procedimiento**, sin afirmar que se haya ejecutado.
- Entidades Home Assistant visibles y changelog V3.5.2: **RECHAZADO como claim verificado** hasta probar broker/HA.
- B622, doble NAT, DMZ, IPs y reservas DHCP: **NO_DECIDIBLE/FUERA DEL PROTOCOLO** hasta medir la red real.
- Diferenciar bridge/router usando gateway y rutas: **INTEGRADO como criterio operativo** en `OPERATIONS.md`.

### `ideas.md` — backlog por frontera

| Idea del draft | Clasificación | Destino | Estado |
|---|---|---|---|
| HMAC uniforme, anti-replay, BOOT_ID y secretos separados | MEJORA | `IoTNode`, `IoTAuth`, plan y BUGS_FIXED | Parcial; pruebas y provisioning pendientes |
| Watchdog, recuperación WiFi/MQTT, registro de eventos | MEJORA | firmware/central/operación | Parcial; event log pendiente |
| Estado completo de dispositivo | MEJORA | registry/telemetría/adapter MQTT | Schema pendiente |
| Ring buffer de 100 eventos | NUEVA | central, no core | Schema e implementación pendientes |
| Máquina `DISARMED`→`ARMING`→`ARMED`→`TRIGGERED`→`ALARMING`→`ACKNOWLEDGED` | NUEVA | perfil alarma | Tabla de transiciones pendiente |
| HOME/AWAY/NIGHT/MAINTENANCE y zonas | NUEVA | perfil alarma/central | Requisitos y persistencia pendientes |
| Configuración remota persistente y rollback | MEJORA | CONFIG/RESPONSE/IoTStorage | Flujo extremo a extremo pendiente |
| Capability Discovery | MEJORA | `CAPABILITY_DISCOVERY.md` | Propuesta detallada; no implementada |
| JSON de telemetría y dashboard | MEJORA | adapter MQTT/HA | Schema, topics y pruebas retained pendientes |
| OTA distribuida/rollback | MEJORA | `OPERATIONS.md`, roadmap | ArduinoOTA parcial; rollback no garantizado por ESP8266 |
| Nuevos sensores DHT22/puerta y relé | EXTENSIÓN FUTURA | perfiles separados | No deben modificar el core sin contrato |
| AES-GCM/confidencialidad | VARIANTE CONDICIONAL | decisión de seguridad V5 | Solo si el modelo de amenazas lo exige |
| Calendario V4.3.1→V4.4→V4.5→V5 | NO_DECIDIBLE | backlog V5 | No es compromiso de implementación |
| Regla de separar core, aplicación, central y HA | INTEGRADO | arquitectura y meta-prompt | Mantener como frontera |

### `instrucciones.md` — patch y publicación

- `git status` antes de cualquier operación: **INTEGRADO**.
- `git apply --check`, aplicar, `git diff --check` y revisar diff: **INTEGRADO** en `OPERATIONS.md`.
- El patch histórico `v4.3.1-security.patch` no existe: **REGISTRADO**, no se inventa ni se aplica.
- Las rutas históricas `emisor_pir_v4` y `receptor_central_v4` no son rutas actuales: **CORREGIDO**; se usan los proyectos unificados actuales.
- `git add`, commit y push automáticos: **FUERA DE ALCANCE**; queda a decisión del usuario.
- Agregar `*.patch` temporalmente al `.gitignore`: **FUERA DE ALCANCE**; no alterar ignores silenciosamente.

### `plantilla de prompt.md` — plantilla de requisitos

- Separar problema, objetivo, usuarios, entradas, procesamiento, salidas y entorno: **INTEGRADO**.
- Distinguir requisito explícito/implícito, suposición, decisión y pregunta: **INTEGRADO**.
- Convertir objetivos en FR/NFR, casos de uso, edge cases, alternativas, riesgos y ADR: **INTEGRADO**.
- Usar `TBD` sin inventar datos y bloquear solo decisiones críticas: **INTEGRADO**.
- Comparar tecnología, evitar overengineering, definir MVP/V1/V2 y plan de pruebas: **INTEGRADO como método; investigación V5 pendiente**.
- Entrega completa y sin secretos: **INTEGRADO**.

### `prodoco.md` — arquitectura

- Tres capas aplicación/protocolo/transporte: **INTEGRADO como objetivo**.
- Core sin PIR, bocina, MQTT ni Home Assistant: **INTEGRADO**; `AlarmProfile` ya separa vocabulario de alarma.
- Message Type separado de Application Data: **INTEGRADO**.
- TLV, namespaces y unknown fields: **PARCIAL/PENDIENTE**; el wire V4 existe, pero la gobernanza V5 y la política de rangos no están cerradas.
- Versionado de protocolo separado del versionado de librería: **PARCIAL**.
- Transporte intercambiable (`ITransport`): **PENDIENTE**; `IoTNode` todavía está acoplado a `WiFiUDP`/ESP8266.
- Seguridad modular: **PARCIAL**; callbacks/HMAC existen, provisioning y AEAD no.
- Storage abstracto (`IStorage`): **PENDIENTE**; `IoTStorage` sigue acoplado a LittleFS.
- Application Profiles: **INTEGRADO**; alarma es el primer perfil.

### `prompt.md` — refactor y entrega

- Auditar antes de programar y conservar V3: **INTEGRADO**.
- Separar core, perfiles, MQTT/HA y OTA: **INTEGRADO como arquitectura objetivo**.
- Wire V4, reliability, ACK, retries, prioridades, dedup y HMAC: **PARCIAL/INTEGRADO**, con pruebas de integración pendientes.
- API pública portable `begin/loop/send/onMessage`: **PENDIENTE**; la API actual es específica de Arduino/ESP8266.
- Tests de codec: **INTEGRADO**; tests completos de IoTNode/UDP/HMAC/efectos: **PENDIENTES**.
- `PROTOCOL_SPEC`, `SECURITY`, `API`, `MIGRATION` V5: **PENDIENTES**, sin alterar V4 por este backlog.
- Patch completo: **RECHAZADO para el patch histórico**; solo generar uno contra el árbol actual y validarlo.

### `prompt2.md` — comparación V5

- No asumir TLV, UDP o protocolo propio: **INTEGRADO como regla de investigación**.
- Comparar TLV, CBOR, MessagePack, nanopb, FlatBuffers, JSON, CoAP, MQTT-SN, DTLS, OSCORE, TLS, Noise y AEAD: **PENDIENTE**; falta investigación efectiva con fuentes, licencias, benchmarks y restricciones ESP8266.
- Proponer al menos tres arquitecturas y matriz de decisión: **PENDIENTE**.
- Permitir concluir “no crear protocolo propio”: **NO_DECIDIBLE** hasta la comparación.
- Modelo de amenazas, provisioning, rotación, revocación, secure boot, OTA y rollback: **PENDIENTE**.
- Ejemplos alarma/sensor/robot/GPS/relé/gateway: **INTEGRADOS como casos de validación**, no como perfiles implementados todos.

## Contradicciones corregidas al consolidar

1. Las rutas históricas `emisor_pir_v4`/`receptor_central_v4` se sustituyen en el estado actual por `emisor_pir_unificado`/`receptor_central_unificado`; V3 se conserva en `legacy/`.
2. `AlarmProfile.h` es la fuente del vocabulario `EventCode`, `DeviceType` y `StateTag`; no se deben agregar nuevos eventos de alarma a `IoTProtocol.h`.
3. El código actual de `IoTNode` autentica antes de efectos internos relevantes; la documentación anterior que lo marcaba como todavía tardío queda supersedida, pero siguen pendientes pruebas de integración y hardware.
4. El código actual conecta el `BOOT_ID` persistente de `IoTStorage` al arranque de los firmwares unificados; siguen pendientes pruebas de LittleFS, reinicio y fallback.
5. HMAC autentica e integra; no cifra. AES-GCM es una decisión condicional, no una tarea V4 automática.
6. El wire V4 se mantiene congelado; las decisiones de codec/transporte/seguridad V5 no modifican V4 sin una versión mayor y migración.

## Destinos canónicos finales

- Arquitectura y estado: `docs/ARCHITECTURE.md`, `docs/ANALISIS_INICIAL_HALLAZGOS.md`.
- Bugs y evidencia: `docs/BUGS_FIXED.md`.
- Plan y fases: `docs/PLAN_EJECUCION_FUTURA.md`.
- Backlog: `docs/ROADMAP.md` y `docs/universal-protocol/CAPABILITY_DISCOVERY.md`.
- Operación y pruebas: `docs/OPERATIONS.md`.
- Comparación/decisión V5: pendiente; no inventar resultados.
- Procedencia de los diez drafts retirados: este archivo.

## Condición de eliminación

Los diez drafts pueden eliminarse solo porque este documento conserva para cada uno: inventario, hallazgos, clasificación, estado, destino, contenido rechazado y trabajo pendiente. La eliminación no implica que las tareas pendientes estén implementadas.
