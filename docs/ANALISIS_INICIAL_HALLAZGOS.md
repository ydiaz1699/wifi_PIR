# Análisis inicial y registro de hallazgos

> Snapshot auditable del análisis estático realizado el 2026-08-31. Este documento conserva lo observado en el repositorio y define cómo comparar ideas futuras del usuario con las soluciones existentes antes de implementar cambios.

## 1. Propósito y alcance

Este documento existe para que una sesión futura pueda retomar el análisis sin depender de la memoria de una conversación. No sustituye al código ni al [plan de ejecución futura](PLAN_EJECUCION_FUTURA.md):

- el código real es la fuente de verdad del comportamiento implementado;
- `docs/PLAN_EJECUCION_FUTURA.md` es la guía de ejecución por fases;
- este archivo conserva el diagnóstico, la trazabilidad de los hallazgos y el método para evaluar nuevas ideas;
- la auditoría específica de los cinco drafts restantes y su backlog está en [`docs/universal-protocol/INFORME_DRAFTS_RESTANTES.md`](universal-protocol/INFORME_DRAFTS_RESTANTES.md);
- `docs/ARCHITECTURE.md`, `CHANGELOG.md` y `ROADMAP.md` describen arquitectura, historia y trabajo previsto, respectivamente.

El análisis original fue estático. No se ejecutaron compilaciones, pruebas host, flasheos, pruebas de red, MQTT, OTA ni pruebas con hardware. Toda afirmación de funcionamiento real debe verificarse antes de marcarla como confirmada.

## 2. Inventario de fuentes analizadas

Se revisaron completamente o en el contexto necesario:

```text
README.md
docs/ARCHITECTURE.md
docs/CHANGELOG.md
docs/ROADMAP.md
docs/PLAN_EJECUCION_FUTURA.md
network_config.h
secrets.h.template
emisor_pir/platformio.ini
emisor_pir/include/*
emisor_pir/src/*
emisor_pir_v4/platformio.ini
emisor_pir_v4/include/*
emisor_pir_v4/src/*
receptor_bocina/platformio.ini
receptor_bocina/include/*
receptor_bocina/src/*
receptor_central_v4/platformio.ini
receptor_central_v4/include/*
receptor_central_v4/src/*
lib/IoTProtocol/*
```

El análisis se centró en las rutas de ejecución, no solo en nombres o comentarios: arranque, lectura de sensores, serialización, transmisión UDP, ACK, reintentos, deduplicación, autenticación, persistencia, MQTT, bocina, discovery, heartbeat, registry y configuración remota.

## 3. Resumen ejecutivo

El repositorio mantiene dos líneas paralelas:

| Línea | Directorios | Estado | Modelo |
|---|---|---|---|
| V3.5.1 | `emisor_pir/`, `receptor_bocina/` | Producción | UDP textual, ACK asíncrono, modo LOCAL/HA |
| V4.3 | `lib/IoTProtocol/`, `emisor_pir_v4/`, `receptor_central_v4/` | Desarrollo | UDP binario, TLV, CRC16, reliable, heartbeat, HMAC, LittleFS |

La decisión de conservación es importante: **V3 debe permanecer estable mientras V4 se valida y endurece**. No se debe reescribir producción para acelerar el prototipo.

La conclusión inicial fue:

1. V3 tiene una ruta de alarma operativa y resiliente frente a la caída de MQTT, pero conserva límites de pérdida de eventos y de concurrencia de la bocina.
2. V4 tiene una base de protocolo más general y reusable, pero varias propiedades declaradas en la documentación todavía no están conectadas completamente en el código.
3. Las prioridades técnicas son: integrar `BOOT_ID` persistente, autenticar antes de producir efectos, cerrar los flujos de mensajes V4, probar sin hardware y solo después añadir nuevas capacidades.

## 4. Arquitectura reconstruida

```text
PIR / timbre
    ↓
emisor ESP8266
    ↓ UDP en LAN, puerto 4210
receptor o central ESP8266
    ├── bocina + LED
    └── MQTT opcional → Home Assistant
```

### 4.1 V3.5.1

