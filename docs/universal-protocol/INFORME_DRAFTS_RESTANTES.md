# Informe de auditoría — cinco drafts restantes

## Estado y propósito

- **Fecha:** 2026-08-31.
- **Estado:** auditoría documental completada; implementación pendiente salvo donde se indique evidencia en el código actual.
- **Fuentes auditadas:** `_drafts/1mejoras.md`, `_drafts/BUGS_FIXED.md`, `_drafts/META_PROMPT.md`, `_drafts/bugs.md` y `_drafts/ideas.md`.
- **Documento relacionado:** [`INFORME_UNIFICACION.md`](INFORME_UNIFICACION.md), que audita los cuatro drafts de diseño del protocolo universal.
- **Código funcional modificado durante esta auditoría:** ninguno.

Este informe evita que las cinco fuentes se pierdan o se interpreten como un conjunto de tareas ya implementadas. Cada contenido se clasifica contra el código y la documentación canónica actuales, se le asigna un destino y se conserva la incertidumbre cuando no existe una prueba reproducible.

Los drafts originales permanecen en `_drafts/` como evidencia histórica. No se eliminan ni se aplican sus bloques de código automáticamente.

## 1. Criterio de evidencia y estados

La documentación, un changelog o un patch histórico no prueban por sí solos que una función exista o funcione. Para este informe se usan estas clasificaciones:

- **INTEGRADA:** el principio o fix ya aparece en código/documentación actual; todavía puede requerir verificación funcional.
- **MEJORA:** corrige o amplía una capacidad existente y tiene destino claro.
- **NUEVA:** no se identificó un equivalente actual.
- **VARIANTE:** propone otra estrategia o calendario para un problema ya identificado.
- **DUPLICADA:** ya está cubierta sin aportar una decisión nueva.
- **CONTRADICTORIA:** el draft afirma algo que contradice el código o la evidencia disponible.
- **PENDIENTE:** no hay evidencia suficiente para elegir o declarar resuelto.
- **FUERA DE ALCANCE:** es metodología documental, topología externa no demostrada o una instrucción que no pertenece al firmware universal.

Los estados de implementación se mantienen separados de la clasificación:

```text
PROPUESTO → APLICADO → COMPILADO → VERIFICADO → VERIFICADO EN HARDWARE
```

Una propuesta puede estar integrada en el roadmap sin estar aplicada. Una corrección puede estar aplicada en el código y seguir sin verificación de MQTT, Home Assistant o hardware.

## 2. Inventario de fuentes

| Fuente | Tamaño leído | Función | Resultado de la auditoría |
|---|---:|---|---|
| `_drafts/1mejoras.md` | 230 líneas | Propuesta de sirena V3 no bloqueante | Mejora válida; el bloque histórico no debe copiarse sin corregir rollover de `millis()` |
| `_drafts/BUGS_FIXED.md` | 142 líneas | Registro histórico BUG-001…BUG-010 | Conserva causas y reglas, pero declara resueltos varios bugs que no están verificados |
| `_drafts/META_PROMPT.md` | 159 líneas | Meta-prompt para generar documentación | Metodología absorbida; no es especificación del protocolo |
| `_drafts/bugs.md` | 98 líneas | BUG-011 y diagnóstico de red/HA | El fix de ArduinoJson está en el código; MQTT/HA y la topología siguen sin prueba registrada |
| `_drafts/ideas.md` | 473 líneas | Backlog de seguridad, diagnóstico, perfiles y evolución | Fuente principal de mejoras futuras; requiere separar núcleo, central, perfil alarma y operación |

## 3. Matriz de cobertura por draft

