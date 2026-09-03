# META-PROMPT — AUDITORÍA, DISEÑO Y MIGRACIÓN HACIA UN PROTOCOLO UNIVERSAL PARA SISTEMAS EMBEBIDOS

## Estado

- Estado: borrador unificado para revisión humana
- Fecha: 2026-08-31
- Objetivo: transformar el conocimiento disperso de cuatro borradores en un único meta-prompt operativo.
- Alcance: auditar el proyecto existente, extraer un núcleo reutilizable, diseñar el protocolo universal, definir perfiles de aplicación y planificar la migración sin romper la versión de producción.
- No implica: que el protocolo, el wire format, la seguridad o la implementación estén aprobados.

---

## 🧠 ANÁLISIS PREVIO DE FRAGMENTOS

### 1. Material recibido

- Fragmentos: 4
- **Fuentes históricas:** cuatro documentos que antes estaban en `_drafts/` (`plantilla de prompt.md`, `prodoco.md`, `prompt.md` y `prompt2.md`).
- Tamaño total aproximado: 4.535 líneas y 77.937 caracteres.
- Tipo de material:
  - un meta-prompt general para convertir ideas en especificaciones;
  - una visión arquitectónica por capas;
  - un meta-prompt dirigido a refactorizar `IoTProtocol`;
  - un meta-prompt de auditoría y diseño comparativo.

### 2. Tema central detectado

Diseñar un protocolo y una librería universales para sistemas embebidos, partiendo del proyecto `wifi_PIR`, separando el núcleo de comunicación de la aplicación de alarma y permitiendo su reutilización en sensores, actuadores, robots, telemetría, automatización, gateways y otros dispositivos.

### 3. Versiones duplicadas o superpuestas

Los cuatro documentos tienen el mismo propósito general, pero cumplen funciones diferentes:

- `plantilla de prompt.md` aporta el ciclo general idea → requisitos → alternativas → arquitectura → especificación → plan → implementación.
- `prodoco.md` aporta la visión arquitectónica y la extracción de un núcleo independiente de la alarma.
- `prompt.md` aporta una lista exhaustiva de áreas técnicas, reglas de preservación, migración, pruebas y entrega.
- `prompt2.md` aporta una auditoría comparativa menos sesgada hacia TLV, UDP y protocolo propio.

La versión unificada conserva esas cuatro funciones en ese orden: entender, auditar, comparar, diseñar, migrar e implementar.

### 4. Contradicciones detectadas

1. `prompt2.md` pide no asumir TLV, UDP, MQTT ni protocolo propio; `prompt.md` y `prodoco.md` presentan TLV, `SEQ`, `BOOT_ID` y un Core propio como dirección preferida.
2. `prompt2.md` permite concluir que no conviene crear un protocolo propio; el objetivo confirmado del proyecto sí es llegar a un protocolo universal. La unificación resuelve esto permitiendo un protocolo universal compuesto por estándares, una capa propia o un híbrido: el objetivo se conserva, pero no se fuerza una tecnología concreta.
3. `prodoco.md` coloca MQTT junto a UDP, TCP, serial, LoRa y ESP-NOW como transporte; la versión unificada trata MQTT/Home Assistant como integración o gateway salvo que la auditoría demuestre otra necesidad.
4. `prompt.md` exige muchas capacidades desde el núcleo; `plantilla de prompt.md` pide evitar imponer arquitectura antes de entender el problema. La versión unificada separa requisitos obligatorios, capacidades opcionales y extensiones futuras.
5. `prompt.md` nombra V5 como una posible versión y propone artefactos de migración, pero también exige auditar y diseñar antes de implementar. La versión unificada conserva esa disciplina: usa migración gradual y no considera una versión universal aprobada hasta cerrar requisitos, wire format y pruebas.
6. Los ejemplos de nombres (`NodeProtocol`, `UniversalProtocol`, `NodeLink`, `NexusProtocol`) no son decisiones. Se conservan como ejemplos y se deja pendiente el nombre definitivo.

### 5. Información que ya está completa

Los fragmentos cubren suficientemente:

- la necesidad de separar aplicación, núcleo, codec, seguridad y transporte;
- la obligación de leer y auditar el repositorio real;
- la preservación de la producción actual;
- las áreas que debe cubrir una especificación embebida;
- la necesidad de comparar alternativas;
- la migración progresiva;
- los ejemplos de perfiles de aplicación;
- las pruebas de protocolo, seguridad, fiabilidad y compatibilidad;
- la necesidad de documentar decisiones y no solo generar código.

### 6. Brechas detectadas

Quedan pendientes y el meta-prompt debe obligar a resolverlas con evidencia:

- requisitos cuantitativos de nodos, tráfico, latencia, pérdida, RAM, Flash y energía;
- frontera exacta de lo que significa universal;
- wire format normativo;
- codec definitivo;
- semántica de ACK, ejecución, respuesta y retransmisión;
- ventana anti-replay y persistencia de contadores;
- modelo de amenazas y provisioning;
- autenticación, autorización y cifrado;
- API de transporte sin acoplamiento a Arduino/ESP8266;
- compatibilidad entre V3, V4 y la futura versión universal;
- gobernanza de versiones, tags, tipos y perfiles;
- resultados de compilación, tests, benchmarks y hardware-in-the-loop.

### 7. Valores que deben parametrizarse

El auditor debe extraer del repositorio, no inventar:

- nombres y directorios reales;
- boards y plataformas;
- transporte actual;
- puertos e interfaces;
- límites de payload;
- tamaños de buffers;
- timeouts y reintentos;
- claves y secretos, sin imprimirlos;
- versiones de dependencias;
- métricas observadas.

Los valores que no estén confirmados deben aparecer como `⚠️ PENDIENTE`.

### 8. Orden de ejecución identificado

```text
1. Definir alcance y conservar el estado de producción.
2. Inventariar y leer el repositorio completo.
3. Reconstruir el comportamiento real.
4. Separar hechos, documentación, propuestas y brechas.
5. Extraer requisitos universales y requisitos del perfil alarma.
6. Comparar alternativas tecnológicas.
7. Elegir una arquitectura universal justificada.
8. Especificar wire format, API, seguridad y perfiles.
9. Definir compatibilidad y migración.
10. Crear pruebas y fixtures antes de implementar cambios amplios.
11. Implementar primero el núcleo y después un perfil.
12. Validar con host, simulador y hardware.
13. Migrar gradualmente sin eliminar V3 prematuramente.
14. Documentar resultados y actualizar la especificación.
```

### 9. Acción

- [x] Proceder con una unificación estructural porque el objetivo del usuario está confirmado.
- [x] Mantener las contradicciones como decisiones pendientes o alternativas explícitas.
- [x] No declarar elegida ninguna tecnología que los fragmentos no hayan demostrado.
- [x] Separar diseño universal de implementación inmediata.

---

# TEXTO DEL META-PROMPT UNIFICADO

Copia desde esta línea hasta el final cuando quieras encargar a otro agente la auditoría y el diseño del protocolo universal.

---

## ROL

Actúa como arquitecto principal de sistemas embebidos, protocolos de comunicación, redes, seguridad, librerías C/C++ y sistemas distribuidos con recursos limitados.

Debes tener experiencia práctica o analizar con rigor:

- ESP8266, ESP32, Arduino y PlatformIO;
- C y C++ embebido;
- protocolos binarios y serialización;
- TLV, CBOR, MessagePack, Protocol Buffers/nanopb u otros codecs;
- UDP, TCP, serial, ESP-NOW, LoRa, CoAP, MQTT y gateways;
- ACK, retransmisión, secuencia, duplicados, orden y pérdida;
- autenticación, integridad, confidencialidad, replay y provisioning;
- diseño de APIs y adapters;
- compatibilidad hacia atrás y evolución de protocolos;
- pruebas host, simulación, fuzzing y hardware-in-the-loop;
- límites de RAM, Flash, CPU, energía y tamaño de paquete.

Tu trabajo no es limitarte a corregir el código actual ni convertir automáticamente la implementación existente en una V5. Tu objetivo es diseñar, con evidencia, un protocolo y una librería universales para sistemas embebidos, partiendo del proyecto existente y preservando lo que funciona.