El emisor detecta PIR y timbre de forma independiente, transmite inmediatamente y registra hasta cuatro eventos en vuelo. Los ACK se procesan en segundo plano y los eventos se reenvían después de un timeout hasta el máximo previsto.

El receptor drena varios datagramas por ciclo, responde ACK incluso a duplicados, deduplica mediante una ventana por emisor y activa la aplicación solo para eventos nuevos. MQTT funciona en modo dual LOCAL/HA y UDP conserva prioridad sobre MQTT.

El wire format observado es:

```text
Emisor → receptor:  DEVICE_ID|EVENT_ID|TIPO
Ejemplo:            PIR01|5|TIMBRE
Receptor → emisor:  OK|EVENT_ID
```

### 4.2 V4.3

`IoTProtocol` define una cabecera binaria, payload TLV, CRC16, secuencia, `BOOT_ID`, flags y tipos de mensaje. `IoTNode` añade cola con prioridades, un canal reliable en vuelo, reintentos con backoff, ACK automático, deduplicación, heartbeat, registry y estados `ONLINE`, `STALE` y `OFFLINE`.

El emisor V4 usa `IoTStorage`, `IoTNode`, `IoTConfigHandler` e `IoTAuth`. La central V4 recibe eventos, heartbeat, `HELLO`, datos y reportes de estado, y los publica por MQTT. Sin embargo, `COMMAND`, `CONFIG`, `RESPONSE` y `HELLO_ACK` todavía no forman un flujo de aplicación completo de extremo a extremo.

## 5. Matriz de hallazgos

La matriz separa comportamiento observado de solución propuesta. Las propuestas no deben considerarse implementadas hasta que el código y las pruebas lo confirmen.

