# Capability Discovery — especificación futura

## Estado

- **Estado:** PROPUESTA IMPLEMENTABLE; no está implementada en el código V4 actual.
- **Origen histórico:** la idea fue consolidada desde el backlog de drafts; no depende de que el archivo fuente siga presente.
- **Auditoría relacionada:** [`INFORME_DRAFTS_RESTANTES.md`](INFORME_DRAFTS_RESTANTES.md) y [`../DRAFTS_AUDIT.md`](../DRAFTS_AUDIT.md).
- **Código relacionado:** `lib/IoTProtocol/IoTProtocol.h`, `lib/IoTProtocol/IoTNode.h`, `lib/IoTProtocol/IoTNode.cpp`, `receptor_central_unificado/src/event_handler.cpp` y `receptor_central_unificado/src/main.cpp`.
- **Objetivo:** conservar suficiente detalle para que una sesión futura pueda implementar esta capacidad sin depender del draft original ni de la memoria de una conversación.

Este documento describe el contrato propuesto, las decisiones de compatibilidad, los cambios probables y las pruebas necesarias. **No afirma que la capacidad exista actualmente.**

## 1. Idea original conservada

La idea de `docs/DRAFTS_AUDIT.md` es que cada dispositivo anuncie qué funciones soporta cuando se conecta:

```text
DEVICE_ID: 0x12

CAPABILITIES:
  MOTION
  TEMPERATURE
  BATTERY

DEVICE_ID: 0x13

CAPABILITIES:
  DOOR
  BATTERY
```

La central registra esas capacidades y deja de depender de una lista fija de dispositivos o de lógica específica para cada modelo.

La intención no es que el nodo envíe solo un nombre genérico como `PIR01`. La intención es que el protocolo comunique una descripción funcional mínima y que la central pueda seleccionar handlers, datos, comandos y entidades según esa descripción.

## 2. Distinciones obligatorias

### 2.1 Capability, evento y estado no son lo mismo

```text
CAPABILITY = lo que el dispositivo sabe hacer
EVENT      = algo que ocurrió
STATE      = el valor o condición actual
```

Ejemplo:

```text
CAPABILITY: TEMPERATURE
EVENT:      DATA/TEMPERATURE_UPDATED
STATE:      TEMPERATURE = 23.7 °C
```

`BATTERY` significa inicialmente “el dispositivo puede reportar batería”. No significa que el anuncio de discovery ya incluya el porcentaje actual. El porcentaje debe viajar como telemetría o estado separado.

### 2.2 `DeviceType` no reemplaza capabilities

`DeviceType::PIR_SENSOR` describe una clasificación principal del dispositivo. No permite representar por sí solo que un nodo PIR también tenga temperatura y batería.

```text
DeviceType:   PIR_SENSOR
Capabilities: MOTION, TEMPERATURE, BATTERY
```

Un `MULTI_SENSOR` tampoco constituye capability discovery: solo es otro valor singular de `DeviceType`.

### 2.3 Las capabilities no pertenecen todas al mismo nivel

Una capability puede describir:

- un evento que el nodo puede producir: `MOTION`, `DOOR`;
- un dato que puede medir: `TEMPERATURE`, `HUMIDITY`;
- una telemetría que puede reportar: `BATTERY`, `RSSI`;
- un actuador que puede recibir comandos: `RELAY`;
- una interfaz que puede mostrar información: `DISPLAY`.

La primera versión puede usar un único namespace de IDs, pero cada ID debe tener una semántica documentada. No se debe reutilizar directamente `EventCode` como si fuera un catálogo de capabilities.

## 3. Estado real antes de implementar

La auditoría del código V4 encontró lo siguiente:

| Elemento | Estado actual |
|---|---|
| `MsgType::HELLO` | Declarado y enviado por el nodo V4 |
| `MsgType::HELLO_ACK` | Declarado; no existe un flujo funcional de aplicación desde la central |
| ACK de protocolo | Automático cuando HELLO lleva `ACK_REQUIRED`; no es `HELLO_ACK` |
| `TlvTag::CAPABILITY` | Declarado como tag repetible `0x62`, pero sin envío ni parser funcional |
| Campos actuales de HELLO | `DEVICE_TYPE`, `DEVICE_NAME`, `FW_VERSION`, `BOOT_ID_TAG` |
| Lista/bitmask de capabilities | No existe en `RemoteDevice` |
| Registro central de capabilities | No existe |
| Publicación MQTT de capabilities | No existe |
| Selección de handlers por capability | No existe; el dispatch usa tipos y tags codificados explícitamente |
| Compatibilidad V3 | V3 usa su protocolo textual y no debe modificarse para introducir esta función |

