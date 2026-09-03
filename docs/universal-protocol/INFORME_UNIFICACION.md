# Informe de unificación — protocolo universal para sistemas embebidos

## Estado

- Estado: auditoría de unificación completada; resultado pendiente de revisión humana.
- Fecha: 2026-08-31.
- **Fuentes históricas:** cuatro borradores que fueron consolidados y retirados; su trazabilidad vigente está en [`../DRAFTS_AUDIT.md`](../DRAFTS_AUDIT.md).
- Resultado principal: `docs/universal-protocol/META_PROMPT.md`.
- Código funcional modificado: ninguno.
- **Auditoría complementaria:** el detalle de los diez drafts históricos, incluidos estos cuatro, está en [`../DRAFTS_AUDIT.md`](../DRAFTS_AUDIT.md).

## 1. Alcance de la auditoría

Se analizaron individualmente los cuatro documentos completos:

| Fuente | Líneas | Función principal |
|---|---:|---|
| `_drafts/plantilla de prompt.md` | 1.075 | Meta-prompt general para transformar una idea en especificación profesional |
| `_drafts/prodoco.md` | 867 | Visión arquitectónica por capas y extracción del núcleo genérico |
| `_drafts/prompt.md` | 1.701 | Meta-prompt dirigido a refactorizar `IoTProtocol`, con seguridad, migración, tests y entrega |
| `_drafts/prompt2.md` | 892 | Auditoría independiente, comparación tecnológica y diseño del protocolo universal |

La unificación no se limitó a concatenar documentos ni a elegir el más largo. Se compararon sus funciones, variantes, prescripciones, ejemplos y contradicciones.

## 2. Decisión de estructura

La salida unificada usa esta secuencia:

```text
análisis previo
→ objetivo universal confirmado
→ auditoría del repositorio
→ reconstrucción del comportamiento
→ trazabilidad
→ requisitos
→ separación núcleo/perfiles/adapters
→ comparación tecnológica
→ decisión de arquitectura
→ especificación
→ seguridad
→ API
→ perfiles
→ migración
→ pruebas
→ implementación
→ documentación y gobernanza
```

Esta estructura combina el ciclo general de `plantilla de prompt.md`, la arquitectura de `prodoco.md`, la cobertura técnica de `prompt.md` y la independencia comparativa de `prompt2.md`.

## 3. Matriz de trazabilidad por fuente

### 3.1 `plantilla de prompt.md`

| Contenido | Clasificación | Destino | Decisión |
|---|---|---|---|
| Separar qué se quiere conseguir de cómo se quiere implementar | INTEGRADO | Reglas fundamentales y Fase 0 | Conservado como regla metodológica |
| Convertir idea informal en problema, objetivo, usuarios, entradas, procesamiento y salidas | INTEGRADO | Fases 0 y 5 | Conservado y adaptado al proyecto embebido |
| Preguntar por restricciones, dependencias y supuestos | INTEGRADO | Fases 0 y 5 | Conservado |
| Proponer alternativas antes de elegir arquitectura | INTEGRADO | Fases 6 y 7 | Conservado |
| Roadmap MVP/V1/V2/Futuro | MEJORA | Migración, implementación y roadmap | Se convierte en fases, perfiles y extensiones; no se fuerza “MVP” si no aplica |
| No implementar antes de entender | INTEGRADO | Reglas fundamentales | Conservado |
| Meta-prompt reutilizable para cualquier proyecto | VARIANTE | Alcance | Se limita al diseño de un protocolo universal y sus perfiles, evitando perder el propósito concreto |
| Fórmulas o ejemplos que no pertenecen al protocolo | FUERA DE ALCANCE | Ninguno | Se excluyen del prompt técnico unificado |

### 3.2 `prodoco.md`

