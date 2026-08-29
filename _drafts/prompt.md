Sí. Y en este caso **conviene que el meta-prompt sea muy exigente**, porque si se lo pasas a otro LLM sin contexto, no quiero que simplemente "mejore el código": quiero que **entienda el proyecto existente, preserve lo que funciona y rediseñe el protocolo como una librería independiente y reutilizable**.

Además, haría una modificación importante respecto a nuestra idea anterior: **no obligaría al LLM a saltar directamente a V5**. Primero debe hacer una auditoría del proyecto que le entregues, identificar qué partes de `IoTProtocol` son realmente genéricas y cuáles están contaminadas por la aplicación de alarma, y después proponer la arquitectura definitiva.

Te dejo un meta-prompt listo para usar. Puedes pegarlo junto con el proyecto completo.

# META-PROMPT — DISEÑO Y REFACTORIZACIÓN DE UN PROTOCOLO EMBEBIDO UNIVERSAL

## 0. ROL

Actúa como **arquitecto senior de sistemas embebidos, protocolos de comunicación, redes IoT y librerías C/C++ para microcontroladores**, con experiencia práctica en:

* ESP8266
* ESP32
* Arduino
* PlatformIO
* UDP/TCP
* protocolos binarios
* TLV
* CRC
* ACK/retransmisión
* control de secuencia
* detección de replay
* autenticación
* criptografía aplicada a dispositivos embebidos
* diseño de APIs de librerías
* compatibilidad hacia atrás
* versionado de protocolos
* sistemas distribuidos
* sistemas tolerantes a fallos

Tu trabajo NO consiste simplemente en corregir código.

Tu objetivo es **transformar el protocolo existente de este proyecto en una librería de comunicación genérica, reutilizable y extensible**, capaz de utilizarse posteriormente en proyectos completamente diferentes.

---

# 1. CONTEXTO

Te voy a proporcionar el proyecto completo de un sistema IoT actualmente orientado principalmente a una alarma doméstica.

El proyecto contiene, entre otras cosas:

* emisores PIR
* receptor central
* bocina/alarma
* comunicación UDP
* MQTT
* integración con Home Assistant
* `IoTProtocol`
* `IoTNode`
* `IoTAuth`
* `IoTStorage`
* CRC
* ACK
* retransmisiones
* `SEQ`
* `BOOT_ID`
* TLV
* heartbeat
* deduplicación
* estados ONLINE/STALE/OFFLINE
* configuración remota
* OTA

El proyecto actual funciona como una plataforma de alarma, pero la intención futura es convertir el protocolo en una **librería independiente**, de forma similar a cómo se utilizan librerías externas.

La meta es poder hacer posteriormente algo como:

```cpp
#include <NodeProtocol.h>

NodeProtocol node;

void setup() {
    node.begin();
}

void loop() {
    node.loop();
}
```

y reutilizar la misma librería en proyectos diferentes.

Por ejemplo:

```text
ALARMA
ROBÓTICA
SENSORES
TELEMETRÍA
AUTOMATIZACIÓN
DOMÓTICA
VEHÍCULOS
SISTEMAS EMBEBIDOS
GATEWAYS
```

sin tener que modificar el núcleo del protocolo.

---

# 2. REGLA FUNDAMENTAL

NO asumas que el diseño actual es correcto.

NO asumas que la documentación existente está actualizada.

NO asumas que una característica descrita como "implementada" realmente está implementada.

NO hagas cambios inmediatamente.

Primero debes:

1. estudiar todo el proyecto;
2. comprender su arquitectura;
3. identificar dependencias;
4. identificar qué partes funcionan;
5. identificar problemas;
6. identificar contradicciones entre código y documentación;
7. separar lo genérico de lo específico de la alarma;
8. proponer una arquitectura mejor;
9. justificarla;
10. y solamente después implementar.

---

# 3. REGLA DE PRESERVACIÓN

El sistema actual tiene una versión que funciona en producción.

Por tanto:

> **NO debes romper la implementación actual solamente por conseguir una arquitectura más elegante.**

Debes distinguir claramente entre:

```text
PRODUCCIÓN ACTUAL
```

y:

```text
NUEVA ARQUITECTURA
```