El wire format V4 actual usa TLV de un byte de tag, un byte de longitud y N bytes de valor, con payload máximo de 64 bytes. La mera declaración del tag `CAPABILITY` no demuestra que discovery esté implementado.

## 4. Contrato propuesto para Capability Discovery V1

### 4.1 Momento de anuncio

El nodo anuncia capabilities dentro de su `HELLO` inicial. También debe volver a anunciar el conjunto completo cuando cambie cualquiera de estas condiciones:

- arranque del nodo;
- cambio de firmware;
- cambio de configuración que altere sus funciones;
- cambio de hardware/perfil que altere sus funciones.

El anuncio debe ser el conjunto completo, no un delta. Así la central puede reemplazar el registro anterior sin conservar capabilities obsoletas.

### 4.2 Payload de HELLO

Se conservan los TLV actuales y se agregan cero o más TLV `CAPABILITY`:

```text
HELLO payload V1
  DEVICE_TYPE       uint8       obligatorio en V4 actual
  DEVICE_NAME       string      obligatorio en V4 actual
  FW_VERSION        string      obligatorio en V4 actual
  BOOT_ID_TAG       uint16      existente en V4 actual
  CAPABILITY        uint8       repetible, nuevo uso propuesto
  CAPABILITY        uint8       repetible, nuevo uso propuesto
  ...
```

Cada TLV `CAPABILITY` contiene exactamente un ID de capability de un byte. Repetir el mismo ID no aporta información y debe deduplicarse al almacenarlo.

**Presupuesto normativo de V1:** el HELLO admite como máximo **cinco** TLV `CAPABILITY`. El cálculo conservador usa los límites actuales de almacenamiento: `DEVICE_NAME` máximo de 19 bytes en wire, `FW_VERSION` máximo de 11 bytes, los cuatro TLV base (`DEVICE_TYPE`, `DEVICE_NAME`, `FW_VERSION`, `BOOT_ID_TAG`) y, cuando se use la auth actual, el TLV HMAC de 6 bytes (`tag + length + 4`). Cada capability consume 3 bytes (`tag + length + 1`), por lo que cinco dejan margen dentro de `IOT_MAX_PAYLOAD=64` incluso con auth. Si se cambia el tamaño de HMAC, los límites de nombre/version o el payload máximo, hay que recalcular este presupuesto antes de cambiar el contrato.

La API local debe rechazar la sexta capability y no modificar el conjunto ya configurado. La serialización debe fallar sin enviar un HELLO truncado si el payload no cabe. No se permite truncar silenciosamente ni omitir el TLV de auth para hacer espacio. Una futura lista mayor requerirá otro mensaje de discovery o una revisión de V1; no se resuelve aumentando el límite implícitamente.

Esta es una propuesta deliberadamente pequeña porque respeta el formato TLV existente y el límite de payload. No se debe introducir todavía un JSON, una cadena separada por comas ni un bitmap propietario dentro del payload.

### 4.3 Registro inicial de IDs

Estos IDs son una propuesta V1 y deben congelarse solo después de revisión y tests:

| ID propuesto | Nombre | Significado mínimo |
|---:|---|---|
| `0x01` | `MOTION` | Puede producir eventos de movimiento |
| `0x02` | `TEMPERATURE` | Puede reportar temperatura |
| `0x03` | `BATTERY` | Puede reportar estado o nivel de batería |
| `0x04` | `DOOR` | Puede producir apertura/cierre de puerta o contacto |
| `0x05` | `HUMIDITY` | Puede reportar humedad |
| `0x06` | `SMOKE` | Puede producir estado/evento de humo |
| `0x07` | `RELAY` | Puede recibir comandos de relé |
| `0x08` | `BUTTON` | Puede producir pulsación/liberación de botón |
| `0x09` | `FLOOD` | Puede producir detección de inundación |
| `0x0A` | `GAS` | Puede producir detección de gas |
| `0x0B` | `TAMPER` | Puede detectar manipulación |
| `0x0C` | `DISPLAY` | Puede mostrar información o estados |