| Contenido | Clasificación | Destino | Decisión |
|---|---|---|---|
| Separación aplicación / protocolo / transporte | INTEGRADO | Fase 4 | Se amplía a núcleo, codec, reliability, seguridad, transportes, adapters y perfiles |
| El núcleo no debe conocer PIR, bocina, alarma, MQTT o Home Assistant | INTEGRADO | Fase 4 | Es una regla arquitectónica central |
| Separar Message Type de Application Data | INTEGRADO | Fase 4 y especificación | Conservado; debe verificarse contra el wire format final |
| TLV como decisión preferida | VARIANTE | Fase 6 | No se fija; se compara contra CBOR, nanopb, MessagePack y otras opciones |
| Namespaces/rangos de tags | MEJORA | Fase 8 y gobernanza | Conservado como requisito de evolución, no como formato elegido |
| Versionado y compatibilidad hacia adelante | INTEGRADO | Fases 8 y 14 | Conservado |
| Capability Discovery | INTEGRADO | Fases 4, 8 y 11 | Conservado como capacidad a justificar e implementar si procede |
| Transporte intercambiable | INTEGRADO | Fase 10 | Conservado; se exige no acoplar el núcleo a `WiFiUDP` |
| MQTT listado junto a transportes | CONTRADICTORIO | Fase 10 | Reinterpretado como adapter/gateway salvo evidencia contraria |
| Separar Crypto | INTEGRADO | Fases 4 y 9 | Conservado |
| Sacar `IoTNode` del núcleo o dividirlo | MEJORA | Fase 10 | Convertido en decisión de arquitectura según coste, API y portabilidad |
| Application Profiles | INTEGRADO | Fase 11 | Conservado; alarma es primer perfil |
| No hacer overengineering | INTEGRADO | Reglas y Fase 0 | Conservado como control de alcance |
| Nombres `NodeProtocol`, `UniversalProtocol`, `NodeLink`, `NexusProtocol` | PENDIENTE | Datos del proyecto | No se elige nombre por los borradores |

### 3.3 `prompt.md`

| Contenido | Clasificación | Destino | Decisión |
|---|---|---|---|
| Auditoría previa a una futura versión universal | INTEGRADO | Fases 1–3 | Conservado; `prompt.md` no ordena saltar directamente a V5, sino auditar primero |
| Preservar V3 de producción | INTEGRADO | Reglas y Fase 12 | Conservado como restricción crítica |
| Separar Core y aplicación | INTEGRADO | Fase 4 | Conservado, pero como arquitectura a especificar |
| Core con versión, tipo, flags, origen, destino, SEQ, BOOT_ID, payload, CRC, AUTH, ACK y heartbeat | MEJORA | Requisitos y Fase 8 | Convertido en candidatos/requisitos a validar, no todos obligatorios por defecto |
| Message Type frente a Application Data | INTEGRADO | Fases 4 y 8 | Conservado |
| TLV como formato | VARIANTE | Fase 6 | No se fija antes de comparar |
| Compatibilidad, unknown fields y evolución | INTEGRADO | Fases 6, 8 y 14 | Conservado |
| Capability Discovery | INTEGRADO | Fases 4, 8 y 11 | Conservado |
| `ITransport` y transportes UDP/TCP/serial/ESP-NOW/LoRa | MEJORA | Fase 10 | Conservado como objetivo de adapter; no obliga a implementar todos |
| Security Layer intercambiable | INTEGRADO | Fase 9 | Conservado |
| SEQ, BOOT_ID, ACK, reliability, QoS y prioridades | INTEGRADO | Fases 4, 8 y 13 | Conservado, pero cada semántica debe definirse y probarse |
| Fragmentación, routing, compresión, streaming, multicast y gateway | PENDIENTE | Fases 0, 6 y 8 | Se evalúan según requisitos; no se implementan automáticamente |
| Storage, logging y errores | INTEGRADO | Fases 4, 10, 13 y 14 | Conservado |
| MQTT/Home Assistant fuera del Core | INTEGRADO | Fase 10 y 11 | Conservado como adapter/gateway |
| Plan de testing muy detallado | INTEGRADO | Fase 13 | Conservado y ampliado con criterios de aceptación |
| Wire format, endianess e identidad | INTEGRADO | Fase 8 | Conservado |
| Mención de artefactos V5 y migración V4→V5 | MEJORA | Fases 7, 12 y documentación | Se conserva como posible destino, no como orden de implementación inmediata; `prompt.md` exige auditar y diseñar antes |
| Formato de entrega del código completo | INTEGRADO | Reglas y Fase 15 | Conservado |
| Ejemplos alarma/sensor/robot/GPS/relé/gateway | INTEGRADO | Fase 11 | Conservado como perfiles de validación |
| Incluir credenciales | RECHAZADO | Reglas | Solo referencias sanitizadas; nunca secretos reales |

### 3.4 `prompt2.md`