La migración debe poder hacerse progresivamente.

No reemplaces todo de golpe.

---

# 4. OBJETIVO PRINCIPAL

Convertir el actual `IoTProtocol` en una librería verdaderamente genérica.

La librería NO debe conocer conceptos específicos como:

```text
PIR
MOTION
DOORBELL
BOCINA
ALARMA
HOME ASSISTANT
MQTT
```

Esos conceptos pertenecen a la aplicación.

El núcleo debe conocer únicamente conceptos genéricos de comunicación.

---

# 5. SEPARACIÓN ENTRE PROTOCOLO Y APLICACIÓN

Diseña explícitamente dos niveles.

## NIVEL 1 — CORE DEL PROTOCOLO

Debe manejar conceptos como:

```text
VERSION
MESSAGE TYPE
FLAGS
SOURCE
DESTINATION
SEQ
BOOT_ID
PAYLOAD LENGTH
PAYLOAD
CRC
AUTH
ACK
RESPONSE
ERROR
HEARTBEAT
DISCOVERY
```

## NIVEL 2 — APLICACIÓN

Debe manejar conceptos como:

```text
MOTION
DOOR_OPEN
DOOR_CLOSE
TEMPERATURE
HUMIDITY
GPS
RPM
BATTERY
SIREN
ARM
DISARM
MOVE
STOP
```

El Core no debe saber qué significa ninguno de ellos.

---

# 6. MESSAGE TYPE VS APPLICATION DATA

Define claramente esta separación.

Ejemplo:

```text
MESSAGE TYPE:

HELLO
HEARTBEAT
DATA
COMMAND
RESPONSE
ACK
ERROR
CONFIG
DISCOVERY
```

Mientras que:

```text
APPLICATION DATA:

MOTION
TEMPERATURE
GPS
DOOR_OPEN
VOLTAGE
RPM
```

No mezcles ambos conceptos.

El protocolo transporta datos.

La aplicación decide qué significan.

---

# 7. TLV

Evalúa y, si corresponde, conserva el modelo TLV:

```text
TYPE
LENGTH
VALUE
```

Pero crea una separación clara entre:

```text
PROTOCOL TLV
```

y:

```text
APPLICATION TLV
```

Debe existir un mecanismo de namespaces, rangos reservados o equivalente.

Objetivo:

Un proyecto futuro debe poder definir:

```text
TAG_TEMPERATURE
TAG_HUMIDITY
TAG_GPS
```

sin modificar el núcleo del protocolo.

---

# 8. COMPATIBILIDAD

Diseña el protocolo pensando en evolución a largo plazo.

Debe poder existir:

```text
Node A → Protocol 1
Node B → Protocol 1
Node C → Protocol 2
```

siempre que sea técnicamente posible.

Define:

* versionado;
* compatibilidad hacia atrás;
* compatibilidad hacia adelante;
* campos obligatorios;
* campos opcionales;
* comportamiento ante campos desconocidos;
* negociación de capacidades;
* incompatibilidades explícitas.

Regla recomendada:

> Los campos opcionales/desconocidos deben poder ignorarse sin romper el paquete cuando sea seguro hacerlo.

---

# 9. CAPABILITY DISCOVERY

Diseña un mecanismo genérico para que un nodo pueda anunciar:

```text
DEVICE ID
PROTOCOL VERSION
FIRMWARE VERSION
CAPABILITIES
SUPPORTED FEATURES
```

Ejemplo:

```text
DEVICE 17

CAPABILITIES:
    DATA
    COMMAND
    TEMPERATURE
    BATTERY
```

Otro:

```text
DEVICE 23

CAPABILITIES:
    DATA
    GPS
    ACCELEROMETER
```

El Core solo transporta las capacidades.

No debe conocer su significado.

---

# 10. TRANSPORT LAYER

No acoples el protocolo exclusivamente a UDP.

Diseña una abstracción de transporte.

Conceptualmente:

```text
Protocol
    |
    +-- ITransport
          |
          +-- UDP
          +-- TCP
          +-- Serial
          +-- ESP-NOW
          +-- LoRa
          +-- otro
```

El objetivo es poder cambiar el transporte sin cambiar la lógica de aplicación.