| Draft | Hallazgos principales | Clasificación dominante | Destino canónico | Estado real |
|---|---|---|---|---|
| `1mejoras.md` | Sirena MOTION intermitente, `Buzzer::isBusy()`, parámetros configurables y MQTT respetando la sirena | MEJORA | `PLAN_EJECUCION_FUTURA.md`, Fase 8 | PROPUESTO; no aplicado ni probado |
| `BUGS_FIXED.md` | Diez causas y soluciones históricas | INTEGRADA / CONTRADICTORIA | `docs/BUGS_FIXED.md`, `PLAN_EJECUCION_FUTURA.md` sección 4 | Variado por bug; ver registro canónico |
| `META_PROMPT.md` | Flujo para generar arquitectura, changelog, bugs, roadmap y README | INTEGRADA / FUERA DE ALCANCE | Método de `ANALISIS_INICIAL_HALLAZGOS.md` y reglas del plan | Absorbido como metodología; no ordena commit/push automático |
| `bugs.md` | BUG-011 de `as<JsonObject>()`, propuesta de changelog y topología B622/DMZ | INTEGRADA / PENDIENTE / FUERA DE ALCANCE | `docs/BUGS_FIXED.md`, `ANALISIS_INICIAL_HALLAZGOS.md` | Fix visible en código; integración MQTT/HA y red no verificadas |
| `ideas.md` | Seguridad, estado, event log, máquina de estados, zonas, config, OTA, capabilities, telemetría y dashboard | MEJORA / NUEVA / VARIANTE | `ROADMAP.md`, futuras fases V4/V5 y perfiles | Backlog; no implementado como conjunto |

## 4. Auditoría detallada de `1mejoras.md`

### 4.1 Contenido conservado

El draft propone extender el receptor V3 con una sirena no bloqueante:

- `Buzzer::sirenOn(onMs, offMs, totalMs)` para alternar aproximadamente 200 ms encendido y 200 ms apagado durante 4 segundos.
- `Buzzer::isBusy()` para distinguir “el pin está apagado durante una fase” de “el patrón todavía está activo”.
- `timedOn(500)` se conserva para el timbre.
- `mqtt_cliente.cpp` debe consultar `isBusy()` para no iniciar reconexiones durante una sirena.
- Los tiempos deben centralizarse en `config.cpp` y no duplicarse en varios archivos.

La idea resuelve una diferencia funcional real entre MOTION y TIMBRE y respeta la regla de no bloquear UDP.

### 4.2 Corrección necesaria antes de aplicar

El bloque histórico usa comparaciones como `ahora >= deadline`. Para temporizadores basados en `millis()`, la implementación futura debe usar una comparación segura frente a rollover:

```cpp
if ((long)(millis() - deadline) >= 0) {
    // cambiar de fase
}
```

También deben decidirse antes de editar el firmware:

1. Si un TIMBRE interrumpe, encola o ignora una sirena.
2. Si una nueva MOTION reinicia o extiende el patrón.
3. Cómo funciona un apagado manual.
4. Si MQTT considera ocupado el patrón completo o solo la fase audible.
5. Qué prioridad tendrán futuros eventos críticos.

### 4.3 Trazabilidad y destino

- **Clasificación:** MEJORA.
- **Destino:** Fase 8 de `docs/PLAN_EJECUCION_FUTURA.md`.
- **Estado:** PROPUESTO.
- **No afirmar:** que `sirenOn()` o `isBusy()` existen en el código actual.
- **Prueba mínima:** MOTION durante 4 segundos, TIMBRE simultáneo, UDP procesándose durante el patrón y rollover simulado de `millis()`.

## 5. Auditoría de `BUGS_FIXED.md`

El draft es valioso como registro de síntomas, causas y reglas preventivas, pero su título “Bugs Resueltos” no debe transferirse literalmente al registro canónico. BUG-008 y BUG-010 mezclan un fix histórico parcialmente aplicado con un endurecimiento posterior que sigue pendiente:

- Para BUG-008, `IoTNode` ya genera un `BOOT_ID` distinto por arranque; lo que falta es conectar el contador persistente de `IoTStorage` y verificar la política completa.
- Para BUG-010, la decisión de auth está centralizada en `auth.verifyPacket()`/`_required`, pero la verificación llega tarde: el núcleo puede actualizar registry, ACK o deduplicación antes de que el callback rechace el paquete.