| Contenido | Clasificación | Destino | Decisión |
|---|---|---|---|
| Auditoría independiente antes del diseño | INTEGRADO | Fases 1–3 | Es la base del flujo |
| No asumir TLV, UDP, MQTT, protocolo propio o arquitectura actual | INTEGRADO | Objetivo y Fase 6 | Conservado, compatible con el objetivo universal |
| Comparar CBOR, MessagePack, nanopb, FlatBuffers y otros | INTEGRADO | Fase 6 | Conservado con evaluación por requisitos |
| Comparar UDP, TCP, CoAP, MQTT-SN, DTLS, OSCORE y otros | INTEGRADO | Fase 6 | Conservado sin obligar a implementarlos todos |
| Tres o más arquitecturas candidatas | INTEGRADO | Fase 6 | Conservado |
| Matriz de decisión con criterios explicados | INTEGRADO | Fase 7 | Conservado |
| Si no conviene protocolo propio, poder usar combinación de estándares | MEJORA | Objetivo confirmado y Fase 6 | Reformulado: el objetivo es una plataforma universal; la composición puede ser propia, estándar o híbrida |
| Diseño detallado posterior a la decisión | INTEGRADO | Fases 7–10 | Conservado |
| API, perfiles, migración y pruebas | INTEGRADO | Fases 10–13 | Conservado |
| Seguridad desde modelo de amenazas | INTEGRADO | Fase 9 | Conservado |
| Concluir sin implementar código gigante | INTEGRADO | Reglas y Fase 15 | Conservado |
| Entrega con auditoría, problemas, comparación, especificación y roadmap | INTEGRADO | Formato obligatorio | Conservado y ampliado |
| Investigación con documentación oficial y fuentes confiables | MEJORA | Fase 6 y gobernanza | Añadido explícitamente al unificado con URL, versión/fecha y afirmación respaldada |
| Evaluación de secure boot, firma de firmware, OTA y rollback | MEJORA | Fase 9 y criterios de plataforma | Añadido explícitamente; se clasifica como requisito de plataforma o fuera de alcance justificado |

## 3.5 Matriz de claims críticos y evidencia de destino

Esta matriz complementa las tablas por fuente. No pretende reemplazar la revisión del repositorio: indica cómo cada afirmación crítica quedó representada en el resultado unificado.

La trazabilidad es por afirmación, requisito o decisión crítica, no por cada frase literal de los 4.535 renglones. El material editorial repetido se consolidó por tema; si se requiere una auditoría línea por línea, debe hacerse en una revisión posterior antes de declarar que no existe ninguna pérdida textual.

| ID | Claim o decisión | Fuente/sección | Clasificación | Destino en unificado | Estado posterior |
|---|---|---|---|---|---|
| R-001 | Separar objetivo de mecanismo | `plantilla de prompt.md`, reglas iniciales | INTEGRADO | Reglas fundamentales | Confirmado como regla metodológica |
| R-002 | Convertir idea informal en requisitos | `plantilla de prompt.md`, reconstrucción de idea | INTEGRADO | Fase 0 y Fase 5 | Requiere datos del proyecto |
| R-003 | Aplicación separada del núcleo | `prodoco.md`, secciones 1–4 | INTEGRADO | Fase 4 | Decisión arquitectónica objetivo |
| R-004 | Message Type separado de Application Data | `prodoco.md`, sección 4; `prompt.md`, sección 6 | INTEGRADO | Fases 4 y 8 | Requiere wire format |
| R-005 | TLV como opción | `prodoco.md`, sección 5; `prompt.md`, sección 7 | VARIANTE | Fase 6 | No elegido |
| R-006 | Codec estándar como alternativa | `prompt2.md`, comparación tecnológica | INTEGRADO | Fase 6 | Requiere fuentes y benchmarks |
| R-007 | Versionado y unknown fields | `prodoco.md`, secciones 6–7; `prompt.md`, secciones 8 y 30 | INTEGRADO | Fase 8 y gobernanza | Pendiente de formato |
| R-008 | Capability Discovery | `prodoco.md`, sección 8; `prompt.md`, sección 9 | MEJORA | Fases 4, 8 y 11 | Requiere decidir si entra en V1 |
| R-009 | Transportes intercambiables | `prodoco.md`, secciones 9, 11–12; `prompt.md`, secciones 10, 20 y 22 | INTEGRADO | Fase 10 | Falta contrato de adapter |
| R-010 | MQTT/Home Assistant fuera del núcleo | `prompt.md`, sección 25 | INTEGRADO | Fases 4, 10 y 11 | Contrato MQTT pendiente |
| R-011 | MQTT como posible transporte | `prodoco.md`, sección 1 | CONTRADICTORIO | Fase 10 | Resolver con criterios de broker/gateway |
| R-012 | Seguridad modular | `prodoco.md`, sección 10; `prompt.md`, sección 11 | INTEGRADO | Fase 9 | Requiere modelo de amenazas |
| R-013 | SEQ y BOOT_ID | `prompt.md`, sección 12; `prodoco.md`, secciones 6–7 | INTEGRADO | Fases 8, 9 y 13 | Pendiente de semántica formal |
| R-014 | ACK y reliability configurables | `prompt.md`, secciones 13–15; `prompt2.md`, diseño detallado | INTEGRADO | Fases 4, 8 y 13 | Pendiente de contrato |
| R-015 | QoS, prioridades y backpressure | `prompt.md`, secciones 14–15 | MEJORA | Fases 4 y 13 | Condicional a requisitos |
| R-016 | Storage desacoplado | `prompt.md`, sección 18; `prodoco.md`, sección 12 | INTEGRADO | Fases 4 y 10 | Pendiente de API |
| R-017 | Perfiles de aplicación | `prodoco.md`, sección 14; `prompt.md`, sección 24 | INTEGRADO | Fase 11 | Alarma es primer perfil |
| R-018 | Comparar tres o más arquitecturas | `prompt2.md`, Fase 3 | INTEGRADO | Fase 6 | Requiere matriz puntuada |
| R-019 | Investigación oficial y fuentes actuales | `prompt2.md`, Fase 2 | INTEGRADO | Fase 6 y gobernanza | Añadido tras revisión |
| R-020 | Secure boot, firma y rollback | `prompt2.md`, seguridad; `prompt.md`, sección 11 | INTEGRADO | Fase 9 | Plataforma/fuera de alcance explícito |
| R-021 | Migración sin romper producción | `prompt.md`, reglas 3, 39–45 | INTEGRADO | Fase 12 | Requiere bridge/coexistencia |
| R-022 | Tests de codec, seguridad, fiabilidad y compatibilidad | `prompt.md`, sección 28; `prompt2.md`, entrega | INTEGRADO | Fase 13 | Criterios cuantitativos pendientes |
| R-023 | Código completo y patch aplicable | `prompt.md`, secciones 45–46 | INTEGRADO | Fase 15 | Se aplica solo a implementación posterior |
| R-024 | Libertad para comparar y rechazar tecnologías | `prompt2.md`, reglas iniciales y Fase 4 | INTEGRADO | Objetivo y Fase 6 | Compatible con objetivo universal |