Evalúa cuidadosamente qué debe pertenecer al transporte y qué debe pertenecer al protocolo.

No hagas abstracciones innecesarias.

---

# 11. SECURITY LAYER

Separa la seguridad del Core tanto como sea razonable.

Evalúa interfaces para:

```text
NoSecurity
HMAC
AES
AEAD
otro mecanismo apropiado
```

El protocolo debe poder funcionar con diferentes políticas de seguridad sin duplicar toda la lógica.

Analiza:

* autenticación;
* integridad;
* confidencialidad;
* replay protection;
* gestión de claves;
* rotación de claves;
* nonce;
* counters;
* BOOT_ID;
* SEQ;
* amenazas de LAN;
* amenazas de red hostil.

NO implementes criptografía casera.

Usa primitivas existentes y apropiadas para el hardware.

---

# 12. SEQ Y BOOT_ID

Audita profundamente el sistema actual de:

```text
SEQ
BOOT_ID
```

Debes definir formalmente:

* tamaño;
* inicialización;
* persistencia;
* rollover;
* comparación;
* duplicados;
* paquetes fuera de orden;
* replay;
* reinicio del dispositivo;
* pérdida de paquetes;
* retransmisión.

No te limites a implementar una ventana de los últimos N paquetes.

Diseña una política formal de aceptación de secuencia.

---

# 13. ACK Y RELIABILITY

Analiza la implementación actual de:

```text
ACK
retransmisión
timeout
retry
duplicados
```

y rediseñala si es necesario.

Debe existir una separación clara entre:

```text
TRANSMISIÓN
```

y:

```text
SEMÁNTICA DE ENTREGA
```

No asumas que todos los mensajes necesitan ACK.

Permite diferentes políticas:

```text
UNRELIABLE
RELIABLE
RELIABLE_WITH_RESPONSE
```

o un mecanismo equivalente.

---

# 14. QoS

Evalúa si el protocolo necesita niveles de servicio.

Por ejemplo:

```text
BEST_EFFORT
RELIABLE
PRIORITY
```

Pero NO agregues complejidad sin justificación.

Explica si realmente aporta valor para microcontroladores.

---

# 15. PRIORIDADES

Evalúa la existencia de prioridades de mensajes.

Debe poder existir conceptualmente:

```text
CRITICAL
HIGH
NORMAL
LOW
```

pero el Core no debe decidir qué significa "alarma crítica".

Solo debe proporcionar un mecanismo genérico de prioridad.

---

# 16. FRAGMENTACIÓN

Analiza el tamaño máximo actual del paquete.

Determina si el protocolo necesita:

```text
fragmentation
reassembly
```

Si no es necesario, explica por qué no debería implementarse todavía.

No agregues fragmentación solamente "porque podría ser útil".

---

# 17. TIME

Analiza si el protocolo necesita:

```text
timestamp
TTL
timeout
monotonic time
wall-clock time
```

Diferencia cuidadosamente:

```text
millis()/monotonic time
```

de:

```text
UTC timestamp
```

No dependas de RTC/NTP si no es necesario.

---

# 18. STORAGE

`IoTStorage` no debe estar fuertemente acoplado al protocolo.

Evalúa interfaces como:

```text
IStorage
```

para permitir:

```text
LittleFS
EEPROM
NVS
Flash
RAM
```

o incluso ninguna persistencia.

El Core debe poder funcionar sin almacenamiento persistente si el proyecto no lo necesita.

---

# 19. NODE

Evalúa críticamente `IoTNode`.

Determina si debe:

* permanecer;
* dividirse;
* convertirse en una capa opcional;
* separarse del packet engine.

La librería debe poder utilizarse tanto para:

```text
END DEVICE
```

como:

```text
GATEWAY
```

como:

```text
CONTROLLER
```

sin obligar a todos los proyectos a usar exactamente la misma arquitectura.

---

# 20. API

Diseña una API limpia y simple.

El usuario de la librería debería poder hacer algo cercano a:

```cpp
#include <NodeProtocol.h>

NodeProtocol node(config);

void setup() {
    node.begin();
}

void loop() {
    node.loop();
}
```

Enviar:

```cpp
Message msg;
msg.type = MessageType::DATA;

msg.add(...);

node.send(msg);
```

Y recibir:

```cpp
node.onMessage([](const Message& msg) {
    // aplicación
});
```

No obligues al usuario a conocer:

* CRC;
* offsets;
* serialización;
* buffers internos;
* UDP;
* sockets;
* HMAC;
* retransmisiones.

La librería debe ocultar esa complejidad.

---

# 21. MEMORY / EMBEDDED CONSTRAINTS

El diseño debe ser apropiado para:

* ESP8266
* ESP32
* microcontroladores con poca RAM

Evita:

* asignaciones dinámicas innecesarias;
* `String` excesivo;
* buffers gigantes;
* dependencias pesadas;
* excepciones;
* RTTI si no es necesario;
* estructuras difíciles de controlar.

Analiza:

```text
RAM
Flash
stack
heap
fragmentación
tamaño máximo de paquete
buffers
colas
```

Cada decisión arquitectónica debe considerar hardware embebido real.

---

# 22. PORTABILIDAD

El Core idealmente debería depender lo mínimo posible de:

```text
Arduino
ESP8266
ESP32
WiFiUDP
LittleFS
```

Separa:

```text
CORE
```

de:

```text
PLATFORM ADAPTER
```

Objetivo futuro:

```text
Core
 |
 +-- Arduino adapter
 +-- ESP8266 adapter
 +-- ESP32 adapter
 +-- Linux adapter
 +-- otro
```

No es obligatorio implementar todos ahora.

Diseña para que sea posible.

---

# 23. ESTRUCTURA DE LIBRERÍA

Propón una estructura profesional, por ejemplo:

```text
NodeProtocol/
├── include/
│   └── NodeProtocol/
│       ├── NodeProtocol.h
│       ├── Message.h
│       ├── Packet.h
│       ├── Transport.h
│       ├── Security.h
│       └── ...
│
├── src/
│   ├── Packet.cpp
│   ├── Serializer.cpp
│   ├── Reliability.cpp
│   └── ...
│
├── ports/
│   ├── arduino/
│   ├── esp8266/
│   └── esp32/
│
├── transports/
│   ├── udp/
│   ├── serial/
│   └── ...
│
├── examples/
│   ├── BasicNode/
│   ├── Sensor/
│   ├── Gateway/
│   └── ...
│
├── test/
│
├── docs/
│
├── library.json
├── library.properties
└── README.md
```

Puedes proponer una estructura mejor si tienes razones técnicas.

---

# 24. APPLICATION PROFILES

Quiero que evalúes una arquitectura basada en perfiles.

Por ejemplo:

```text
NodeProtocol Core
        |
        +-- Alarm Profile
        +-- Sensor Profile
        +-- Robotics Profile
        +-- Telemetry Profile
```

El Core NO debe conocer esos perfiles.

El proyecto actual de alarma sería simplemente:

```text
NodeProtocol Core
+
Alarm Profile
```

Un futuro robot:

```text
NodeProtocol Core
+
Robotics Profile
```

Un sensor meteorológico:

```text
NodeProtocol Core
+
Sensor Profile
```

Evalúa si esta arquitectura es adecuada y mejórala si es necesario.

---

# 25. HOME ASSISTANT Y MQTT

NO permitas que:

```text
MQTT
Home Assistant
```

entren al Core.

Deben ser adaptadores externos.

La arquitectura debería ser:

```text
Application
     |
NodeProtocol
     |
Transport
     |
MQTT Adapter / UDP / Serial / etc.
```

según corresponda.

El Core jamás debe depender de Home Assistant.

---

# 26. DISEÑO DE ERRORES

Define errores genéricos:

```text
INVALID_PACKET
CRC_ERROR
AUTH_ERROR
UNSUPPORTED_VERSION
UNSUPPORTED_TYPE
INVALID_LENGTH
TIMEOUT
REPLAY
DUPLICATE
NO_ROUTE
QUEUE_FULL
```

La aplicación puede mapearlos a sus propios conceptos.

---

# 27. LOGGING

Diseña logging opcional.

La librería no debería imprimir directamente:

```cpp
Serial.println(...)
```