Reglas del registro:

1. Los IDs de capabilities tienen namespace propio. No son automáticamente valores de `EventCode`, `DeviceType` ni `TlvTag`.
2. Un ID desconocido debe conservar la validez estructural del paquete; la central lo ignora o lo registra como desconocido.
3. No reutilizar un ID para cambiar su significado.
4. Si la semántica necesita cambiar de forma incompatible, asignar un ID nuevo o versionar el schema.
5. El registro debe documentar si una capability produce eventos, datos, acepta comandos o una combinación.

### 4.4 Modelo de datos de la central

La estructura actual `RemoteDevice` debe ampliarse después de medir RAM. El modelo mínimo propuesto es:

```cpp
struct CapabilityState {
    bool present;
    uint8_t schemaVersion;
    uint32_t knownMask; // ID 0x01 -> bit 0; ID 0x20 -> bit 31
};
```

El mapeo es exactamente `bit_index = capability_id - 1`. Por tanto, los IDs `0x01..0x20` son representables en `uint32_t`; un ID `0x20` usa el bit 31 y un ID `0x21` queda fuera del conjunto conocido de V1. No se debe usar el ID directamente como índice del bit.

Reglas del modelo:

- `present=false` significa que el nodo es antiguo o que nunca anunció capabilities; no significa que no soporte ninguna.
- `present=true` con `knownMask=0` significa que el nodo anunció explícitamente un conjunto vacío.
- IDs conocidos `0x01..0x20` se almacenan usando exactamente `1u << (capabilityId - 1)`.
- IDs fuera de `0x01..0x20` se ignoran para dispatch V1 y se registran sin convertirlos en capabilities conocidas.
- `schemaVersion=1` es una constante del contrato V1 en esta primera propuesta; no se transmite en un TLV separado todavía. Si se necesita negociar schemas, debe asignarse un tag nuevo y revisarse el presupuesto de HELLO.
- El conjunto se reemplaza atómicamente al aceptar un HELLO válido; no se mezcla con el conjunto anterior.
- El registro debe conservar el `BOOT_ID` y el momento de recepción para saber a qué sesión pertenece el anuncio.

El uso de un bitmask de 32 bits es una decisión de almacenamiento propuesta para V1, no una obligación del protocolo universal final. Antes de aplicarlo se debe medir el coste sobre los ocho slots actuales de `RemoteDevice`.

### 4.5 Flujo de recepción

**Gate de confianza:** hasta que BUG-010 se corrija, el sistema no debe usar metadata de HELLO o capabilities para crear entidades, seleccionar handlers con efectos o autorizar operaciones. En la migración, un HELLO sin auth puede registrarse únicamente como metadata no confiable/legacy si la política lo permite; un HELLO con auth inválida no debe modificar registry ni capabilities. En producción V1, la recomendación es exigir auth para discovery y cerrar explícitamente cualquier excepción antes de implementarla.

El flujo objetivo, una vez corregida la autenticación temprana de V4, es:

```text
recibir datagrama
  ↓
validar magic, versión, longitud, CRC y TLV
  ↓
verificar autenticación según política
  ↓
validar BOOT_ID/SEQ y deduplicación
  ↓
si es HELLO válido:
    extraer metadata actual
    extraer todos los CAPABILITY
    reemplazar el conjunto anterior
    registrar la sesión
  ↓
enviar ACK de protocolo según la política
  ↓
despachar al handler de aplicación
  ↓
actualizar adapter MQTT/HA si corresponde
```

Mientras BUG-010 siga pendiente, el código actual no debe describirse como si ya siguiera este orden: hoy `IoTNode` puede actualizar registry, enviar ACK y deduplicar antes de que el callback verifique HMAC.

### 4.6 Compatibilidad con nodos antiguos

Un nodo V4 antiguo que envía HELLO sin `CAPABILITY` debe seguir siendo aceptado:

```text
HELLO sin CAPABILITY → capabilitiesPresent=false → modo legacy
HELLO con CAPABILITY → capabilitiesPresent=true → modo dinámico
```

La central no debe rechazar un HELLO solo porque no tenga capabilities durante la migración.

La V3 no se modifica. Un receptor V3 no debe recibir TLV V4 ni se debe intentar traducir automáticamente `PIR01|5|TIMBRE` a un anuncio de capabilities sin un bridge explícito.

