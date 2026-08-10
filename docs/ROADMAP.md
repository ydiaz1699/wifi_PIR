# Roadmap — Mejoras Futuras

## Instrucciones para LLM

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
- `emisor_pir/src/main.cpp` — emisor producción V3.5.1
- `receptor_bocina/src/alarma.cpp` — recepción UDP producción
- `receptor_bocina/src/mqtt_cliente.cpp` — MQTT con modo LOCAL/HA
- `emisor_pir_v4/src/main.cpp` — emisor desarrollo V4.3
- `receptor_central_v4/src/main.cpp` — receptor desarrollo V4.3

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
6. Si el sensor necesita un EventCode nuevo, agregarlo al enum en `IoTProtocol.h`

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