## OBJETIVO CONFIRMADO

El objetivo del proyecto es transformar el conocimiento y la implementación parcial de `wifi_PIR` en una base universal reutilizable para sistemas embebidos.

El resultado debe poder soportar perfiles de aplicación diferentes, por ejemplo:

```text
ALARMA
SENSORES
TELEMETRÍA
ACTUADORES
ROBÓTICA
AUTOMATIZACIÓN
DOMÓTICA
GPS
VEHÍCULOS
GATEWAYS
```

El objetivo universal sí está decidido. Lo que todavía debe decidirse mediante auditoría es la composición tecnológica concreta:

- protocolo propio;
- protocolo estándar;
- codec estándar con capa propia;
- transporte estándar con modelo propio;
- combinación híbrida.

No confundas la decisión de construir una plataforma universal con la obligación de conservar TLV, UDP, MQTT o cualquier otra tecnología actual.

## REGLAS FUNDAMENTALES

1. No implementes antes de terminar la auditoría y la especificación mínima.
2. Lee el repositorio real y usa el código como fuente de verdad del comportamiento.
3. No afirmes que una característica existe solo porque aparezca en un comentario, changelog, prompt o roadmap.
4. Distingue siempre:
   - `CONFIRMADO`: observado en código o prueba reproducible;
   - `PARCIAL`: existe una parte, pero falta integración;
   - `PROPUESTO`: aparece en un documento, pero no está implementado;
   - `NO VERIFICADO`: falta prueba;
   - `PENDIENTE`: falta una decisión o dato;
   - `DESCARTADO`: se examinó y se rechazó con motivo.
5. No conviertas una propuesta en una decisión silenciosamente.
6. No borres ni reemplaces la producción actual mientras la futura versión no haya demostrado compatibilidad suficiente.
7. No mezcles el perfil de alarma con el núcleo universal.
8. No inventes requisitos, benchmarks, archivos, APIs, resultados de prueba ni compatibilidad.
9. No uses secretos reales en documentación, logs, fixtures o ejemplos.
10. Si falta información crítica, detén la implementación y marca `⚠️ PENDIENTE`.
11. Si hay dos alternativas técnicamente válidas, compáralas y documenta la decisión; no ocultes la variante perdedora.
12. Todo código o configuración que se proponga debe ser completo cuando llegue la fase de implementación. Nunca omitas contenido de un archivo mediante marcadores de truncamiento.

## FASE 0 — DEFINIR EL ALCANCE UNIVERSAL

Antes de analizar la solución, define qué se entiende por universal en este proyecto.

Debes responder:

- ¿Qué tipos de nodos debe soportar la primera versión?
- ¿Qué capacidades pertenecen al núcleo y cuáles a un perfil?
- ¿Qué plataformas son obligatorias inicialmente?
- ¿Qué transportes son obligatorios, opcionales o futuros?
- ¿Qué tamaño máximo de dispositivo y presupuesto de recursos se deben respetar?
- ¿Se requiere comunicación local, gateway, nube o todas?
- ¿Qué queda explícitamente fuera de la primera versión?

No conviertas “universal” en “soporta todo desde el día uno”. Define un núcleo pequeño y extensible, con perfiles y adapters.

La fase debe terminar con una definición verificable de la primera versión:

```text
UNIVERSAL_V1:
  plataformas:
  perfiles obligatorios:
  transportes obligatorios:
  codecs admitidos:
  seguridad mínima:
  tamaño máximo de paquete:
  presupuesto RAM/Flash:
  latencia y pérdida aceptables:
  funciones fuera de alcance:
```

Cada campo debe tener una fuente, una decisión del usuario o `⚠️ PENDIENTE`. No se puede pasar a la decisión de arquitectura mientras plataformas, perfiles, recursos y criterios de aceptación críticos sigan indefinidos.

## FASE 1 — INVENTARIO COMPLETO DEL PROYECTO

Lee el repositorio completo de forma progresiva sin saturar el contexto.

Incluye:

- árbol de archivos;
- documentación;
- configuraciones de build;
- librerías;
- firmware;
- tests;
- scripts;
- ejemplos;
- secretos y plantillas, sin exponer secretos;
- dependencias;
- historial relevante si está disponible.

Para cada archivo o grupo registra:

| Campo | Contenido |
|---|---|
| Ruta | Ruta real |
| Tipo | firmware, librería, configuración, documento, test, script |
| Responsabilidad | Qué hace |
| Entradas | Qué recibe |
| Salidas | Qué produce |
| Dependencias | Qué necesita |
| Conexiones | Quién lo llama o consume |
| Estado | confirmado/parcial/propuesto/no verificado |
| Relevancia universal | núcleo/perfil/adapter/operación/descartar |

No des por cerrado el inventario si existen dependencias transitivas que no fueron revisadas.

## FASE 2 — RECONSTRUIR EL COMPORTAMIENTO REAL

Reconstruye los flujos completos:

1. Arranque y configuración.
2. Lectura de sensores.
3. Creación de mensajes.
4. Serialización.
5. Transporte.
6. Recepción.
7. Validación estructural.
8. Autenticación.
9. ACK.
10. Deduplicación.
11. Retransmisión.
12. Despacho a la aplicación.
13. Respuesta de ejecución.
14. Persistencia.
15. Heartbeat y discovery.
16. MQTT o gateway.
17. OTA.
18. Reinicio y recuperación.

Para cada flujo indica qué ocurre realmente, qué afirma la documentación y dónde divergen.

Explica especialmente:

- si el ACK confirma recepción, autenticación, aceptación o ejecución;
- qué sucede con duplicados;
- qué sucede con paquetes fuera de orden;
- qué sucede al reiniciar un nodo;
- qué sucede si se pierde WiFi o el broker;
- qué sucede cuando las colas se llenan;
- qué sucede si el almacenamiento falla;
- qué sucede si la clave es incorrecta;
- qué operaciones pueden bloquear el loop.

## FASE 3 — MATRIZ DE TRAZABILIDAD

Antes de diseñar, crea una matriz completa:

| ID | Afirmación, requisito o idea | Fuente | Tipo | Estado real | Destino | Decisión/motivo |
|---|---|---|---|---|---|---|
| R-001 |  |  | requisito/idea/regla/código |  | núcleo/perfil/adapter/pendiente |  |

Cada hallazgo debe clasificarse como:

- `INTEGRADO`: pasa al diseño unificado;
- `MEJORA`: conserva la idea y corrige la solución;
- `NUEVO`: no existía en el material anterior;
- `VARIANTE`: alternativa equivalente que queda comparada;
- `DUPLICADO`: se conserva la versión más completa y se registra la fuente;
- `CONTRADICTORIO`: se mantiene como decisión pendiente o se rechaza con motivo;
- `FUERA DE ALCANCE`: válido, pero no pertenece a esta versión;
- `RECHAZADO`: no se integra por un motivo técnico explícito;
- `PENDIENTE`: no hay evidencia suficiente.

No afirmes que no se perdió información sin mostrar esta matriz.

## FASE 4 — SEPARAR NÚCLEO, PERFILES Y ADAPTERS

Diseña una frontera explícita:

### Núcleo universal

Debe contener únicamente conceptos reutilizables como:

- versión;
- tipo de mensaje;
- identificidad de nodo;
- origen y destino;
- secuencia o correlación;
- flags;
- payload;
- validación estructural;
- versionado;
- errores;
- semántica de entrega;
- capacidades si se demuestran necesarias.

No debe conocer PIR, bocinas, alarmas, relés, Home Assistant ni topics MQTT concretos.

### Codec o serialización

Evaluar y decidir entre TLV, CBOR, MessagePack, nanopb/Protocol Buffers u otra opción. La decisión debe basarse en requisitos medibles:

- tamaño;
- RAM;
- Flash;
- CPU;
- evolución;
- campos desconocidos;
- herramientas;
- depuración;
- compatibilidad;
- seguridad de parseo.

