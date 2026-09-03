# Roadmap — Mejoras Futuras

> **Actualización canónica 2026-09-02:** los diez drafts fueron auditados y retirados; ver [`DRAFTS_AUDIT.md`](DRAFTS_AUDIT.md). Las rutas activas son `emisor_pir_unificado/`, `receptor_central_unificado/` y `legacy/`. Los nuevos `EventCode`, `DeviceType` y `StateTag` del perfil alarma deben definirse en `lib/AlarmProfile/AlarmProfile.h`; `IoTProtocol.h` solo conserva el vocabulario del core. La evaluación tecnológica V5 sigue pendiente y no modifica V4.
>
> ## Instrucciones para LLM

**CONTEXTO**: Este es un sistema de alarma IoT doméstico con ESP8266. Usa un protocolo binario propio (IoTProtocol) sobre UDP en LAN WiFi. El código está en el repo `ydiaz1699/wifi_PIR` en GitHub. Hay dos versiones: V3.5.1 (producción, protocolo texto) y V4.3 (desarrollo, protocolo binario con biblioteca reutilizable).

**REGLAS CRÍTICAS** (leer antes de modificar):
1. NUNCA bloquear el loop del receptor más de 100ms (ver BUG-001, BUG-006)
2. Los sensores en el emisor deben ser 100% independientes (ver BUG-002)
3. No usar `LOCAL` como identificador (es macro del SDK ESP8266, ver BUG-004)
4. `WiFiClient::connect()` en ESP8266 bloquea ~5s — NUNCA usarlo como "test rápido"
5. Credenciales en `secrets.h` (no versionado), nunca en `.cpp`
6. Un sensor PIR se queda en HIGH por segundos, no genera flancos limpios como un botón
7. UDP tiene prioridad absoluta sobre MQTT en el loop del receptor
8. La deduplicación necesita BOOT_ID además de SEQ (ver BUG-008)

**ARCHIVOS CLAVE**:
- `lib/IoTProtocol/` — biblioteca reutilizable (protocolo + nodo + auth + storage + config)
- `legacy/emisor_pir/src/main.cpp` — emisor producción V3.5.1
- `legacy/receptor_bocina/src/alarma.cpp` — recepción UDP producción
- `legacy/receptor_bocina/src/mqtt_cliente.cpp` — MQTT con modo LOCAL/HA
- `emisor_pir_unificado/src/main.cpp` — emisor desarrollo V4.3
- `receptor_central_unificado/src/main.cpp` — receptor desarrollo V4.3

---

## V4.4 — Seguridad y Testing (Prioridad ALTA)

### Tarea 1: Replay protection con timestamp

**Qué hacer**: Agregar protección contra replay de paquetes capturados.

**Problema actual**: La ventana de dedup (8 SEQ) protege duplicados inmediatos, pero un atacante podría capturar un paquete viejo (fuera de la ventana) y reproducirlo.

**Implementación**:
1. En `IoTNode::_sendHeartbeat()` y `sendEvent()`, agregar TLV `UPTIME_SEC` con `millis()/1000`
2. En el receptor, al recibir un paquete reliable:
   - Comparar el uptime del paquete con el último uptime conocido de ese remoto
   - Si el uptime es MENOR que el último conocido Y el BOOT_ID no cambió → reject (replay)
   - Si BOOT_ID cambió → aceptar (reinicio legítimo, uptime vuelve a 0)
3. Agregar campo `lastUptime` a `RemoteDevice` en `IoTNode.h`
4. Tolerancia: aceptar uptime menor si la diferencia es <5s (jitter de red)

**Archivos a modificar**:
- `lib/IoTProtocol/IoTNode.h` — agregar `uint32_t lastUptime` a RemoteDevice
- `lib/IoTProtocol/IoTNode.cpp` — en `_processIncoming()`, verificar uptime antes de despachar

**Test manual**: Capturar un paquete con Wireshark, esperar 60s, re-inyectar con `ncat -u 192.168.0.201 4210 < captured.bin`. Debe ser rechazado.

---

### Tarea 2: Tests unitarios para IoTProtocol

**Qué hacer**: Tests host-based (corren en PC, no necesitan hardware) para las funciones críticas.

**Implementación**:
1. Crear carpeta `tests/` en la raíz del proyecto
2. Usar PlatformIO `native` environment para compilar en PC
3. Framework: Unity (ya integrado en PlatformIO) o simple assert
4. Tests mínimos requeridos:

```
TEST 01: serialize → deserialize roundtrip (paquete válido)
TEST 02: CRC corrupto (cambiar 1 byte) → deserialize retorna false
TEST 03: Longitud incorrecta (agregar/quitar 1 byte) → false
TEST 04: Versión incorrecta (major diferente) → false  
TEST 05: TLV malformado (length > remaining) → iot_validate_tlv() false
TEST 06: TLV strict: uint16 con length=3 → getTLV_uint16() false
TEST 07: Dedup: mismo SEQ+BOOT_ID → true (duplicado)
TEST 08: Dedup: mismo SEQ, diferente BOOT_ID → false (reinicio)
TEST 09: Dedup: 9 SEQ seguidos → el primero sale de la ventana
TEST 10: HMAC sign → verify → true
TEST 11: HMAC verify con key incorrecta → false
TEST 12: HMAC verify sin TLV auth → depende de _required
TEST 13: Cola overflow: 9 eventos en cola de 8 → política aplica
TEST 14: Prioridad: URGENT se despacha antes que BACKGROUND
```

5. `platformio.ini` para tests:
```ini
[env:native_test]
platform = native
test_framework = unity
build_src_filter = -<*> +<../lib/IoTProtocol/>
```

**Archivos a crear**:
- `tests/test_protocol.cpp`
- `tests/test_dedup.cpp`
- `tests/test_auth.cpp`
- `tests/test_queue.cpp`
- `tests/platformio.ini`

---

### Tarea 3: Simulador de red IoT

**Qué hacer**: Programa en Python que simula dispositivos IoT para testear el receptor sin hardware.

**Implementación**:
1. Script Python (`tools/iot_simulator.py`) que:
   - Envía paquetes UDP al receptor con formato IoTProtocol V4 binario
   - Puede simular: PIR, timbre, temperatura, heartbeat
   - Acepta parámetros: `--device PIR01 --event MOTION --loss 20%`
   - Escucha y muestra ACKs recibidos
2. Usar `struct` de Python para serializar el formato binario
3. Implementar CRC16-CCITT idéntico al de la biblioteca C++
4. Opciones de simulación:
   - `--loss N%` — descartar N% de paquetes (simula WiFi malo)
   - `--delay Nms` — agregar latencia
   - `--duplicate` — enviar cada paquete 2 veces
   - `--corrupt` — corromper CRC (debe ser rechazado)
   - `--replay` — reenviar paquete viejo (debe ser rechazado)

**Archivos a crear**:
- `tools/iot_simulator.py`
- `tools/README.md` (instrucciones de uso)

---

## V4.5 — Nuevos sensores (Prioridad MEDIA)

### Tarea 4: Sensor de temperatura DHT22

**Qué hacer**: Nuevo emisor que publica temperatura y humedad cada 30 segundos.

**Implementación**:
1. Crear carpeta `emisor_temp_v4/`
2. Hardware: ESP8266 D1 Mini + DHT22 en pin D4
3. `device_config.h`:
   ```cpp
   #define MY_DEVICE_ID    0x40
   #define MY_DEVICE_TYPE  DeviceType::TEMP_SENSOR
   #define MY_DEVICE_NAME  "Temp Living"
   ```
4. En `main.cpp`:
   - Leer DHT22 cada 30 segundos
   - Enviar MsgType::DATA con TLV TEMPERATURE (int16, ×10) + HUMIDITY (uint16, ×10)
   - Flags: solo `IOT_FLAG_BACKGROUND` (no requiere ACK, datos continuos)
   - Heartbeat cada 60s
   - HELLO al boot
5. El receptor central ya procesa DATA con TEMPERATURE y HUMIDITY (está en event_handler.cpp)
6. No se necesitan cambios en el receptor

**Archivos a crear**:
- `emisor_temp_v4/platformio.ini`
- `emisor_temp_v4/include/device_config.h`
- `emisor_temp_v4/src/device_config.cpp`
- `emisor_temp_v4/src/main.cpp`

**Dependencias PlatformIO**: `adafruit/DHT sensor library@^1.4`

---

### Tarea 5: Sensor de puerta (reed switch)

**Qué hacer**: Emisor que detecta apertura/cierre de puerta con reed switch magnético.