El registro canónico separa explícitamente esas dos capas para no perder el fix histórico ni declarar terminado el hardening pendiente.

Los diez registros se conservan con estado individual en [`docs/BUGS_FIXED.md`](../BUGS_FIXED.md). La fuente histórica no se borra; se corrige su interpretación mediante estados de evidencia.

### 5.1 Clasificación resumida

| Bug | Clasificación | Estado canónico | Acción restante |
|---|---|---|---|
| BUG-001 MQTT bloquea UDP | INTEGRADA parcialmente | APLICADO parcialmente; no verificado exhaustivamente | Medir conexión y mantener MQTT fuera de la ruta crítica |
| BUG-002 PIR/TIMBRE se bloquean | INTEGRADA en diseño V3 | APLICADO en diseño; hardware pendiente | Prueba casi simultánea en emisor y receptor |
| BUG-003 PIR sostenido/antirebote | INTEGRADA parcialmente | APLICADO parcialmente | Verificar duración real del HC-SR501 y política de flancos |
| BUG-004 macro `LOCAL` | INTEGRADA | APLICADO | No reintroducir el identificador |
| BUG-005 OTA/firewall | INTEGRADA como procedimiento | APLICADO documentalmente; no verificado | Probar desde el PC real |
| BUG-006 `connect()` bloqueante | INTEGRADA como regla | APLICADO como prevención | No usar TCP connect como health check |
| BUG-007 repetidor/B622/UDP | PENDIENTE | PROPUESTO; no verificado | Medir topología, gateway y rutas |
| BUG-008 BOOT_ID | INTEGRADA parcialmente / MEJORA pendiente | APLICADO parcialmente; persistencia PROPUESTA y pendiente V4 | Conectar el contador persistente a `IoTNode` y verificar reinicios |
| BUG-009 `AUTH_KEY` histórica | INTEGRADA preventivamente | APLICADO preventivo; auditoría histórica pendiente | Revisar Git y rotar si hubo exposición |
| BUG-010 auth unificada/temprana | INTEGRADA parcialmente / MEJORA pendiente | Decisión única APLICADA sin verificación; auth temprana PROPUESTA | Verificar auth del núcleo antes de ACK, registry y deduplicación |

## 6. Auditoría de `META_PROMPT.md`

### 6.1 Contenido integrado

El meta-prompt exige documentar arquitectura, changelog, bugs, roadmap y README con paths, síntomas, causas, reglas, instrucciones y criterios de verificación. Ese método ya está representado por:

- `docs/ANALISIS_INICIAL_HALLAZGOS.md`, que define hechos, inferencias, decisiones e ideas `IDEA-XXX`.
- `docs/PLAN_EJECUCION_FUTURA.md`, que define fases, gates y orden de ejecución.
- `docs/universal-protocol/META_PROMPT.md`, que define la auditoría y trazabilidad para el protocolo universal.
- `docs/BUGS_FIXED.md`, que conserva el registro de bugs con estados de evidencia.

### 6.2 Contenido no adoptado literalmente

La instrucción de hacer commit y push automáticamente se clasifica como **FUERA DE ALCANCE / RECHAZADA como regla general**. Publicar cambios requiere revisión del usuario y validación del resultado. Tampoco se usa este meta-prompt como autorización para declarar una función implementada solo porque aparece en un documento.

- **Clasificación:** DUPLICADA/INTEGRADA como metodología.
- **Destino:** referencias metodológicas existentes.
- **Estado:** FUERA DE ALCANCE técnico.

## 7. Auditoría de `bugs.md`

### 7.1 BUG-011 — discovery MQTT y ArduinoJson