El lenguaje de esta fase debe ser neutral al codec. Si se elige TLV, define namespaces y tags; si se elige otro codec, define su equivalente para extensiones, campos desconocidos y validación. No incluyas pruebas o estructuras específicas de TLV en el plan común sin marcar que son condicionales.

### Reliability

Definir si es parte del núcleo o módulo opcional:

- ACK;
- retransmisión;
- timeout;
- backoff;
- duplicados;
- orden;
- ventana;
- prioridades;
- expiración;
- idempotencia;
- backpressure.

### Seguridad

Definir una interfaz o módulo independiente para:

- integridad;
- autenticación;
- autorización;
- anti-replay;
- confidencialidad;
- provisioning;
- rotación y revocación;
- OTA firmado cuando aplique.

No inventes criptografía. No describas HMAC como cifrado.

### Transportes e integraciones

Definir adapters para los transportes realmente necesarios. No supongas que todos deben implementarse en la primera versión.

MQTT y Home Assistant no deben clasificarse automáticamente como transporte ni automáticamente como simple integración. La auditoría debe decidir y documentar el contrato según el caso de uso:

- broker y sesiones;
- topics o mapeo de recursos;
- QoS y retained messages;
- semántica de entrega;
- correlación entre ACK del protocolo y confirmación MQTT;
- reconexión;
- bridge/gateway;
- límites de tamaño y disponibilidad.

La hipótesis inicial es tratarlos como integración o gateway cuando su semántica de broker no encaje en un transporte de datagramas, pero esa hipótesis queda `PENDIENTE` hasta justificarla. El contrato MQTT queda pendiente de definir con sus garantías de entrega, reconexión y correlación.

### Perfiles

La alarma de `wifi_PIR` debe ser el primer perfil, no el núcleo. Otros perfiles posibles:

- sensor de temperatura;
- puerta;
- relé;
- robot;
- GPS;
- telemetría;
- gateway.

Cada perfil debe definir sus mensajes y estados sin modificar el núcleo.

## FASE 5 — REQUISITOS TÉCNICOS Y NO FUNCIONALES

Extrae del repositorio y pregunta por lo que falte:

- número máximo de nodos;
- frecuencia normal y ráfaga;
- latencia máxima;
- pérdida tolerada;
- tamaño máximo de paquete;
- MTU;
- RAM y Flash;
- CPU;
- consumo y deep sleep;
- tiempo máximo de loop;
- disponibilidad;
- topología;
- NAT, broadcast y gateway;
- compatibilidad de plataforma;
- tiempo de soporte;
- trazabilidad y diagnóstico.

No inventes números. Cada valor debe tener fuente o quedar como `⚠️ PENDIENTE`.

## FASE 6 — COMPARAR ARQUITECTURAS CANDIDATAS

La comparación debe usar documentación técnica actual y fuentes confiables cuando se evalúen tecnologías externas. Para cada fuente registra URL, organización o autor, versión o fecha consultada, licencia cuando sea relevante y la afirmación que respalda. No presentes una recomendación basada solo en conocimiento no trazable. Si no hay acceso a Internet o no se puede verificar una fuente, marca esa parte como `⚠️ PENDIENTE`.

Compara al menos estas familias, ajustándolas al problema real:

1. Núcleo propio con codec propio.
2. Núcleo propio con codec estándar.
3. Protocolo estándar con perfiles propios.
4. Combinación de estándar de transporte, codec y seguridad con una capa de aplicación propia.

Evalúa, cuando sean relevantes:

- TLV;
- CBOR;
- MessagePack;
- nanopb/Protocol Buffers;
- FlatBuffers;
- JSON solo si el presupuesto lo permite;
- UDP;
- TCP;
- serial;
- ESP-NOW;
- LoRa;
- CoAP;
- MQTT/MQTT-SN;
- DTLS;
- OSCORE;
- TLS;
- AEAD;
- Noise u otra alternativa justificada.

No hagas una lista superficial. Para cada alternativa indica:

- qué problema resuelve;
- qué no resuelve;
- coste de integración;
- recursos;
- riesgos;
- interoperabilidad;
- herramientas;
- evolución;
- seguridad;
- depuración;
- compatibilidad con V3/V4;
- decisión.

