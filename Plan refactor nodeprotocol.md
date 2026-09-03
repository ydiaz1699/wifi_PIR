# Plan de refactor: de `IoTProtocol` a una librería genérica

**Objetivo:** convertir el protocolo actual (acoplado a la alarma PIR/bocina) en una librería de comunicación para dispositivos embebidos que cualquier proyecto futuro (robot, sensor, telemetría) pueda usar sin tocar el núcleo.

**Principio rector:** no romper la alarma en producción. Migración incremental, por fases, con la V3/V4 actual como fallback recuperable en todo momento.

---

## Fase 0 — Congelar y proteger lo que funciona

Antes de tocar nada:

1. Etiquetar el estado actual como `v4.3.1-stable` (tag de git).
2. No mezclar refactor con features nuevas (zonas, capability discovery, etc. — quedan bloqueadas hasta terminar esto).
3. Mantener USB como método de recuperación para ambos nodos durante todo el proceso; OTA no se usa como único camino hasta el final.
4. Correr y anotar el resultado actual de los tests (`10/10 C++`, `16/16 Python`, `10/10 simulador`) como línea base — cualquier regresión durante el refactor se compara contra esto.

**Salida de esta fase:** un commit/tag limpio del que partir, y un criterio explícito de "no romper esto".

---

## Fase 1 — Auditoría del código actual

Objetivo: separar, archivo por archivo, qué es genérico (comunicación) y qué es específico de la alarma (aplicación).

Revisar cada archivo de `lib/IoTProtocol/` y clasificar su contenido en una tabla:

| Archivo/función | ¿Genérico o específico de alarma? | Motivo |
|---|---|---|
| `IoTProtocol.cpp` (framing, CRC, TLV base) | Genérico | No conoce PIR/MOTION |
| `IoTNode.cpp` (envío/recepción, ACK) | Genérico, revisar mezcla | Puede tener callbacks con nombres de alarma |
| `IoTAuth.cpp` (HMAC) | Genérico | — |
| `IoTStorage.cpp` (BOOT_ID en LittleFS) | Genérico, pero acoplado a LittleFS | Debe pasar a interfaz `IStorage` |
| `IoTConfigHandler.cpp` | Mixto | Revisar si conoce campos de alarma |
| Códigos de evento (`MOTION`, `TIMBRE`) | Específico de alarma | Debe salir del core a un "profile" |

Entregable: `docs/AUDITORIA_V4.md` con esta tabla completa y una lista de "contaminaciones" (lugares donde el core conoce conceptos de alarma).

---

## Fase 2 — Definir las capas objetivo

Arquitectura destino (capas, no todas se implementan de golpe):

```
APLICACIÓN (Alarm Profile, futuro Robotics/Sensor Profile)
        │
NODE / MESSAGE MODEL   (HELLO, DATA, COMMAND, RESPONSE, ACK, ERROR)
        │
RELIABILITY            (ACK, retransmisión, SEQ, BOOT_ID, dedup)
        │
SECURITY                (interfaz: NoSecurity / HMAC / futuro AEAD)
        │
SERIALIZATION            (TLV: protocol-tags vs application-tags separados)
        │
TRANSPORT                (interfaz: UDP hoy, Serial/ESP-NOW mañana)
```

Reglas de diseño que fija esta fase (no negociables durante la implementación):

- El **core no importa nada** que mencione PIR, bocina, MQTT o Home Assistant.
- Los "tags" TLV se dividen en dos rangos: `0x00–0x7F` reservados al protocolo, `0x80+` libres para la aplicación.
- Todo campo desconocido/opcional debe poder ignorarse sin romper el paquete (compatibilidad hacia adelante).
- Un solo formato de wire, versionado explícito (`PROTOCOL_VERSION`), separado del versionado de la librería.

Entregable: `docs/ARCHITECTURE_V5.md` con el diagrama de capas, la especificación binaria del paquete (offsets y tamaños) y la tabla de tags reservados vs libres.

---

## Fase 3 — Diseñar la API pública

Definir cómo se ve usar la librería desde un proyecto nuevo, antes de escribir una línea de implementación:

```cpp
#include <NodeProtocol.h>

NodeProtocol node(config);

void setup() { node.begin(); }

void loop() {
  node.loop();
}

// Enviar
Message msg;
msg.type = MessageType::DATA;
msg.add(APP_TAG_MOTION, 1);
node.send(msg);

// Recibir
node.onMessage([](const Message& msg) {
  // la aplicación decide qué significa cada tag
});
```

Puntos a decidir en esta fase:

- ¿`Node` vive dentro del core o es una capa opcional encima de `Packet`/`Serializer` (para poder usar solo el framing en un gateway)?
- Política de entrega por mensaje: `UNRELIABLE` vs `RELIABLE` — configurable por `send()`, no global.
- Cómo se registra un "Application Profile" (p. ej. un `namespace AlarmProfile { ... }` que define sus propios tags y helpers, sin tocar el core).