| ID | Área | Evidencia / comportamiento observado | Consecuencia | Estado | Próxima comprobación |
|---|---|---|---|---|---|
| H-001 | `BOOT_ID` | `IoTStorage::getBootId()` genera un contador persistente, pero `IoTNode::begin()` genera otro valor pseudoaleatorio y los firmwares V4 no pasan el contador persistente | La deduplicación no distingue de forma fiable un reinicio usando la fuente diseñada para ello | Pendiente V4 | Integrar `storage.getBootId()` en `node.begin(bootId)` y probar dos arranques consecutivos |
| H-002 | Autenticación | HMAC se verifica en callbacks de aplicación, después de que el núcleo puede actualizar registry, responder ACK y tocar deduplicación | Un paquete no autenticado puede producir efectos internos antes de ser rechazado | Pendiente V4 | Mover la validación al núcleo antes de ACK, registry y deduplicación |
| H-003 | Cobertura HMAC | `STATE_REPORT` puede firmarse condicionalmente; eventos, `HELLO` y heartbeat no se firman uniformemente | La política de seguridad no es coherente para todos los mensajes | Pendiente V4 | Definir qué tipos requieren HMAC y probar emisor/central |
| H-004 | Persistencia | La documentación habla de `config.json`, pero `IoTStorage` usa líneas `key=value` | Una futura herramienta puede leer o escribir un formato equivocado | Divergencia documental | Elegir un formato, alinearlo y añadir prueba de roundtrip |
| H-005 | `auth.key` | Existe API de clave en LittleFS, pero la ruta real usa `IOT_AUTH_KEY` de `secrets.h` y no integra claramente la clave persistida | La provisión y rotación de claves no están definidas | Parcial / pendiente | Decidir una única fuente de clave y documentar migración |
| H-006 | V4 aplicación | Existen tipos `COMMAND`, `CONFIG`, `RESPONSE` y `HELLO_ACK`, pero no hay flujo completo entre MQTT, central y nodo | La extensibilidad declarada no equivale aún a funcionalidad operativa | Parcial | Implementar un caso extremo a extremo, por ejemplo relé o configuración |
| H-007 | V4 recepción | `_processIncoming()` procesa un datagrama por llamada, mientras V3 usa drain loop | Una ráfaga puede acumular retrasos en V4 | Riesgo no medido | Medir tiempo de loop y decidir drain loop acotado |
| H-008 | V3 pérdida sin WiFi | El emisor descarta directamente un evento detectado cuando no hay WiFi | Se pierde una alarma durante una interrupción de conectividad | Limitación conocida | Decidir buffer persistente, RAM o aceptar explícitamente la pérdida |
| H-009 | V3 cola llena | Si los cuatro slots en vuelo están ocupados, el paquete puede salir pero no quedar registrado para ACK/reintento | La garantía de entrega deja de ser uniforme bajo ráfaga | Limitación conocida | Crear prueba de cinco eventos simultáneos y definir política |
| H-010 | V3 deduplicación | La deduplicación usa IP/event ID y no tiene `BOOT_ID` | Tras reinicio puede coincidir con una ventana anterior | Limitación V3 | Determinar si V3 se conserva así o se migra con compatibilidad |
| H-011 | Bocina | `timedOn()` usa un temporizador único; un evento posterior sobrescribe el plazo anterior | PIR y timbre son independientes en red, pero no pueden mantener patrones acústicos concurrentes | Riesgo funcional | Definir cola/prioridad de patrones sin bloquear UDP |
| H-012 | MQTT V3 | Hay declaraciones `static` separadas del contador de fallos dentro de ramas diferentes | El conteo y el reinicio de fallos pueden no afectar a la misma variable | Sospecha de bug | Confirmar alcance de variables y probar caída/recuperación del broker |
| H-013 | Documentación | `README.md` enlazaba `docs/BUGS_FIXED.md`, que no existía en el árbol inicial | Una referencia de continuidad estaba rota | Corregido documentalmente | Mantener `docs/BUGS_FIXED.md` con estados de evidencia y procedencia histórica |
| H-014 | Declaraciones históricas | `CHANGELOG.md` presenta “BOOT_ID persistente” y “auth logic unificada” como completados | El historial puede inducir a confiar en propiedades aún parciales | Divergencia código/documentación | Marcar la implementación real y añadir referencia a este análisis |
| H-015 | MQTT/HA | No existe evidencia en el repositorio de una prueba real completa de discovery retained y entidades Home Assistant | La integración está documentada pero no demostrada | No verificado | Probar broker, discovery, entidades y registrar resultados |
| H-016 | OTA | Hay configuración OTA para receptores, pero no evidencia de prueba en la red real | OTA puede depender de firewall, rutas y configuración externa | No verificado | Probar desde el equipo real y documentar firewall/resultado |
| H-017 | Seguridad | HMAC proporciona autenticidad/integridad, no confidencialidad | Los datos siguen visibles en UDP aunque la autenticación funcione | Propiedad de diseño | No describir HMAC como cifrado; decidir si el cifrado es realmente necesario |
| H-018 | Validación | No se encontraron pruebas host, simulador UDP ni evidencia de compilación en este análisis | No se puede afirmar todavía que todas las variantes compilen o funcionen | Pendiente | Ejecutar primero la línea base de compilación y crear tests aislados |

## 6. Separación entre hechos, inferencias y decisiones

### Hechos comprobados por lectura

- V3 usa protocolo textual; V4 usa protocolo binario.
- V3 es la línea documentada como producción y V4 como desarrollo.
- V3 tiene ACK asíncrono, reintentos, deduplicación y drain loop.
- V4 tiene CRC/TLV, cola, reliable, heartbeat, registry y estados remotos.
- `IoTStorage` implementa contador de arranque persistente.
- Los firmwares V4 llaman a `node.begin()` sin conectar ese contador persistente.
- La autenticación se decide en callbacks de firmware, no completamente dentro de `IoTNode`.
- OTA está configurado en los receptores, no en el emisor V4.
- No se encontró un flujo completo centralizado de `COMMAND`/`CONFIG` desde MQTT hasta un nodo.

### Inferencias o riesgos que requieren prueba

- El tiempo exacto de bloqueo de MQTT depende de las versiones instaladas.
- El impacto de procesar un único datagrama V4 por ciclo depende de la carga real.
- La integración con Home Assistant puede funcionar o no según broker, retained y configuración externa.
- OTA puede funcionar o no según firewall y topología.
- El comportamiento físico de la bocina y la pérdida de eventos requieren hardware o simulación.

### Decisiones aún abiertas