**Implementación**:
1. Crear carpeta `emisor_puerta_v4/`
2. Hardware: D1 Mini + reed switch en D2 (INPUT_PULLUP, cerrado=LOW, abierto=HIGH)
3. `device_config.h`:
   ```cpp
   #define MY_DEVICE_ID    0x07
   #define MY_DEVICE_TYPE  DeviceType::DOOR_SENSOR
   #define MY_DEVICE_NAME  "Puerta Principal"
   ```
4. Lógica:
   - Detectar cambio de estado (flanco subida=abierta, bajada=cerrada)
   - Enviar EVENT con EventCode::DOOR_OPEN o DOOR_CLOSE
   - Flags: ACK_REQUIRED + RELIABLE (un evento de puerta es crítico)
   - Al boot: enviar STATE_REPORT con estado actual del pin
5. Deep sleep entre detecciones (opcional, para batería)

**Archivos a crear**: misma estructura que emisor_temp_v4/

---

### Tarea 6: Relé controlable (actuador)

**Qué hacer**: Dispositivo que recibe COMMANDs y activa un relé.

**Implementación**:
1. Crear carpeta `actuador_relay_v4/`
2. Hardware: D1 Mini + módulo relé en D1
3. `device_config.h`:
   ```cpp
   #define MY_DEVICE_ID    0x60
   #define MY_DEVICE_TYPE  DeviceType::RELAY
   #define MY_DEVICE_NAME  "Relay Garage"
   ```
4. Lógica:
   - En `onPacketReceived()`: si MsgType::COMMAND → extraer CMD_STATE (ON/OFF/TOGGLE)
   - Activar/desactivar relé según comando
   - Enviar MsgType::RESPONSE con RESULT_CODE=OK + RESULT_STATE=(estado actual)
   - CMD_ID del original → RESULT_CMD_ID en respuesta (matching)
   - En STATE_REQUEST → responder STATE_REPORT con STATE_RELAY
5. La central necesita una función `sendCommand()` que genere COMMAND con CMD_ID

**Archivos a crear**: misma estructura que emisor_temp_v4/

**Cambios en receptor**: agregar handler para RESPONSE (logear resultado), agregar MQTT topic `casa/iot/device_60/set` que al recibir "ON"/"OFF" envíe COMMAND al relé.

---

## V4.6 — Infraestructura (Prioridad BAJA)

### Tarea 7: DHCP + Discovery real (sin IPs fijas)

**Qué hacer**: Eliminar la necesidad de IPs estáticas.

**Implementación**:
1. Dispositivos arrancan con DHCP (sin `WiFi.config()`)
2. Central tiene IP fija O usa mDNS (`central-iot.local`)
3. HELLO incluye la IP actual del dispositivo
4. La central mantiene tabla IP dinámica desde HELLO/HEARTBEAT
5. Si un dispositivo cambia IP (DHCP renewal), la central se actualiza automáticamente

**Problema conocido**: mDNS en ESP8266 es frágil en Windows (no resuelve `.local`). Alternativa: que los emisores siempre conozcan la IP de la central (configurada en LittleFS), y la central les responde desde su IP actual.

---

### Tarea 8: OTA distribuido

**Qué hacer**: Actualizar firmware de nodos remotos desde la central.

**Implementación** (concepto, no trivial):
1. Central almacena el firmware.bin en LittleFS/SPIFFS
2. Protocolo OTA propio sobre IoTProtocol:
   - Central → Nodo: CONFIG con `CFG_OTA_URL` o `CFG_OTA_START`
   - Nodo: descarga firmware via HTTP desde la central
   - Nodo: verifica hash, aplica, reinicia
3. Alternativa más simple: enviar CONFIG con `CFG_REBOOT` + que el nodo al boot intente OTA desde un servidor HTTP específico

**Complejidad**: Alta. Dejarlo para cuando haya >5 nodos y sea impracticable flashear por USB.

---

### Tarea 9: Cifrado AES-128-GCM

**Qué hacer**: Agregar confidencialidad (el contenido no es visible en la red).

**Implementación**:
1. Después de serializar payload TLV, cifrarlo con AES-128-GCM
2. El "payload" en el wire format pasa a ser el ciphertext + nonce (12 bytes) + auth tag (16 bytes)
3. BearSSL en ESP8266 soporta AES-GCM
4. Flag nuevo: `IOT_FLAG_ENCRYPTED`
5. Nonce: BOOT_ID(2) + SEQ(4) + random(6) = 12 bytes (nunca se repite)