El draft identifica correctamente el riesgo de construir un documento JSON vacío con `doc.as<JsonObject>()` en vez de inicializarlo con `doc.to<JsonObject>()`. El código actual contiene la forma corregida:

```cpp
JsonObject o = doc.to<JsonObject>();
```

Por tanto:

- **Clasificación:** INTEGRADA EN CÓDIGO.
- **Estado:** APLICADO; MQTT retained/Home Assistant no verificado en esta auditoría.
- **Destino:** `docs/BUGS_FIXED.md` como BUG-011.
- **No afirmar aún:** una versión V3.5.2 publicada ni que Home Assistant registró las entidades.
- **Prueba requerida:** inspeccionar el payload retained real en el broker y verificar las entidades en Home Assistant.

Los comandos de `mosquitto_sub`, la configuración de topics y cualquier procedimiento de validación pertenecen a operación/pruebas, no al núcleo universal.

### 7.2 Topología B622, doble NAT y DMZ

La topología descrita —IPs, B622, DMZ, NAS y rutas entre subredes— no está demostrada por el repositorio. Se conserva como hipótesis operativa pendiente, no como hecho de arquitectura ni como configuración del protocolo.

- **Clasificación:** PENDIENTE / FUERA DE ALCANCE del protocolo universal.
- **Acción:** medir gateways, DHCP, subredes, rutas, broker y accesibilidad UDP antes de escribirlo en `ARCHITECTURE.md` o cambiar firmware.
- **Regla:** no copiar IPs o afirmar doble NAT basándose solo en el draft.

## 8. Auditoría de `ideas.md`

### 8.1 Seguridad y confiabilidad

| Idea | Clasificación | Destino | Estado |
|---|---|---|---|
| HMAC uniforme en mensajes importantes | MEJORA | Fases 3–5 del plan; núcleo universal | Pendiente de política y pruebas |
| Anti-replay | MEJORA | Fase 4 | Pendiente de semántica UDP y ventana |
| `BOOT_ID` persistente | MEJORA | Fase 2 | Pendiente de conectar y probar |
| `SEQ` robusto | INTEGRADA parcialmente | Fases 2–4 | La estructura existe; semántica completa pendiente |
| Auth obligatoria para comandos | MEJORA | Fase de `COMMAND`/`CONFIG` | Pendiente de flujo extremo a extremo |
| OTA protegido y rollback | MEJORA | Roadmap posterior a V4 | No implementado ni verificado |
| Watchdog y recuperación WiFi/MQTT | INTEGRADA parcialmente | Fase hardware/operación | Watchdog V4 existe; recuperación completa pendiente |
| Registro de eventos | NUEVA | Backlog central | No existe como event log persistente de 100 entradas |

### 8.2 Modelo de estado y diagnóstico

| Idea | Clasificación | Destino | Estado |
|---|---|---|---|
| Estado completo: ID, nombre, IP, RSSI, uptime, BOOT_ID, firmware, errores | MEJORA | Registry/telemetría V4 | Hay campos parciales; falta contrato completo |
| Heartbeat enriquecido | INTEGRADA parcialmente | V4.2/telemetría | Estructura documentada; validar valores y consumidores |
| Telemetría MQTT | MEJORA | Adapter MQTT/HA | Parcial; necesita schema y pruebas retained |
| Dashboard de diagnóstico | NUEVA | Home Assistant/operación | No existe como dashboard completo |
| Persistencia de estado en central | MEJORA | Roadmap posterior a V4 | No implementada |

### 8.3 Dominio de alarma

Estas ideas pertenecen al perfil de aplicación alarma, no al núcleo universal:

| Idea | Clasificación | Destino | Estado |
|---|---|---|---|
| Estados `DISARMED`, `ARMING`, `ARMED`, `TRIGGERED`, `ALARMING`, `ACKNOWLEDGED` | MEJORA | Perfil alarma, después de estabilizar V4 | No implementado como máquina completa |
| Modos `HOME`, `AWAY`, `NIGHT`, `MAINTENANCE` | MEJORA | Perfil alarma/configuración | No implementado como contrato universal |
| Zonas (sala, cocina, dormitorio, garaje, patio) | NUEVA | Perfil alarma | No implementado; nombres son ejemplos, no catálogo aprobado |
| Priorización por zona y modo | NUEVA | Perfil alarma | Pendiente de requisitos y pruebas |

No se deben introducir `zona`, `ARM_HOME` o estados de alarma en el Core solo porque aparecen en el draft. Deben modelarse como mensajes o adapter de perfil con contrato separado.

### 8.4 Configuración, capacidades y evolución

| Idea | Clasificación | Destino | Estado |
|---|---|---|---|
| Configuración centralizada persistente | MEJORA | `COMMAND`/`CONFIG` y `IoTStorage` | Existe configuración básica, falta flujo completo y auth temprana |
| Añadir sensores sin cambiar el protocolo | INTEGRADA como objetivo | Perfiles y tipos extensibles | Objetivo arquitectónico; no demostrado por una segunda familia real |
| Capability discovery | MEJORA | `HELLO`/perfil universal | Tipos de discovery existen; contrato completo pendiente |
| Convertir `IoTProtocol` en protocolo propio estándar | VARIANTE | Evaluación V5 | No adoptar automáticamente; comparar codec, transporte y seguridad |
| Relé y comandos | MEJORA | Perfil actuador | Tipos existen; flujo central→nodo→respuesta incompleto |

### 8.5 Roadmap histórico

El calendario `V4.3.1 → V4.4 → V4.5 → V5` se conserva como **VARIANTE**, no como compromiso de versión. El calendario canónico es el orden de gates del plan:

```text
línea base → tests host → BOOT_ID → auth temprana → anti-replay
→ simulador → hardware → mejoras V3 → perfiles → evaluación V5
```

Una futura versión solo se nombra cuando sus criterios de aceptación están definidos y la versión anterior tiene evidencia suficiente.

## 9. Matriz de ideas futuras y decisión de integración

| ID | Idea consolidada | Fuente | Clasificación | Destino | Dependencias | Criterio mínimo |
|---|---|---|---|---|---|---|
| D-001 | Sirena V3 no bloqueante | `1mejoras.md` | MEJORA | Fase 8 | Decidir concurrencia; no tocar V4 | MOTION/TIMBRE simultáneos sin bloquear UDP |
| D-002 | Registro canónico de bugs con estados de evidencia | `BUGS_FIXED.md`, `bugs.md` | INTEGRADA | `docs/BUGS_FIXED.md` | Verificar código y pruebas | Ningún “resuelto” sin estado y evidencia |
| D-003 | BUG-011 ArduinoJson | `bugs.md` | INTEGRADA EN CÓDIGO | BUG-011 | Broker y HA | Retained válido y entidades descubiertas |
| D-004 | Event log de últimos 100 eventos | `ideas.md` | NUEVA | Roadmap central | Definir RAM/flash y schema | Insertar, reiniciar y consultar sin perder orden |
| D-005 | Máquina de estados de alarma | `ideas.md` | MEJORA | Perfil alarma | Definir transiciones y permisos | Tabla de transiciones probada |
| D-006 | Zonas y modos de armado | `ideas.md` | NUEVA | Perfil alarma | Estado de alarma y config auth | Mensaje de zona/mode validado |
| D-007 | Configuración remota completa | `ideas.md` | MEJORA | `COMMAND`/`CONFIG` | Auth temprana, persistencia y rollback | Set/get autenticado y durable |
| D-008 | OTA con integridad y rollback | `ideas.md` | MEJORA | Plataforma/operación | Firma, watchdog, recuperación | Imagen inválida rechazada; fallo recuperable |
| D-009 | Capability discovery | `ideas.md` | MEJORA | Core/perfil | Contrato HELLO y unknown fields | Central identifica capacidades reales |
| D-010 | Telemetría y dashboard | `ideas.md` | MEJORA/NUEVA | Adapter MQTT/HA | Schema estable, discovery probado | Estado visible y retained validado |
| D-011 | Evaluación de protocolo universal propio | `ideas.md` | VARIANTE | `INFORME_UNIFICACION.md`, V5 | Benchmarks y requisitos | Matriz comparativa antes de elegir |