- Política obligatoria u opcional de HMAC durante migración.
- Tamaño de truncamiento HMAC.
- Ventana anti-replay frente a orden estricto.
- Fuente definitiva de claves: `secrets.h`, LittleFS o provisión híbrida.
- Si V3 recibirá mejoras compatibles o permanecerá congelada.
- Si una futura V5 seguirá con protocolo propio o adoptará un estándar.

## 7. Método para incorporar y comparar ideas futuras

Cuando el usuario comparta una idea, no se debe elegir automáticamente entre “la idea existente” y “la idea nueva”. Primero se registra, se compara y se decide con evidencia.

### 7.1 Registro inicial de la idea

Cada idea nueva debe documentarse primero sin reinterpretarla:

```markdown
### IDEA-XXX — Título breve

- Fecha:
- Autor/origen: usuario / análisis / documentación / prueba
- Problema que intenta resolver:
- Propuesta original, sin resumir demasiado:
- Componentes afectados:
- Versión objetivo: V3 / V4 / futura V5 / transversal
- Supuestos declarados:
- Evidencia disponible:
- Preguntas abiertas:
```

La propuesta original debe conservarse aunque después se rechace, se combine o se reformule. Esto evita perder la intuición que motivó la idea.

### 7.2 Clasificación de cada propuesta

Cada idea o subidea se clasifica con una de estas etiquetas:

- **INTEGRADA:** ya existe en el código o documentación y la nueva idea la confirma.
- **MEJORA:** aporta una corrección clara sobre una solución existente.
- **NUEVA:** no hay equivalente identificado en el repositorio.
- **VARIANTE:** resuelve el mismo problema con otra estrategia razonable.
- **CONTRADICTORIA:** rompe una regla, requisito o compatibilidad ya aceptada.
- **DUPLICADA:** repite una solución ya existente sin mejora demostrable.
- **PENDIENTE:** no hay evidencia suficiente para decidir.

Una idea puede contener varias subideas con clasificaciones diferentes. No se debe clasificar toda una propuesta como un bloque si mezcla, por ejemplo, una mejora de seguridad con una decisión de hardware.

### 7.3 Matriz comparativa

Para cada problema se debe completar una tabla equivalente a esta:

| Criterio | Solución actual | Idea del usuario | Alternativa híbrida | Evidencia / observación |
|---|---|---|---|---|
| Problema cubierto |  |  |  |  |
| V3 compatible |  |  |  |  |
| V4 compatible |  |  |  |  |
| Latencia |  |  |  |  |
| Fiabilidad ante pérdida UDP |  |  |  |  |
| Seguridad |  |  |  |  |
| RAM/flash |  |  |  |  |
| Complejidad |  |  |  |  |
| Facilidad de prueba |  |  |  |  |
| Migración |  |  |  |  |
| Diagnóstico/mantenimiento |  |  |  |  |
| Riesgo de regresión |  |  |  |  |

La tabla debe describir diferencias reales. No se debe declarar que una alternativa es “mejor” solo porque sea más moderna o más grande.

### 7.4 Evaluación de madurez

La madurez de una idea se registra por separado de su calidad técnica:

| Nivel | Significado |
|---|---|
| M0 | Intuición o problema observado, sin diseño suficiente |
| M1 | Propuesta entendible, con objetivo y supuestos |
| M2 | Diseño con flujo, límites, compatibilidad y casos de error |
| M3 | Prototipo o prueba reproducible |
| M4 | Validada en integración/hardware y documentada |

Una idea M4 no es automáticamente mejor que una M2 si resuelve un problema diferente. La madurez indica evidencia, no preferencia.

### 7.5 Cómo construir una solución híbrida

Una solución híbrida solo debe crearse después de separar responsabilidades:

1. Identificar el mismo problema en las dos propuestas.
2. Conservar de cada una sus propiedades demostrablemente valiosas.
3. Detectar incompatibilidades de wire format, estado, tiempos, memoria y migración.
4. Definir una frontera clara: protocolo, núcleo de transporte, aplicación, hardware, operación o documentación.
5. Elegir una sola fuente de verdad para cada estado.
6. Diseñar compatibilidad hacia atrás cuando afecte a V3.
7. Crear una prueba mínima que diferencie la solución actual, la nueva y la híbrida.
8. Implementar primero en V4 o en un prototipo aislado si existe riesgo para producción.
9. Registrar qué parte fue integrada y qué parte fue descartada, con el motivo.