**Costo**: +28 bytes overhead por paquete (nonce + tag). Con 64B max payload, quedan 36B para TLV real. Puede requerir ampliar MAX_PAYLOAD.

**Prioridad**: Solo si hay concern real de confidencialidad (ej: datos médicos, cerraduras).

---

### Tarea 10: Persistencia de estado en receptor

**Qué hacer**: Que el receptor recuerde el estado de los dispositivos entre reinicios.

**Implementación**:
1. En LittleFS del receptor: guardar tabla de dispositivos conocidos
2. Al boot: cargar tabla → los dispositivos aparecen como OFFLINE hasta que envíen HELLO/HEARTBEAT
3. Guardar: ID, tipo, nombre, último estado, último uptime
4. Actualizar tabla cada 5 minutos (no cada paquete, para no desgastar flash)

---

## Mejoras de calidad de vida

### Tarea 11: Alertas de OFFLINE en Home Assistant

**Qué hacer**: Que cuando un dispositivo pase a OFFLINE, HA envíe notificación.

**Implementación**: No requiere cambios en firmware. Crear automation en HA:
```yaml
automation:
  - alias: "Alerta sensor offline"
    trigger:
      - platform: state
        entity_id: sensor.device_02_status
        to: "offline"
        for: "00:03:00"
    action:
      - service: notify.mobile_app
        data:
          message: "PIR Entrada está offline hace 3 minutos"
```

---

### Tarea 12: Dashboard HA con todos los sensores

**Qué hacer**: Crear dashboard automático con MQTT Discovery.

**Implementación**:
1. En el receptor, al recibir HELLO de un dispositivo nuevo:
   - Publicar HA MQTT Discovery config para ese device
   - Topic: `homeassistant/sensor/{device_id}/config`
   - Incluir: nombre, tipo, state_topic, availability
2. Ya existe algo de esto en `mqtt_discovery.cpp` del receptor V3.x — extenderlo para V4

---

### Tarea 13: Modo "test" para el PIR

**Qué hacer**: Que el PIR pueda activarse rápidamente para testing sin esperar el timeout del módulo HC-SR501.

**Implementación**:
1. Agregar un segundo pin como "PIR test" (ej: D7) conectado a un botón
2. Al presionar D7: enviar MOTION inmediatamente (antirebote 100ms)
3. El pin real del PIR (D2) sigue con su lógica normal
4. En producción: no conectar nada a D7 (pull-up, no genera eventos)

Alternativa: comando CONFIG desde la central que active "modo test" (antirebote mínimo + log verbose).

---

## Notas para el LLM futuro

### Al modificar el emisor:
- El loop debe correr a máxima velocidad (<1ms por iteración)
- Nunca `delay()` en el loop (excepto `delayMicroseconds` para timing crítico)
- Los sensores se leen por flanco o por nivel, nunca por polling bloqueante
- `enviarEvento()` NUNCA debe esperar ACK — es fire-and-forget con tracking async

### Al modificar el receptor:
- `manejarAlarma()` debe ser lo PRIMERO después de WiFi/buzzer
- Si agregan una función que puede bloquear (HTTP, TCP connect, DNS):
  - Solo llamarla si `!buzzer.isOn()`
  - Solo llamarla si pasó suficiente tiempo desde el último intento
  - NUNCA en cada iteración del loop
- El receptor es "tonto" respecto a sensores — procesa por EventCode genérico

### Al agregar un nuevo sensor:
1. Crear carpeta `emisor_XXX_v4/`
2. Solo cambiar `device_config.h` (ID, tipo, nombre, pines)
3. El `main.cpp` es casi idéntico: WiFi + node.begin() + loop con lectura de sensor + node.sendEvent()
4. NO modificar la biblioteca IoTProtocol
5. NO modificar el receptor (debe procesar genéricamente)
6. Si el sensor necesita un `EventCode` nuevo, agregarlo al perfil correspondiente, normalmente `lib/AlarmProfile/AlarmProfile.h`; no modificar `IoTProtocol.h` por vocabulario específico de la alarma.