## 4. Variantes equivalentes comparadas

| Tema | Variante A | Variante B | Decisión unificada |
|---|---|---|---|
| Enfoque | Generalizar una idea inicial | Auditar y cuestionar la solución | Auditar primero y diseñar el protocolo universal después |
| Objetivo | Meta-prompt reusable para cualquier proyecto | Protocolo universal para embebidos | Mantener el objetivo concreto y usar la metodología general |
| Codec | TLV | Codec estándar o híbrido | Comparar y decidir por requisitos |
| Transporte | UDP/TCP/serial/LoRa/ESP-NOW/MQTT como lista común | Separar transporte de integración | Adapters de transporte; MQTT/HA como adapter/gateway salvo decisión justificada |
| Arquitectura | Tres capas | Core + codec + reliability + seguridad + adapters + perfiles | Adoptar la división ampliada |
| Evolución | V5 directa | Auditoría, decisión y migración gradual | Migración gradual; V5 solo si el contrato lo requiere |
| Implementación | Empezar a escribir código tras la propuesta | Tests y especificación antes de implementación | Especificación mínima y pruebas primero |
| Seguridad | HMAC/AEAD como opciones | Modelo de amenazas + provisioning + anti-replay | Modelo de amenazas antes de elegir mecanismo |
| Aplicación | Alarma como contexto | Alarma como primer perfil | Alarma fuera del núcleo y usada como validación inicial |

## 5. Contenido integrado

Se integró:

- el método de pasar de idea informal a especificación;
- la auditoría completa del repositorio;
- la diferencia entre objetivo y mecanismo;
- la preservación de V3;
- la extracción del núcleo genérico;
- la separación por capas;
- la comparación de codecs, transportes y seguridad;
- el diseño de reliability y semántica de entrega;
- la identidad, secuencia y anti-replay;
- el descubrimiento y las capabilities;
- la API portable;
- los perfiles de aplicación;
- la migración V3/V4;
- las pruebas host, simulación y hardware;
- la documentación, gobernanza y compatibilidad;
- el formato completo de salida del auditor.

## 6. Contenido integrado como pendiente, no como decisión

Se conservaron sin elegir todavía:

- TLV frente a codec estándar;
- UDP frente a otros transportes;
- HMAC frente a AEAD y modelo de claves;
- tamaño de tags y campos;
- `BOOT_ID` y política anti-replay;
- fragmentación;
- routing;
- compresión;
- streaming;
- multicast/broadcast;
- storage requerido por el Core;
- implementación de `ITransport` mediante virtuales, templates o callbacks;
- nombre definitivo de la librería;
- alcance de la primera versión;
- relación exacta entre protocolo, codec y adapter MQTT.