### 4.7 Política de la central

La central debe usar capabilities para seleccionar comportamiento, pero no debe asumir que una capability es una credencial de seguridad.

Ejemplos:

```text
MOTION       → habilitar handler de eventos de movimiento
TEMPERATURE  → habilitar parser de DATA/TEMPERATURE
BATTERY      → habilitar parser de telemetría de batería
DOOR         → habilitar DOOR_OPEN/DOOR_CLOSE
RELAY        → habilitar comandos del perfil relay
```

Una capability declarada no autoriza por sí misma un comando. Los comandos deben seguir requiriendo autenticación, autorización y validación del perfil.

Cuando llega un evento o dato no anunciado:

- durante migración, se recomienda no romper el nodo: registrar una discrepancia y aplicar una política explícita del perfil;
- no crear automáticamente una entidad insegura basándose únicamente en un paquete inesperado;
- no aceptar un comando solo porque el nodo declaró `RELAY`;
- la política final debe quedar en los tests y en el adapter correspondiente.

## 5. Uso por MQTT/Home Assistant

MQTT/Home Assistant es un adapter, no parte del núcleo universal.

El adapter puede publicar, después de un HELLO válido:

```text
casa/iot/device_12/capabilities
[
  "MOTION",
  "TEMPERATURE",
  "BATTERY"
]
```

Este topic es una propuesta operativa; no debe considerarse nombre definitivo hasta definir la convención MQTT del proyecto.

El adapter puede generar Discovery según capabilities:

```text
MOTION       → entidad/evento de movimiento
TEMPERATURE  → sensor de temperatura
BATTERY      → sensor de batería
DOOR         → binary_sensor de puerta
RELAY        → entidad de comando/estado de relé
```

Requisitos del adapter:

1. `unique_id` debe incluir el `DEVICE_ID` y el recurso/capability.
2. Una capability no debe generar dos entidades por cada retransmisión de HELLO.
3. El cambio de capabilities debe reemplazar o retirar entidades obsoletas según una política documentada.
4. El payload retained debe validarse en el broker real; un log de `publish()` no es suficiente.
5. Un nodo sin capabilities debe usar el comportamiento legacy o una configuración manual, no inventar entidades.
6. La corrección de ArduinoJson de BUG-011 debe verificarse en el payload retained real antes de declarar discovery operativo.

## 6. API propuesta para implementar en V4

Los nombres son candidatos concretos para guiar una implementación, no una afirmación de que ya existan:

```cpp
enum class CapabilityId : uint8_t {
    MOTION      = 0x01,
    TEMPERATURE = 0x02,
    BATTERY     = 0x03,
    DOOR        = 0x04,
    HUMIDITY    = 0x05,
    SMOKE       = 0x06,
    RELAY       = 0x07,
    BUTTON      = 0x08,
    FLOOD       = 0x09,
    GAS         = 0x0A,
    TAMPER      = 0x0B,
    DISPLAY     = 0x0C,
};

bool addCapability(CapabilityId capability);
void clearCapabilities();
bool hasCapability(CapabilityId capability) const;
```

La API de `IoTNode` debería:

1. almacenar capabilities configuradas antes de `sendHello()`;
2. deduplicar IDs;
3. rechazar overflow de la capacidad local de almacenamiento;
4. añadir un TLV por capability al HELLO;
5. permitir que el emisor configure el conjunto desde su perfil, no desde lógica central hardcodeada;
6. no enviar capabilities si el nodo está en modo legacy explícito;
7. conservar el HELLO actual cuando el conjunto está vacío.

Para la central se necesita una función equivalente a:

```cpp
bool parseCapabilities(const IoTPacket& packet,
                       CapabilityState& output,
                       ParseError& error);

bool remoteHasCapability(uint8_t deviceId, CapabilityId capability);
```

El parser debe distinguir:

- ausencia total de `CAPABILITY`;
- capability repetida;
- capability conocida;
- capability desconocida;
- TLV con longitud distinta de uno;
- demasiados IDs para el límite de almacenamiento;
- HELLO duplicado de la misma sesión.

## 7. Plan de implementación por etapas

### Etapa 0 — contrato y pruebas host