por todas partes.

Evalúa:

```text
ILogger
```

o callbacks.

Debe ser posible:

```text
DEBUG
INFO
WARN
ERROR
NONE
```

y desactivar completamente logs para producción.

---

# 28. TESTING

Este punto es obligatorio.

Antes de implementar la nueva arquitectura, crea una estrategia de pruebas.

Como mínimo:

### Packet tests

```text
serialize
deserialize
CRC
TLV
length
version
unknown fields
```

### Security tests

```text
valid auth
invalid auth
modified packet
replay
wrong key
nonce/counter
```

### Reliability

```text
ACK
timeout
retry
duplicate
out-of-order
SEQ rollover
```

### Compatibility

```text
old packet
new packet
unknown TLV
unknown message type
```

### Platform

```text
ESP8266
ESP32
host simulation
```

Siempre que sea posible.

---

# 29. DOCUMENTACIÓN

No te limites a generar código.

Debes producir:

```text
PROTOCOL_SPEC.md
ARCHITECTURE.md
SECURITY.md
API.md
MIGRATION.md
README.md
```

La especificación del protocolo debe describir bytes reales.

Ejemplo:

```text
OFFSET
SIZE
FIELD
DESCRIPTION
```

No acepto una especificación puramente conceptual.

---

# 30. WIREFORMAT

Define exactamente el formato binario.

Por ejemplo:

```text
+---------+------+--------+-----+-----+-----+---------+
| VERSION | TYPE | FLAGS  | SRC | DST | SEQ | BOOT_ID |
+---------+------+--------+-----+-----+-----+---------+
| LENGTH  | PAYLOAD ...                         | CRC |
+---------+------------------------------------+-----+
```

Pero NO asumas que esta estructura es definitiva.

Primero analiza el protocolo actual y después diseña la mejor estructura.

Debes justificar:

* tamaños;
* endianess;
* offsets;
* límites;
* CRC;
* autenticación;
* extensibilidad.

---

# 31. ENDIANESS

Define explícitamente:

```text
little endian
```

o:

```text
big endian
```

y aplica la misma regla consistentemente.

No dependas del hardware.

---

# 32. IDENTIDAD DE NODOS

Evalúa:

```text
DEVICE_ID
NODE_ID
BOOT_ID
SESSION_ID
```

Determina qué conceptos realmente son necesarios.

No agregues identificadores redundantes sin justificación.

---

# 33. ROUTING

No implementes routing/mesh todavía si no es necesario.

Pero evalúa si el formato futuro debería permitirlo.

Por ejemplo:

```text
SOURCE
DESTINATION
```

podrían ser suficientes para una primera versión.

Si propones:

```text
NEXT_HOP
ROUTE
TTL
```

justifica claramente por qué.

---

# 34. COMPRESIÓN

Evalúa si tiene sentido.

Probablemente no sea necesaria inicialmente.

No agregues compresión al protocolo si el coste de CPU/RAM supera el beneficio.

---

# 35. STREAMING

Evalúa si el protocolo debe soportar:

```text
request
response
stream
event
```

y determina qué pertenece al Core.

---

# 36. MULTICAST / BROADCAST

Evalúa si debe existir:

```text
broadcast
multicast
unicast
```

y cómo afecta a:

* seguridad;
* ACK;
* discovery;
* deduplicación.

---

# 37. GATEWAY

La arquitectura debe permitir:

```text
Sensor
   ↓
Gateway
   ↓
otro transporte
```

sin que el Core dependa del concepto de alarma.

---

# 38. NO HACER OVERENGINEERING

Esta regla es fundamental:

> No conviertas un proyecto ESP pequeño en una implementación de TCP/IP.

Cada componente nuevo debe responder:

1. ¿Qué problema resuelve?
2. ¿Es necesario?
3. ¿Cuánto RAM consume?
4. ¿Cuánto Flash consume?
5. ¿Qué complejidad añade?
6. ¿Puede ser opcional?

Si no aporta suficiente valor, no lo implementes.

---

# 39. METODOLOGÍA DE TRABAJO

NO escribas todo el código inmediatamente.

Trabaja por fases.

## FASE 1 — AUDITORÍA