## 10. Contenido rechazado, no incorporado o fuera de alcance

| Contenido | Decisión | Motivo |
|---|---|---|
| Aplicar directamente el código de `1mejoras.md` | RECHAZADO por ahora | Usa comparaciones de tiempo inseguras y no define concurrencia |
| Declarar BOOT_ID persistente resuelto | RECHAZADO por ahora | El `BOOT_ID` por sesión existe, pero el contador persistente no está conectado ni verificado |
| Declarar auth temprana resuelta | RECHAZADO por ahora | La decisión única está aplicada parcialmente, pero la verificación ocurre después de efectos internos |
| Declarar V3.5.2 publicado | RECHAZADO por ahora | BUG-011 no tiene verificación MQTT/HA registrada |
| Copiar B622/DMZ/IPs a arquitectura | RECHAZADO por ahora | Topología externa no medida en este repositorio |
| Convertir zonas y modos de alarma en campos obligatorios del Core | RECHAZADO | Son conceptos del perfil alarma |
| Commit/push automático desde un meta-prompt | RECHAZADO como regla | La publicación requiere revisión humana |
| Eliminar los cinco drafts | RECHAZADO | Son fuentes históricas y contienen backlog aún no integrado |

## 11. Destinos y orden de ejecución

La integración queda distribuida así:

1. `docs/BUGS_FIXED.md`: hechos históricos, síntomas, causas, reglas y estado real de BUG-001…BUG-011.
2. `docs/PLAN_EJECUCION_FUTURA.md`: gates técnicos y Fase 8 de sirena; no se modifica firmware como parte de esta auditoría.
3. `docs/ROADMAP.md`: backlog de event log, estados, zonas, capabilities, telemetría, configuración y OTA, con dependencias.
4. `docs/ANALISIS_INICIAL_HALLAZGOS.md`: método de futuras ideas y referencia a este informe.
5. `docs/universal-protocol/INFORME_UNIFICACION.md`: referencia a esta auditoría complementaria.
6. `_drafts/`: se conserva como evidencia de procedencia y no como fuente de estado actual.

## 12. Checklist de cierre de esta auditoría

- [x] Los cinco drafts fueron leídos completos.
- [x] Cada draft tiene inventario y clasificación.
- [x] Cada mejora tiene destino, dependencia y criterio mínimo.
- [x] Se separaron hechos del código, propuestas, hipótesis de red y metodología.
- [x] BUG-008 y BUG-010 no se declaran resueltos.
- [x] BUG-011 se declara aplicado en código, pero no verificado en MQTT/HA.
- [x] La sirena no se aplica automáticamente.
- [x] Las ideas de alarma quedan fuera del núcleo universal.
- [x] Los cinco drafts originales se conservan.
- [ ] Compilación, tests host, MQTT, OTA y hardware: no ejecutados en esta auditoría.

## Resultado

Los cinco drafts no eran basura ni contenido redundante para borrar. Contienen una propuesta concreta de mejora V3, un registro de causas históricas, un método documental, un fix de código ya reflejado y un backlog de evolución de la plataforma. Quedan conservados mediante esta cadena:

```text
fuente histórica → clasificación → evidencia actual → destino canónico → criterio de aceptación
```

La siguiente implementación debe empezar por los gates de `PLAN_EJECUCION_FUTURA.md`, no por copiar un draft completo. Mientras no haya compilación y pruebas, las mejoras futuras deben permanecer como backlog explícito y no como funcionalidades declaradas.