- Congelar el registro V1 o documentar explícitamente que sigue provisional.
- Añadir tests de parseo de cero, una, varias, repetidas y desconocidas.
- Verificar que HELLO antiguo sin capabilities sigue siendo válido.
- Medir payload máximo con el número de capabilities esperado.
- Medir RAM de `RemoteDevice` con el almacenamiento elegido.

### Etapa 1 — biblioteca

Archivos candidatos:

```text
lib/IoTProtocol/IoTProtocol.h
lib/IoTProtocol/IoTNode.h
lib/IoTProtocol/IoTNode.cpp
```

Cambios:

1. Definir `CapabilityId` en un namespace propio.
2. Añadir almacenamiento local de capabilities al nodo.
3. Añadir `addCapability`, `clearCapabilities` y `hasCapability`.
4. Extender `sendHello()` con TLV repetidos `CAPABILITY`.
5. No cambiar el orden ni el significado de los campos existentes.
6. Mantener el payload dentro de 64 bytes.

### Etapa 2 — parser y registry

Archivos candidatos:

```text
lib/IoTProtocol/IoTNode.h
lib/IoTProtocol/IoTNode.cpp
```

Cambios:

1. Añadir `capabilitiesPresent`, `capabilitySchema` y `knownMask` al `RemoteDevice`.
2. Extraer el conjunto completo solo en HELLO.
3. Reemplazar el conjunto previo de forma atómica.
4. Mantener unknown IDs fuera del bitmask sin invalidar el HELLO.
5. No actualizar capabilities con EVENT, DATA o HEARTBEAT.
6. Definir comportamiento cuando se recicla un slot del registry.

### Etapa 3 — perfiles de emisor

Archivos candidatos:

```text
emisor_pir_unificado/src/main.cpp
receptor_central_unificado/src/main.cpp
```

Ejemplos de configuración:

```cpp
node.addCapability(CapabilityId::MOTION);
node.addCapability(CapabilityId::TEMPERATURE);
node.addCapability(CapabilityId::BATTERY);
node.sendHello();
```

Un emisor de puerta debería anunciar `DOOR` y `BATTERY`, no copiar la lista de un PIR.

### Etapa 4 — central y adapters

Archivos candidatos:

```text
receptor_central_unificado/src/event_handler.cpp
receptor_central_unificado/src/main.cpp
receptor_bocina/src/mqtt_discovery.cpp
```

Cambios:

1. Loguear capabilities recibidas de forma legible.
2. Publicar el conjunto mediante un adapter con schema definido.
3. Seleccionar handlers por capability sin eliminar los handlers legacy.
4. Crear MQTT Discovery solo después de validar JSON retained.
5. No mezclar capacidades V4 con el discovery textual V3 sin bridge explícito.

### Etapa 5 — seguridad y promoción

Antes de considerarlo parte del protocolo universal:

1. Ejecutar la corrección de autenticación temprana de BUG-010.
2. Decidir si un HELLO sin auth se acepta solo durante migración.
3. No usar capabilities como autorización.
4. Probar reinicio y cambio de `BOOT_ID`.
5. Verificar que un HELLO inválido no sustituye metadata existente.
6. Documentar el schema y la compatibilidad.

## 8. Matriz de pruebas

| Caso | Entrada | Resultado esperado |
|---|---|---|
| Legacy | HELLO sin `CAPABILITY` | Aceptar; `capabilitiesPresent=false` |
| Una capability | `MOTION` | Registrar una capability |
| Varias | `MOTION`, `TEMPERATURE`, `BATTERY` | Registrar las tres |
| Ejemplo de puerta | `DOOR`, `BATTERY` | Registrar ambas y seleccionar perfil puerta |
| Repetida | `BATTERY`, `BATTERY` | Una sola entrada almacenada |
| Desconocida | `0x7E` | HELLO estructuralmente válido; ignorar/registrar desconocida |
| Longitud incorrecta | `CAPABILITY` con len 0 o 2 | Rechazar el paquete o marcar HELLO inválido según política definida; no modificar registry |
| Payload excesivo | sexta capability o límites de nombre/version excedidos | Rechazar `addCapability()`/serialización sin modificar el conjunto ni enviar HELLO truncado |
| Duplicado fiable | mismo HELLO y mismo BOOT_ID/SEQ | ACK según reliable, sin duplicar efectos |
| Reinicio | mismo device, BOOT_ID nuevo, capacidades diferentes | Aceptar nueva sesión y reemplazar conjunto |
| HELLO inválido | CRC/HMAC incorrecto | No modificar registry ni capabilities después de implementar auth temprana |
| Evento no anunciado | EVENT de capability ausente | Registrar discrepancia y aplicar política de perfil; no crear autorización |
| MQTT retained | HELLO válido | JSON válido, sin entidades duplicadas |
| Nodo antiguo | V4 sin capabilities | Mantener modo legacy |
| V3 | `PIR01|5|TIMBRE` | Seguir procesándose por el protocolo V3, sin capability discovery automático |