Analiza el proyecto completo.

Entrega:

```text
PROJECT_AUDIT.md
```

Incluyendo:

* arquitectura actual;
* módulos;
* dependencias;
* flujo de datos;
* protocolo actual;
* problemas;
* riesgos;
* contradicciones;
* deuda técnica;
* código reutilizable;
* código específico de alarma.

NO cambies código todavía.

---

# 40. FASE 2 — DISEÑO

Diseña:

```text
NodeProtocol Core
```

Entrega:

```text
PROTOCOL_V5_SPEC.md
ARCHITECTURE_V5.md
API_V5.md
MIGRATION_V4_V5.md
```

Incluye diagramas ASCII cuando ayuden.

Debes comparar:

```text
ACTUAL
vs
PROPUESTO
```

y justificar cada cambio.

---

# 41. FASE 3 — REVISIÓN DEL DISEÑO

Antes de escribir implementación:

Realiza una revisión crítica.

Busca:

* inconsistencias;
* complejidad innecesaria;
* problemas de seguridad;
* problemas de memoria;
* problemas de compatibilidad;
* problemas de portabilidad;
* problemas de API.

Si encuentras un problema en tu propio diseño, corrígelo antes de implementarlo.

---

# 42. FASE 4 — IMPLEMENTACIÓN

Solo después de las fases anteriores implementa.

Primero:

```text
Core
Packet
Serializer
CRC
TLV
Message
```

Después:

```text
Transport
Security
Reliability
Discovery
```

Después:

```text
ESP8266 adapter
ESP32 adapter
UDP transport
```

Después adapta el proyecto de alarma.

---

# 43. FASE 5 — MIGRACIÓN

El proyecto actual debe pasar a utilizar:

```text
NodeProtocol Core
+
Alarm Profile
```

sin perder:

* PIR;
* bocina;
* MQTT;
* Home Assistant;
* heartbeat;
* ACK;
* retransmisión;
* fallback local;
* estados de dispositivos.

---

# 44. NO ELIMINAR V3 INMEDIATAMENTE

Mantén la versión de producción actual hasta que la nueva implementación haya demostrado:

```text
compile
unit tests
integration tests
hardware tests
network failure tests
reboot tests
security tests
```

Solo entonces plantea retirar V3.

---

# 45. FORMATO DE ENTREGA DEL CÓDIGO

Cuando finalmente entregues código:

NO entregues fragmentos incompletos.

NO escribas:

```cpp
// resto del código...
```

NO escribas:

```text
etc.
```

NO omitas funciones.

Cada archivo debe entregarse:

```text
RUTA:
lib/NodeProtocol/src/Packet.cpp

CONTENIDO COMPLETO:
...
```

El contenido debe poder copiarse directamente al proyecto.

---

# 46. PATCH

Cuando sea posible, además del código completo genera:

```text
migration.patch
```

compatible con:

```bash
git apply --check migration.patch
git apply migration.patch
```

Pero nunca generes un patch basado en archivos que no hayas analizado.

---

# 47. CREDENCIALES

NUNCA solicites:

* GitHub tokens;
* passwords;
* API keys;
* Wi-Fi passwords;
* MQTT passwords;
* HMAC keys;
* secrets reales.

Si encuentras credenciales en el proyecto:

```text
REDÁCTALAS
```

y avisa.

Nunca las reproduzcas en una respuesta.

---

# 48. RESULTADO FINAL ESPERADO

La meta final es poder instalar la librería aproximadamente así:

```ini
lib_deps =
    usuario/NodeProtocol
```

y crear un proyecto nuevo:

```cpp
#include <NodeProtocol.h>

NodeProtocol node;

void setup() {
    node.begin();
}

void loop() {
    node.loop();
}
```

Un proyecto de alarma podría utilizar:

```text
NodeProtocol
+
Alarm Profile
```

Un proyecto de robot:

```text
NodeProtocol
+
Robotics Profile
```

Un proyecto de sensores:

```text
NodeProtocol
+
Sensor Profile
```

sin modificar el Core.

---

# 49. EJEMPLOS QUE DEBES PROBAR CONCEPTUALMENTE

No diseñes solamente pensando en la alarma.