### Al modificar IoTProtocol:
- El wire format está CONGELADO para V4.x (14 bytes header)
- Se puede agregar nuevos MsgType, TlvTag, EventCode sin romper compatibilidad
- NUNCA cambiar el orden/tamaño de campos existentes en la cabecera
- Si se necesita un cambio breaking: incrementar PROTOCOL_MAJOR
- Los tests unitarios (cuando existan) deben pasar antes de commit

### Errores comunes a evitar:
- No usar palabras reservadas del SDK como identificadores (`LOCAL`, `REMOTE`)
- No asumir que `setSocketTimeout()` afecta `connect()` en ESP8266
- No hacer `espClient.connect()` como "ping" — bloquea 5 segundos
- No guardar credenciales en archivos versionados
- No diseñar con un solo canal TX si hay múltiples sensores
- No usar antirebote >500ms para PIR en testing (el módulo tiene su propio delay)
- No hardcodear IPs de broadcast — usar WiFi.broadcastIP()



---

### Backlog consolidado de ideas históricas

Esta sección resume el backlog conservado desde los drafts retirados. La trazabilidad completa está en [`DRAFTS_AUDIT.md`](DRAFTS_AUDIT.md) y el detalle de capabilities en [`universal-protocol/CAPABILITY_DISCOVERY.md`](universal-protocol/CAPABILITY_DISCOVERY.md).

### Regla de estado

Cada entrada debe avanzar por estos estados, en orden:

```text
PROPUESTO → APLICADO → COMPILADO → VERIFICADO → VERIFICADO EN HARDWARE
```

Que una idea aparezca en este roadmap solo significa que fue conservada y tiene un destino. No significa que exista en el firmware.

### Prioridad 0 — cerrar la línea base V4

Antes de implementar el backlog de producto, completar las fases 0–7 de `PLAN_EJECUCION_FUTURA.md`:

- conectar `BOOT_ID` persistente de `IoTStorage` con `IoTNode`;
- autenticar antes de ACK, registry, deduplicación y callbacks;
- decidir la ventana anti-replay para UDP fuera de orden;
- definir HMAC, provisión/rotación de claves y comportamiento ante paquetes inválidos;
- crear tests host y un simulador reproducible;
- validar con hardware sin romper V3.5.1.

**Estado:** pendiente. **Dependencia:** todas las tareas de este backlog, excepto la documentación y el registro histórico. **Criterio:** la definición de “terminado” de la sección 12 del plan se cumple o cada excepción queda documentada.

### Prioridad alta — event log de la central

**Objetivo:** conservar los últimos 100 eventos para que el sistema sea diagnosticable y no dependa solo del estado actual.

**Alcance inicial:** central V4, no el núcleo universal. Un registro debe distinguir al menos `timestamp/uptime`, dispositivo, tipo de evento, origen y resultado. Ejemplos de categorías: evento de sensor, cambio ONLINE/STALE/OFFLINE, ACK timeout, conexión MQTT y transición de alarma.

**Diseño por etapas:**

1. Definir un schema versionado y un tamaño máximo fijo.
2. Implementar primero un ring buffer en RAM para probar orden, overflow y consulta.
3. Decidir después si la persistencia en LittleFS es necesaria; no escribir flash en cada paquete.
4. Exponer una consulta interna o adapter MQTT/HA sin introducir topics específicos en el Core.
5. Definir qué eventos no deben incluir secretos ni payloads sensibles.

**Archivos candidatos:** `receptor_central_unificado/`, un módulo de diagnóstico/registro separado y el adapter MQTT; no modificar `IoTProtocol` hasta que el contrato esté justificado.

**Criterios de aceptación:** 100 entradas máximo, orden estable tras overflow, reinicio probado si se elige persistencia, consulta reproducible y consumo de RAM/flash medido.

**Estado:** NUEVO / PROPUESTO. **Dependencias:** línea base V4, schema de telemetría y política de persistencia.

### Prioridad alta — modelo de estado y telemetría de dispositivos

**Objetivo:** convertir el registry parcial en un estado operativo completo sin mezclarlo con estados de alarma.

**Datos candidatos:** ID, nombre, tipo, IP observada, RSSI, uptime, `BOOT_ID`, último `SEQ`, firmware, batería si existe, errores, último heartbeat y estado de disponibilidad.

**Pasos:**

1. Enumerar qué campos ya existen en `RemoteDevice`, HELLO y heartbeat.
2. Separar datos de identidad, métricas, disponibilidad y estado de aplicación.
3. Definir un schema compatible con unknown fields y una frecuencia de publicación.
4. Publicar solo mediante un adapter/gateway MQTT cuando el contrato esté validado.
5. Probar cambios de IP, reinicio, heartbeat retrasado y transición ONLINE → STALE → OFFLINE.