Entregable: `docs/API_V5.md` con firmas de funciones y 2-3 ejemplos de uso (alarma actual + un caso hipotético como "sensor de temperatura") para validar que la API sirve para ambos sin cambios en el core.

---

## Fase 4 — Extraer las interfaces (sin reescribir la lógica)

Este es el primer paso de código real, y es deliberadamente conservador: mover, no reescribir.

1. **`ITransport`**: extraer la interfaz; la implementación UDP actual pasa a `transports/udp/UdpTransport.cpp` sin cambiar su lógica interna.
2. **`ISecurity`**: extraer interfaz de `IoTAuth`; implementación HMAC actual pasa a `security/HmacSecurity.cpp`. Añadir `NoSecurity` como implementación trivial (hoy el "auth disabled" es un `if`, debería ser una clase).
3. **`IStorage`**: extraer interfaz de `IoTStorage`; implementación LittleFS actual pasa a `storage/LittleFsStorage.cpp`.
4. **Separar tags**: mover `MOTION`, `TIMBRE` y demás constantes de evento fuera de `IoTProtocol.h` a un nuevo `AlarmProfile.h` en el proyecto de aplicación (no en la librería).

Regla de esta fase: **cada extracción se compila y se corre contra los tests existentes antes de seguir con la siguiente.** Si algo se rompe, se revierte esa extracción puntual, no todo el refactor.

---

## Fase 5 — Corregir la deuda técnica bloqueante en paralelo

Estos bugs ya identificados en el informe de pruebas hardware deben resolverse **como parte de esta migración**, no después, porque tocan justamente las interfaces que se están extrayendo:

| Item | Por qué toca al refactor |
|---|---|
| Cola URGENT puede descartar eventos si está llena | Vive en la capa Reliability que se está extrayendo |
| Fallo de LittleFS no es seguro / BOOT_ID no garantizado único | Se resuelve naturalmente al definir bien `IStorage` (declarar `STORAGE_DEGRADED`, no formatear solo) |
| Política de auth inconsistente (dos puntos de decisión) | Se resuelve al forzar `ISecurity.verify()` como único punto de decisión |
| Config remota no atómica | Toca `IoTConfigHandler`, que queda pendiente de re-diseño en Fase 6 |

No se resuelven "de más" — solo lo mínimo para que la interfaz extraída quede correcta.

---

## Fase 6 — Reensamblar el core y migrar la alarma

1. Construir `NodeProtocol` (nombre a decidir) componiendo las interfaces ya extraídas.
2. Crear `AlarmProfile`: capa fina en el proyecto de aplicación que define los tags de MOTION/TIMBRE/ARM/DISARM y traduce eventos de hardware a `Message` genéricos.
3. Migrar `emisor_pir_unificado` y `receptor_central_unificado`/`legacy/receptor_bocina` para usar `NodeProtocol` + `AlarmProfile` en vez de `IoTProtocol` directamente.
4. Mantener MQTT y Home Assistant como adaptadores externos que consumen mensajes ya decodificados por `AlarmProfile` — nunca dentro del core.

Validación de esta fase: repetir exactamente el plan de pruebas de hardware que ya tenías (Fase 1 a 8 del informe: USB, protocolo mínimo, sensores, reinicios, MQTT, auth, OTA) pero contra el nuevo stack.

---

## Fase 7 — Documentar y cerrar

- `PROTOCOL_SPEC.md`: formato binario final, byte a byte.
- `MIGRATION_V4_V5.md`: qué se conservó, qué cambió, qué se eliminó — para que quede registro de por qué.
- Actualizar `README.md` con el ejemplo de uso de la librería.
- Solo después de que V5 pase el mismo checklist de hardware que V4, se retira V4 como fallback (no antes).

---

## Qué queda fuera de este refactor (a propósito)

Para no mezclar objetivos, estas ideas del roadmap **no entran** en este plan y se retoman después:

- Capability discovery, máquina de estados de alarma, zonas, rollback OTA, telemetría avanzada.
- Soporte multi-transporte real (Serial/ESP-NOW) — la interfaz se deja lista, pero no se implementa un segundo transporte todavía.
- Evaluar formatos de serialización alternativos (CBOR, Protobuf/nanopb) — se puede investigar en paralelo, pero cambiar el wire format es una decisión aparte de "modularizar lo que ya existe".

---

## Orden resumido

```
Fase 0  Congelar estado actual (tag + baseline de tests)
Fase 1  Auditoría: qué es genérico vs específico
Fase 2  Definir capas y spec binaria (solo diseño, sin código)
Fase 3  Diseñar API pública (solo diseño, sin código)
Fase 4  Extraer interfaces (ITransport, ISecurity, IStorage) sin reescribir lógica
Fase 5  Corregir deuda técnica bloqueante en las mismas interfaces
Fase 6  Reensamblar core + AlarmProfile + migrar la alarma real
Fase 7  Documentar y retirar V4 como fallback
```

Cada fase termina solo cuando los tests existentes (o los ampliados) pasan. Ninguna fase avanza si la anterior deja algo roto.