## 7. Contenido rechazado o reformulado

| Contenido | Estado | Motivo |
|---|---|---|
| Tratar TLV como decisión ya aprobada | REFORMULADO | El objetivo universal no exige un codec concreto |
| Saltar directamente a V5 | REFORMULADO | Primero hay que fijar requisitos, contrato y compatibilidad |
| Tratar MQTT como transporte equivalente a UDP | REFORMULADO | MQTT incorpora broker y semántica de integración |
| Llamar universal al `IoTProtocol` actual | RECHAZADO | El código actual contiene tipos de alarma y depende de ESP8266/WiFiUDP |
| Tratar los nombres de ejemplo como API final | RECHAZADO | Son ilustraciones, no decisiones del proyecto |
| Implementar todas las capacidades enumeradas en la primera versión | RECHAZADO | Riesgo de overengineering; debe haber perfiles y alcance |
| Incluir secretos o credenciales reales | RECHAZADO | Seguridad y reproducibilidad |
| Mantener cuatro prompts paralelos como instrucciones simultáneas | RECHAZADO | Produce contradicciones y deriva de decisiones |

> **Actualización 2026-09-02:** las fuentes históricas fueron consolidadas en `docs/DRAFTS_AUDIT.md` y retiradas. El código actual conecta BOOT_ID persistente y autenticación temprana en `IoTNode`; siguen pendientes compilación de firmware, integración, MQTT/HA, OTA y hardware. El check host posterior está documentado en `docs/OPERATIONS.md`.

## 8. Fuera de alcance de esta unificación

No se hizo lo siguiente:

- modificar `lib/IoTProtocol` como parte de esta auditoría;
- aplicar el patch histórico ausente;
- decidir el wire format final V5;
- cambiar HMAC de 4 a 8 bytes;
- crear `ITransport` real;
- implementar V5;
- compilar o probar el firmware unificado en hardware;
- publicar cambios en GitHub.

La trazabilidad de la retirada de los cuatro drafts de diseño está en `docs/DRAFTS_AUDIT.md`.

## 9. Decisiones pendientes que requieren revisión humana

1. ¿El nombre final será `IoTProtocol`, `NodeProtocol`, `UniversalProtocol`, `NodeLink` u otro?
2. ¿La primera versión universal se limitará a ESP8266/ESP32 y host, o incluirá desde el principio serial/ESP-NOW/LoRa?
3. ¿El codec será TLV, CBOR, nanopb, otro o una combinación?
4. ¿La reliability será parte obligatoria del Core o un módulo/perfil?
5. ¿Qué amenazas debe cubrir la primera versión?
6. ¿HMAC es suficiente para el entorno o se necesita AEAD?
7. ¿Cómo se provisionan, rotan y revocan las claves?
8. ¿Qué semántica exacta tendrá `BOOT_ID`?
9. ¿Cuál es el tamaño de la ventana anti-replay?
10. ¿Qué significa ACK en cada tipo de mensaje?
11. ¿Habrá bridge V3/V4 o solo coexistencia temporal?
12. ¿Qué requisitos cuantitativos fijan la primera versión?
13. ¿Qué perfiles se implementan después del perfil alarma?
14. ¿Qué parte de MQTT pertenece a un gateway/adaptador?
15. ¿Qué evidencia permite declarar la migración exitosa?

## 10. Verificación del resultado

Comprobaciones realizadas:

- Los cuatro archivos fuente fueron leídos en un worktree aislado del commit indicado.
- Se registró el tamaño y la función de cada fuente.
- Se compararon sus encabezados y fases.
- Se revisaron duplicaciones y contradicciones conceptuales.
- Se generó una matriz fuente → contenido → clasificación → destino.
- El documento unificado contiene un bloque `🧠 ANÁLISIS PREVIO` obligatorio.
- El objetivo de protocolo universal quedó confirmado y no fue reemplazado por una decisión de “no crear protocolo”.
- Las tecnologías concretas quedaron como decisiones justificables, no como elecciones silenciosas.
- No se modificó código funcional.

## 11. Resultado

La unificación no elimina la idea de construir un protocolo universal. La hace operable:

```text
objetivo universal confirmado
+ auditoría independiente
+ extracción del núcleo
+ comparación tecnológica
+ decisión explícita
+ especificación normativa
+ perfiles de aplicación
+ migración segura
+ pruebas reproducibles
```

El resultado unificado debe revisarse antes de marcarlo como canónico. Mientras existan decisiones pendientes en codec, seguridad, transportes, wire format, compatibilidad y requisitos cuantitativos, debe conservar el estado `borrador`.