## 9. Criterios de aceptación

Capability Discovery V1 solo puede marcarse como `VERIFICADO` cuando:

- dos perfiles distintos anuncian conjuntos distintos;
- la central registra y consulta capabilities sin una tabla hardcodeada por dispositivo;
- HELLO antiguo sigue interoperando;
- IDs desconocidos no rompen el parser;
- duplicados no generan registros duplicados;
- un reinicio reemplaza el conjunto de la sesión anterior correctamente;
- un paquete inválido no altera registry ni capabilities;
- los handlers y entidades se seleccionan según capability con una política documentada;
- MQTT retained contiene JSON válido si el adapter está habilitado;
- el uso de RAM, flash y payload está medido;
- existen pruebas host y al menos una prueba de integración con dos perfiles;
- la documentación distingue `DeviceType`, capability, evento y estado.

## 10. Decisiones todavía abiertas

Estas decisiones deben resolverse antes de congelar el contrato:

1. Si el `knownMask` de 32 bits es suficiente o se necesita una lista/bitmap extensible.
2. Si se añade un TLV explícito de versión de schema o basta con versionar por protocolo.
3. Si una capability desconocida se conserva para diagnóstico o solo se ignora.
4. Si un evento no anunciado se acepta, se registra o se rechaza por perfil.
5. Si MQTT publica una lista de strings, un objeto con metadata o solo genera Discovery.
6. Qué capacidades son únicamente de datos, solo de eventos, solo de comandos o mixtas.
7. Cómo se retiran entidades MQTT cuando desaparece una capability tras una actualización.
8. Si `HELLO_ACK` de aplicación es necesario o basta el ACK automático de protocolo.
9. Cómo se autentica HELLO durante la migración V4.
10. Qué parte de este contrato pertenece al núcleo universal y qué parte pertenece a los perfiles de alarma, sensor y actuador.

Hasta cerrar estas decisiones, el documento permanece en estado **PROPUESTA IMPLEMENTABLE**, no `APLICADO` ni `VERIFICADO`.

## 11. Trazabilidad

| Elemento | Fuente | Estado | Destino |
|---|---|---|---|
| Device 0x12 con MOTION/TEMPERATURE/BATTERY | `docs/DRAFTS_AUDIT.md` | Conservado y formalizado | Secciones 1, 4 y 8 |
| Device 0x13 con DOOR/BATTERY | `docs/DRAFTS_AUDIT.md` | Conservado y formalizado | Secciones 1, 4 y 8 |
| Añadir sensores sin protocolos paralelos | `docs/DRAFTS_AUDIT.md` | Objetivo arquitectónico | Secciones 2, 7 y 9 |
| `CAPABILITY=0x62` | `IoTProtocol.h` actual | Declarado, no implementado | Secciones 3 y 4 |
| HELLO/registry existente | Código V4 | Parcialmente implementado | Secciones 3 y 7 |
| MQTT/HA dinámico | `docs/DRAFTS_AUDIT.md` + roadmap | Futuro adapter | Sección 5 |
| Auth antes de efectos internos | Hallazgos H-002/BUG-010 y código actual | Integrado en la frontera de `IoTNode`; pruebas pendientes | Secciones 4.5 y 7.5 |

## Resultado

La idea de “un sistema de capacidades” ya no está reducida a una frase de roadmap. Está conservada como una especificación futura con:

```text
idea original
→ semántica
→ estado real del código
→ contrato de wire propuesto
→ modelo de registry
→ compatibilidad
→ API candidata
→ plan por etapas
→ pruebas
→ criterios de aceptación
→ decisiones abiertas
```

Eso permite borrar el draft original sin perder la capacidad de implementar la idea más adelante, siempre que esta especificación permanezca en el repositorio y sus decisiones abiertas se cierren antes de modificar el firmware.