Usa una matriz con puntuaciones explicadas. No elijas una tecnología por popularidad.

## FASE 7 — DECISIÓN DE ARQUITECTURA UNIVERSAL

La decisión debe producir una arquitectura concreta, no solo una lista.

Documenta:

- objetivo de la versión;
- núcleo;
- codec;
- reliability;
- seguridad;
- transportes;
- adapters;
- perfiles;
- límites;
- extensiones futuras;
- componentes rechazados;
- compatibilidad;
- riesgos aceptados.

Si se elige una combinación de tecnologías, explica la frontera exacta entre ellas.

La salida no puede decir únicamente “usar un protocolo universal”. Debe decir cómo se divide y qué contrato tiene cada capa.

## FASE 8 — ESPECIFICACIÓN DEL PROTOCOLO

Solo después de la decisión, escribe una especificación normativa.

Debe incluir:

- nombre y versión;
- objetivos y fuera de alcance;
- conceptos y vocabulario;
- tipos de mensaje;
- identificadores;
- campos;
- flags;
- secuencias;
- correlación de comandos;
- payload;
- codec;
- header;
- offsets y tamaños;
- endianess;
- límites;
- validación;
- CRC si aplica;
- autenticación;
- cifrado si aplica;
- canonicalización;
- unknown fields;
- errores;
- ACK y respuestas;
- retransmisión;
- duplicados;
- orden;
- replay;
- fragmentación si realmente es necesaria;
- broadcast/multicast;
- discovery;
- capabilities;
- versionado;
- compatibilidad.

No uses términos como “universal”, “seguro”, “reliable” o “extensible” sin definir el comportamiento que los respalda.

## FASE 9 — SEGURIDAD Y MODELO DE AMENAZAS

Define primero el modelo de amenazas:

- atacante en la LAN;
- nodo no autorizado;
- MITM;
- replay;
- DoS;
- captura física;
- extracción de Flash;
- broker comprometido;
- OTA malicioso;
- pérdida de almacenamiento.

Después define:

- identidad del dispositivo;
- autenticación;
- autorización;
- integridad;
- confidencialidad;
- nonce o contador;
- anti-replay;
- provisioning inicial;
- rotación;
- revocación;
- almacenamiento de claves;
- recuperación;
- compatibilidad de transición;
- secure boot, si la plataforma lo soporta y el modelo de amenazas lo requiere;
- firma y verificación de firmware;
- OTA autenticado;
- rollback y recuperación de una actualización fallida.

Separa qué resuelve la librería de protocolo y qué depende de la plataforma, bootloader, almacenamiento seguro o infraestructura de actualización. Si secure boot, firma de firmware u OTA quedan fuera de la primera versión, decláralo como `FUERA DE ALCANCE` con la razón y el riesgo aceptado.

Separar claramente:

```text
HMAC no es cifrado.
CRC no es autenticación.
BOOT_ID no es anti-replay completo.
Deduplicación no es autorización.
ACK no es ejecución exitosa.
```

## FASE 10 — API Y PORTABILIDAD

Diseña una API que permita separar:

- núcleo de protocolo;
- codec;
- reloj;
- almacenamiento;
- logging;
- seguridad;
- transporte;
- aplicación.

Evalúa si las interfaces deben ser virtuales, estáticas, basadas en templates o callbacks, considerando RAM, Flash, fragmentación, testabilidad y facilidad de uso.

La implementación no debe depender obligatoriamente de `WiFiUDP`, `WiFi.status()` o clases específicas de ESP8266 dentro del núcleo universal.

Define:

- ownership de buffers;
- memoria estática/dinámica;
- errores;
- reentrancia;
- callbacks;
- ciclo `begin()/loop()` si aplica;
- adaptación a host para pruebas;
- adaptación a ESP8266/ESP32;
- adaptación a serial u otro transporte elegido.

## FASE 11 — PERFILES DE APLICACIÓN

Define el perfil de alarma primero:

- eventos PIR;
- timbre;
- estado armado/desarmado;
- bocina;
- estados del dispositivo;
- telemetría;
- comandos;
- integración MQTT/Home Assistant como adapter.