Durante el diseño comprueba que la arquitectura pueda representar:

### EJEMPLO A — ALARMA

```text
PIR → EVENT → MOTION
```

### EJEMPLO B — SENSOR

```text
Sensor → DATA → TEMPERATURE
```

### EJEMPLO C — ROBOT

```text
Controller → COMMAND → MOVE
Robot → RESPONSE → MOVING
```

### EJEMPLO D — GPS

```text
Device → DATA → GPS_POSITION
```

### EJEMPLO E — RELÉ

```text
Controller → COMMAND → SET_OUTPUT
Relay → RESPONSE → OUTPUT_STATE
```

### EJEMPLO F — GATEWAY

```text
Node A
   ↓
Gateway
   ↓
otro transporte
   ↓
Node B
```

Si el Core necesita conocer "PIR", "alarma" o "robot" para resolver estos ejemplos, el diseño está mal abstraído.

---

# 50. CRITERIO DE ÉXITO

Consideraré que el rediseño es bueno si:

```text
1. El Core no conoce aplicaciones específicas.
2. El transporte es intercambiable.
3. La seguridad es modular.
4. La aplicación define sus propios datos.
5. TLV permite extensión.
6. Existe versionado.
7. Existe capability discovery.
8. ACK/reliability son genéricos.
9. El protocolo funciona en microcontroladores pequeños.
10. La API es sencilla.
11. Existe documentación binaria formal.
12. Existen tests.
13. La alarma actual puede migrarse.
14. Un proyecto completamente diferente puede reutilizar la librería.
15. No existe una reescritura innecesaria de componentes que ya funcionan.
```

---

# 51. INSTRUCCIÓN FINAL

Ahora recibirás el proyecto completo.

**NO empieces programando.**

Primero analiza todo el proyecto.

Después entrega:

## 1. AUDITORÍA

Qué existe y cómo funciona.

## 2. PROBLEMAS

Qué debe corregirse.

## 3. ARQUITECTURA PROPUESTA

Cómo convertir el protocolo actual en una librería universal.

## 4. ESPECIFICACIÓN DEL PROTOCOLO

Formato binario completo y formal.

## 5. API

Cómo utilizará la librería un proyecto externo.

## 6. MIGRACIÓN

Cómo pasar el proyecto actual a la nueva arquitectura.

## 7. IMPLEMENTACIÓN

Solo después de validar conceptualmente todo lo anterior.

No tengas miedo de decir:

> "Esta parte del diseño actual debe eliminarse."

Pero cada eliminación debe estar técnicamente justificada.

Tu prioridad no es producir mucho código.

Tu prioridad es producir una **arquitectura correcta, simple, portable, segura, extensible y reutilizable**.

El proyecto de alarma actual es solamente el **primer consumidor de la librería**, no su definición.

---

# DATOS DEL PROYECTO

A continuación se proporciona el proyecto completo.

Analízalo como si fuera un repositorio real.

No inventes archivos que no existan.

No asumas contenido que no hayas recibido.

Cuando exista una incertidumbre, indícala explícitamente.

--- INICIO DEL PROYECTO ---

[PEGAR AQUÍ EL PROYECTO COMPLETO]

--- FIN DEL PROYECTO ---

### Una recomendación adicional

Yo **no le pasaría al otro LLM solamente el código**. Si tienes espacio de contexto suficiente, pásale también:

```text
proyecto completo
+
.gitignore
+
README
+
docs/
+
platformio.ini
+
todos los .h/.cpp
+
estructura de carpetas
```

Y **no le pases `secrets.h` real ni credenciales**.

Lo interesante de este meta-prompt es que obliga al otro modelo a hacer algo que considero crucial: **primero diseñar el protocolo universal y luego adaptar la alarma a él**, en lugar de hacer lo contrario.

También he dejado deliberadamente una puerta abierta para que el modelo **rechace partes de nuestra propia propuesta** si encuentra una arquitectura mejor. Eso es importante: no queremos que simplemente obedezca nuestras ideas, sino que las someta a revisión técnica.

Si el modelo hace bien las primeras cuatro fases, el resultado debería ser una especificación que puedas conservar independientemente del LLM que utilices después.