**Criterios de aceptación:** la central reconstruye el estado de un nodo tras HELLO/heartbeat, no confunde reboot con replay y publica valores verificables; no se declara “dashboard listo” solo porque exista un log.

**Estado:** MEJORA / PROPUESTO. **Dependencias:** `BOOT_ID`, anti-replay, contrato HELLO/heartbeat y pruebas MQTT.

### Prioridad media — máquina de estados del perfil alarma

**Objetivo:** modelar el dominio de alarma como perfil de aplicación, no como requisito del núcleo universal.

**Estados candidatos:** `DISARMED`, `ARMING`, `ARMED`, `TRIGGERED`, `ALARMING`, `ACKNOWLEDGED`, `MAINTENANCE`.

**Pasos:**

1. Especificar una tabla de transiciones, eventos válidos y permisos.
2. Definir qué ocurre con una alarma durante reinicio, pérdida de MQTT o pérdida de un sensor.
3. Separar el estado de alarma del estado de conectividad del dispositivo.
4. Definir comandos autenticados para armar, desarmar, reconocer y cancelar.
5. Probar transiciones normales, repetidas, inválidas y recuperación después de reinicio.

**Criterios de aceptación:** no existen transiciones implícitas; cada transición tiene evento, precondición, efecto y prueba; V3 no cambia mientras el perfil se prototipa en V4.

**Estado:** MEJORA / PROPUESTO. **Dependencias:** autenticación, `COMMAND`/`RESPONSE` y decisión de perfil alarma.

### Prioridad media — modos y zonas

**Objetivo:** permitir que sensores pertenezcan a zonas y que los modos `HOME`, `AWAY`, `NIGHT` y `MAINTENANCE` seleccionen reglas distintas.

**Límite arquitectónico:** zona, modo y política de alarma pertenecen al perfil alarma o a la central. No deben convertirse automáticamente en campos obligatorios de cada paquete universal.

**Pasos:**

1. Definir IDs estables de zona y su configuración en la central.
2. Definir la relación dispositivo → zona y su persistencia.
3. Especificar qué sensores se habilitan en cada modo.
4. Autorizar y validar cambios de modo/configuración.
5. Probar una alarma con sensores activos, inactivos, desconocidos y fuera de línea.

**Criterios de aceptación:** una configuración de zona no cambia el wire format universal, sobrevive al reinicio según la política elegida y tiene un comportamiento seguro si la configuración es inválida.

**Estado:** NUEVO / PROPUESTO. **Dependencias:** máquina de estados, configuración autenticada y persistencia.

### Prioridad media — configuración remota completa

**Objetivo:** completar el flujo central → nodo → respuesta para heartbeat, debounce, nombre, zona, prioridad y modo, manteniendo autenticación obligatoria para comandos.

**Pasos:**

1. Especificar `COMMAND`, `CONFIG`, `RESPONSE`, `CMD_ID`, código de resultado y versión de configuración.
2. Implementar un solo caso extremo a extremo antes de generalizar.
3. Verificar autenticación y anti-replay antes de aplicar cambios.
4. Guardar cambios de forma atómica y validar rangos.
5. Responder con resultado y configuración efectiva, sin incluir secretos.
6. Definir rollback de configuración inválida y comportamiento si LittleFS falla.

**Criterios de aceptación:** una configuración autenticada se aplica, persiste y puede consultarse; una no autenticada no produce efectos; un timeout no deja al nodo en un estado ambiguo.

**Estado:** MEJORA / PROPUESTO. **Dependencias:** Fases 2–5, `IoTStorage` y contrato de comandos.

### Prioridad media — capability discovery y extensibilidad

La especificación detallada está en [`docs/universal-protocol/CAPABILITY_DISCOVERY.md`](universal-protocol/CAPABILITY_DISCOVERY.md). Ese documento conserva la idea original completa, distingue el estado real V4 del diseño propuesto y define wire contract, IDs iniciales, registry, compatibilidad, API candidata, MQTT adapter, etapas y pruebas.

**Objetivo:** que la central conozca las capacidades reales de cada dispositivo y permita añadir perfiles sin crear protocolos paralelos.