Ejemplo de criterio: una idea puede conservar el modelo de eventos de V3 por su simplicidad, usar el reliable/CRC de V4 para transporte y añadir una ventana anti-replay independiente. Eso no se acepta por intuición: hay que especificar el wire format, la compatibilidad, el coste y las pruebas.

## 8. Plantilla de decisión para futuras ideas

Usar esta plantilla en una próxima revisión:

```markdown
## IDEA-XXX — [título]

### Propuesta original
[Registrar la idea del usuario con fidelidad]

### Problema objetivo
[Qué falla o qué capacidad falta]

### Alcance
- V3:
- V4:
- V5/futuro:
- Documentación/operación:

### Comparación con lo existente
| Subidea | Clasificación | Equivalente actual | Diferencia | Evidencia |
|---|---|---|---|---|
|  |  |  |  |  |

### Alternativas
1. Solución actual:
2. Solución propuesta:
3. Solución híbrida:

### Riesgos y compatibilidad
- Wire format:
- Estado y reinicios:
- Seguridad:
- RAM/flash:
- Latencia:
- V3:
- OTA/MQTT:

### Decisión
- Estado: integrada / mejora / nueva / variante / contradictoria / duplicada / pendiente
- Madurez: M0 / M1 / M2 / M3 / M4
- Decisión técnica:
- Motivo:
- Prueba que falta:
- Archivos que podrían modificarse:

### Trazabilidad
- Fuentes leídas:
- Código relevante:
- Pruebas ejecutadas:
- Contenido rechazado y motivo:
- Contenido pendiente:
```

## 9. Orden seguro de continuidad

La secuencia recomendada se mantiene en `PLAN_EJECUCION_FUTURA.md`:

```text
1. preservar y compilar la línea V3
2. obtener línea base de compilación V4
3. crear tests host del protocolo
4. integrar BOOT_ID persistente
5. autenticar antes de efectos internos
6. decidir anti-replay y compatibilidad HMAC
7. validar con simulador UDP
8. validar con hardware
9. cerrar flujos COMMAND/CONFIG/RESPONSE
10. añadir sensores o mejoras de bocina
11. evaluar una futura V5 con comparación formal
```

No implementar nuevas ideas directamente sobre V3 si todavía no se ha identificado su impacto. Si una idea parece madura, se compara contra este documento y el plan; si ambas tienen fortalezas diferentes, se diseña un híbrido explícito en vez de reemplazar una por otra sin trazabilidad.

## 10. Registro de ideas futuras

Este registro ya incluye las ideas históricas auditadas. La matriz completa, con procedencia, clasificación, destino, dependencias y criterios de aceptación, está en [`universal-protocol/INFORME_DRAFTS_RESTANTES.md`](universal-protocol/INFORME_DRAFTS_RESTANTES.md). Las ideas nuevas deben seguir la plantilla de la sección 8 y no reemplazar entradas previas.

| ID | Fecha | Título | Clasificación | Madurez | Estado |
|---|---|---|---|---|---|
| D-001…D-011 | 2026-08-31 | Sirena, bugs, event log, estados, zonas, config, capabilities, telemetría, OTA y evaluación V5 | Mixta: integrada / mejora / nueva / variante | M1–M2 según subidea | Backlog documentado; no implica implementación |

## 11. Regla de actualización

Después de cada sesión relevante:

1. Añadir nuevas observaciones a la matriz, sin borrar hallazgos previos.
2. Si un hallazgo cambia, conservar el estado anterior y registrar por qué cambió.
3. No marcar una solución como integrada solo porque esté en un plan.
4. Añadir la prueba o evidencia que permite cambiar `Pendiente` a `Confirmado`.
5. Mantener separadas las ideas del usuario, las decisiones técnicas y el código implementado.
6. Actualizar también `PLAN_EJECUCION_FUTURA.md`, `CHANGELOG.md` o `ROADMAP.md` cuando el cambio corresponda a esos documentos.
7. Revisar `git diff --check` y no modificar código funcional como parte de una actualización puramente documental.