Después muestra cómo se añadirían, sin modificar el núcleo:

- temperatura/humedad;
- puerta;
- relé;
- robot;
- GPS;
- gateway.

Cada perfil debe separar:

- message types del núcleo;
- datos propios de aplicación;
- comandos;
- respuestas;
- estados;
- validaciones;
- permisos.

## FASE 12 — MIGRACIÓN DESDE `wifi_PIR`

La migración debe ser gradual.

Clasifica cada componente como:

- conservar;
- adaptar;
- extraer al núcleo;
- mover a perfil alarma;
- reemplazar;
- retirar;
- dejar pendiente.

Mantén la V3.5.1 como producción hasta que la futura versión demuestre:

- compilación;
- recepción de eventos;
- ACK y retransmisión;
- duplicados;
- PIR y timbre simultáneos;
- pérdida de WiFi;
- caída de MQTT;
- reinicio;
- compatibilidad de seguridad;
- comportamiento de la bocina;
- OTA cuando aplique;
- métricas aceptables.

No declares que V3 y V4 son compatibles sin definir un bridge, una ventana de coexistencia o una migración de nodos.

La migración debe producir una matriz concreta:

| Componente | V3 actual | V4 actual | Versión universal | Estrategia | Criterio de salida |
|---|---|---|---|---|---|
| Emisor |  |  |  |  |  |
| Receptor |  |  |  |  |  |
| Codec |  |  |  |  |  |
| Transporte |  |  |  |  |  |
| Seguridad |  |  |  |  |  |
| MQTT/bridge |  |  |  |  |  |

Define además una de estas estrategias, o una combinación justificada:

- bridge V3 ↔ versión universal;
- coexistencia con puertos, tipos o namespaces diferenciados;
- actualización coordinada de todos los nodos;
- compatibilidad de lectura pero no de escritura;
- periodo de transición y fecha de retirada.

La retirada de V3 solo puede aprobarse cuando cada fila tenga evidencia de compilación, pruebas de integración y resultado observado.

## FASE 13 — PLAN DE PRUEBAS

Crea pruebas antes de una migración amplia.

### Codec y protocolo

- roundtrip válido;
- CRC o integridad alterada;
- longitud incorrecta;
- versión incompatible;
- codec/serialización malformada según el formato elegido;
- campos desconocidos;
- límites de payload;
- endianess;
- valores cero y máximos;
- packet fixtures reproducibles.

### Fiabilidad

- ACK duplicado;
- timeout;
- reintento;
- backoff;
- duplicado;
- fuera de orden;
- ventana;
- rollover;
- reinicio;
- cola llena;
- prioridad;
- broadcast;
- pérdida y latencia.

### Seguridad

- clave correcta;
- clave incorrecta;
- paquete alterado;
- autenticación ausente;
- replay;
- nonce/contador repetido;
- rotación;
- autorización;
- tamaño de tag;
- falta de espacio para seguridad;
- OTA no autorizado.

### Plataforma y aplicación

- prueba host;
- simulador UDP o del transporte elegido;
- fuzzing del parser;
- sanitizers cuando sea posible;
- pérdida de WiFi;
- caída del broker;
- watchdog;
- power loss;
- LittleFS/almacenamiento corrupto;
- PIR sostenido;
- PIR y timbre simultáneos;
- hardware-in-the-loop;
- soak test.

Cada prueba debe tener:

- objetivo;
- entrada;
- procedimiento;
- resultado esperado;
- criterio cuantitativo de aprobación;
- resultado observado;
- estado.

## FASE 14 — DOCUMENTACIÓN Y GOBERNANZA

Genera o propone documentos separados:

```text
REQUIREMENTS.md
ARCHITECTURE.md
PROTOCOL_SPEC.md
WIRE_FORMAT.md
SECURITY.md
TRANSPORTS.md
APPLICATION_PROFILES.md
COMPATIBILITY.md
MIGRATION.md
TEST_PLAN.md
ROADMAP.md
```

Define también:

- registro de tipos y tags;
- política de versionado;
- compatibilidad;
- changelog normativo;
- proceso para añadir perfiles;
- proceso para añadir transportes;
- criterios de revisión;
- evidencia mínima para declarar una función implementada;
- registro de fuentes externas: URL, organización/autor, versión o fecha consultada, licencia cuando aplique, fecha de acceso y afirmación respaldada.

Toda recomendación basada en una tecnología externa debe poder reconstruirse desde esa lista de fuentes. Si no se realizó investigación externa, declara la limitación y no presentes la comparación como validada actualmente.

## FASE 15 — IMPLEMENTACIÓN

Solo después de cerrar la auditoría y la decisión:

1. Implementa el Core mínimo.
2. Implementa el codec elegido.
3. Implementa el transporte mínimo.
4. Implementa seguridad según la especificación.
5. Implementa tests host y fixtures.
6. Implementa el perfil alarma.
7. Añade adapters de plataforma.
8. Prueba migración y coexistencia.
9. Valida hardware.
10. Documenta cada resultado.

Antes de modificar archivos:

- muestra el plan de archivos;
- distingue archivos nuevos y modificados;
- conserva cualquier cambio previo del usuario;
- no sobrescribas secretos;
- no produzcas parches incompletos;
- no afirmes compilación sin ejecutarla.

## FORMATO OBLIGATORIO DE LA RESPUESTA DEL AUDITOR

La salida debe seguir este orden:

1. `ANÁLISIS PREVIO`
2. `ALCANCE Y SUPUESTOS`
3. `INVENTARIO DEL REPOSITORIO`
4. `MAPA DE ARQUITECTURA ACTUAL`
5. `FLUJOS REALES`
6. `MATRIZ DE TRAZABILIDAD`
7. `REQUISITOS CONFIRMADOS`
8. `REQUISITOS PENDIENTES`
9. `HALLAZGOS Y RIESGOS`
10. `SEPARACIÓN NÚCLEO/PERFILES/ADAPTERS`
11. `ALTERNATIVAS TECNOLÓGICAS`
12. `MATRIZ DE DECISIÓN`
13. `ARQUITECTURA UNIVERSAL ELEGIDA`
14. `DECISIONES PENDIENTES`
15. `ESPECIFICACIÓN DEL PROTOCOLO`
16. `SEGURIDAD`
17. `API Y PORTABILIDAD`
18. `PERFILES DE APLICACIÓN`
19. `PLAN DE MIGRACIÓN`
20. `PLAN DE PRUEBAS`
21. `PLAN DE IMPLEMENTACIÓN`
22. `DOCUMENTOS Y ARCHIVOS AFECTADOS`
23. `CONTENIDO INTEGRADO, RECHAZADO, FUERA DE ALCANCE Y PENDIENTE`
24. `LIMITACIONES Y EVIDENCIA FALTANTE`

No implementes si quedan decisiones críticas sin clasificar.

## CRITERIOS DE ÉXITO

El trabajo se considera correcto solo si:

- el objetivo de protocolo universal queda cubierto;
- la alarma deja de contaminar el núcleo;
- se conoce qué parte del código actual se reutiliza;
- se comparan alternativas reales;
- la arquitectura se justifica con requisitos;
- el wire format es verificable;
- la seguridad no se describe de forma superficial;
- la compatibilidad y migración están definidas;
- existen pruebas reproducibles;
- V3 no se rompe prematuramente;
- cada afirmación importante tiene fuente o evidencia;
- todos los bloques temáticos identificados tienen integración, variante, rechazo, fuera de alcance o pendiente documentado; una auditoría textual línea por línea sigue siendo una actividad posterior si se requiere preservar cada formulación literal;
- las decisiones pendientes quedan visibles;
- el código se implementa solo después de la revisión humana del diseño.

## DATOS DEL PROYECTO

Completa estos datos con información comprobada del repositorio. No inventes valores:

```text
Repositorio:
Versión de producción:
Versión de desarrollo:
Plataformas:
Transportes actuales:
Codec actual:
Seguridad actual:
Aplicaciones actuales:
Dependencias:
Restricciones de recursos:
Pruebas existentes:
Objetivo de la nueva versión:
```

Si un dato no existe, escribe `⚠️ PENDIENTE`.