**Estado actual real:** V4 tiene `HELLO`, `DeviceType` y un tag `CAPABILITY` declarado, pero no transmite, parsea, almacena ni publica capabilities. `HELLO_ACK` tampoco es hoy un handshake de aplicación; el ACK existente es automático y de protocolo.

**Implementación futura resumida:**

1. Congelar el registro V1 y probar TLV repetidos `CAPABILITY` de un byte dentro de `HELLO`.
2. Separar capability declarada, evento, estado, `DeviceType` y autorización.
3. Añadir almacenamiento de capabilities al registry sin romper nodos V4 antiguos.
4. Hacer que la central seleccione handlers/adapters por capability, no por nombres hardcodeados.
5. Validar dos perfiles distintos, por ejemplo `MOTION + TEMPERATURE + BATTERY` y `DOOR + BATTERY`.

**Criterios de aceptación:** la central identifica capacidades sin conocer pines ni lógica de producto, ignora extensiones desconocidas de forma segura, reemplaza el conjunto al cambiar de sesión y no rompe nodos antiguos.

**Estado:** MEJORA / PROPUESTA IMPLEMENTABLE; no aplicada. **Dependencias:** contrato HELLO, versionado, auth temprana, registry y pruebas de compatibilidad.

### Prioridad media — dashboard y adapter MQTT/Home Assistant

**Objetivo:** presentar disponibilidad, telemetría, eventos y diagnóstico sin introducir Home Assistant en el núcleo universal.

**Pasos:**

1. Definir topics, payloads, `unique_id`, availability y retención.
2. Separar estado, eventos y comandos; no usar un único JSON ambiguo para todo.
3. Verificar que discovery usa JSON válido y que el payload retained real llega al broker.
4. Probar alta, actualización, offline y reinicio de Home Assistant.
5. Documentar la configuración manual además de la importación/discovery automática.

**Criterios de aceptación:** payload retained válido, entidades sin colisiones, estado offline reproducible y comandos rechazados si no están autenticados. Un log de `publish()` no es evidencia suficiente.

**Estado:** MEJORA / PROPUESTO. **Dependencias:** BUG-011 verificado, schema de telemetría y contrato de comandos.

### Prioridad baja — OTA controlada con rollback

**Objetivo:** actualizar nodos remotos sin convertir una imagen inválida o una interrupción de energía en un dispositivo irrecuperable.

**Pasos:**

1. Definir quién autoriza la actualización y cómo se autentica la orden.
2. Verificar firma/hash, tamaño, versión y compatibilidad antes de escribir.
3. Usar una estrategia de rollback real soportada por la plataforma o documentar recuperación física.
4. Reportar estados `REQUESTED`, `DOWNLOADING`, `VERIFIED`, `APPLIED`, `BOOT_OK` y `FAILED`.
5. Probar imagen corrupta, pérdida de red, reinicio durante actualización y arranque posterior.
6. No mezclar el procedimiento de firewall de Windows con una garantía del firmware.

**Criterios de aceptación:** una imagen inválida no se aplica, una actualización interrumpida tiene recuperación definida y la central distingue éxito de “reinició sin confirmar”.

**Estado:** MEJORA / PROPUESTO. **Dependencias:** seguridad de plataforma, watchdog, storage y pruebas hardware.

### Prioridad condicional — cifrado y confidencialidad

La idea histórica de AES-GCM se mantiene solo si existe un requisito real de confidencialidad. HMAC no cifra. Antes de implementarlo hay que comparar overhead, nonce, RAM/flash, payload máximo y compatibilidad; no adoptar la propuesta histórica de nonce sin demostrar unicidad.

**Estado:** VARIANTE / CONDICIONAL. **Dependencia:** modelo de amenazas y decisión de arquitectura universal.

### Priorización consolidada

```text
línea base y seguridad V4
    ↓
event log + estado/telemetría
    ↓
configuración autenticada + capabilities
    ↓
perfil alarma: estados + modos + zonas
    ↓
MQTT/HA diagnosticable + dashboard
    ↓
OTA con rollback
    ↓
evaluación V5 y nuevos sensores/actuadores
```

La sirena V3 no bloqueante permanece en la **Fase 8** del plan y no se duplica aquí. Los sensores DHT22, puerta y relé ya descritos en este roadmap siguen subordinados a la estabilización de V4 y a sus contratos de perfil.
