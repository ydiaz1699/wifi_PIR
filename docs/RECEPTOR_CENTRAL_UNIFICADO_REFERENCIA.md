# `receptor_central_unificado` — Documento de referencia condensado

- **Tipo:** firmware embebido / central IoT ESP8266 con gateway UDP–MQTT
- **Generado a partir de:** código fuente completo del componente `receptor_central_unificado`, sus headers, `platformio.ini`, README del componente y dependencias directas que cambian su contrato: `IoTProtocol`, `IoTNode`, `IoTAuth`, `IoTStorage` y `AlarmProfile`.
- **Estado de evidencia:** `COMPLETO` para el alcance de código declarado; ejecución real de firmware, broker, Home Assistant, OTA y hardware no confirmada.
- **Referencia de versión:** rama `docs/condensed-reference-prompt`, commit `1991ab3`; versión de aplicación declarada `4.3.0`; wire protocol `IOT_PROTOCOL_VER = 0x41`.

## Propósito

`receptor_central_unificado` es la central candidata V4.3 para recibir paquetes binarios `IoTProtocol` por UDP, validar su frontera de seguridad/sesión, procesar eventos de sensores y controlar buzzer/LED localmente. Expone estados, telemetría, eventos y comandos de alarma mediante MQTT opcional, conservando aliases MQTT V3 para compatibilidad.

## Contrato público / interfaz

### Entry points Arduino

```cpp
void setup();
void loop();
```

`setup()` inicializa serial, watchdog, HAL, `IoTStorage`, WiFi, `IoTNode`, MQTT y OTA. `loop()` ejecuta la política periódica de recepción UDP, buzzer, sincronización de estados, MQTT, publicación de estados y OTA.

### Callback UDP de aplicación

```cpp
void handleIoTPacket(const IoTPacket& pkt,
                     IPAddress remoteIP,
                     uint16_t remotePort);
```

Declarado en `receptor_central_unificado/include/event_handler.h`. `IoTNode` lo invoca solo después de validar el paquete, destino, autenticación configurada, `BOOT_ID`, ACK/reliable, sesión y deduplicación según la frontera de `IoTNode`.

### Gestión MQTT

Declaraciones públicas de `receptor_central_unificado/include/mqtt_manager.h`:

```cpp
enum class ModoMQTT {
    MODO_LOCAL,
    MODO_HA,
};

extern PubSubClient mqtt;
extern bool mqttDisponible;
extern ModoMQTT modoMQTT;

void inicializarMQTT();
void manejarMQTT();
bool consumirSolicitudStateSync();
void publicarEstadoBocina();
const char* modoMQTTStr();
```

`mqttCallback()` es interno al módulo MQTT. Acepta comandos MQTT de bocina y modo mediante los topics definidos en `config.h`; no existe una API pública de `CMD_ID`, expiración o deduplicación MQTT.

### Discovery Home Assistant

```cpp
void publicarDiscovery();
```

Declarada en `receptor_central_unificado/include/mqtt_discovery.h`. Publica configuraciones MQTT Discovery retained bajo `homeassistant/#` cuando MQTT está disponible.

### HAL local

Interfaces públicas de `receptor_central_unificado/include/hal.h`:

```cpp
class Led {
public:
    explicit Led(int pin);
    void begin();
    void on();
    void off();
    void toggle();
    bool isOn() const;
};

class Buzzer {
public:
    explicit Buzzer(int pin);
    void begin();
    void on();
    void off();
    void timedOn(unsigned long durationMs);
    void setLed(Led* led);
    void loop();
    bool isOn() const;
};
```

El `Buzzer` mantiene un único temporizador activo. `setLed()` conecta su indicación de estado con un `Led` externo.

### Mensajes UDP procesados

`handleIoTPacket()` procesa explícitamente estos `MsgType`:

- `EVENT`: exige `TlvTag::EVENT_TYPE`; `TlvTag::EVENT_VALUE` es opcional y usa `1` si falta.
- `HEARTBEAT`: consume telemetría disponible y publica datos retained.
- `HELLO`: registra identidad y publica discovery/estado del dispositivo.
- `DATA`: publica temperatura y humedad si los TLV existen.
- `STATE_REPORT`: traduce los `StateTag` conocidos a estados MQTT retained.
- `STATUS`: solo registra recepción.
- Otros tipos se registran como no procesados. No hay flujo implementado en este componente para `COMMAND`, `CONFIG`, `RESPONSE` ni `HELLO_ACK` de aplicación.

## Datos concretos

### Firmware, red y plataforma

- Plataforma PlatformIO: `espressif8266`.
- Placa: `nodemcuv2`.
- Framework: `arduino`.
- Monitor serial: `115200`.
- Dependencias declaradas: `knolleary/PubSubClient@^2.8` y `bblanchon/ArduinoJson@^6.21.5`.
- Hostname OTA: `central-iot`.
- Puerto de upload OTA: `192.168.0.201`.
- IP local: `192.168.0.201`.
- Gateway: `192.168.0.1`.
- Máscara: `255.255.255.0`.
- Puerto UDP: `4210` (`IOT_UDP_PORT`).
- ID local: `IOT_DEVICE_CENTRAL = 0x01`.
- Buzzer: `D5` (`pinBocina`).
- LED: `D6` (`pinLed`).
- Versión de aplicación declarada: `FW_VERSION = "4.3.0"`.

### IoTProtocol e IoTNode

- Versión wire: `IOT_PROTOCOL_VER = 0x41` (major `4`, minor `1`).
- Magic wire: `0xA5 0x5A`.
- Payload TLV máximo: `64` bytes (`IOT_MAX_PAYLOAD`).
- Paquete máximo: `80` bytes (`IOT_MAX_PACKET`).
- Cabecera: `14` bytes; CRC16: `2` bytes.
- Cola de salida: `8` entradas (`IOT_QUEUE_SIZE`).
- Remotos registrados como máximo: `8` (`IOT_MAX_REMOTES`).
- Reliable: máximo `5` intentos (`IOT_MAX_RETRIES`), un paquete en vuelo.
- Ventana de deduplicación: `8` secuencias (`IOT_DEDUP_WINDOW`).
- Datagrams procesados por cada `IoTNode::loop()`: máximo `8` (`IOT_MAX_RX_PER_LOOP`).
- Timeout ACK inicial: `300 ms`; máximo: `2000 ms`.
- Estado remoto `STALE`: desde `90000 ms`; `OFFLINE`: desde `180000 ms` sin paquete.

### Eventos y política acústica

Códigos `AlarmProfile::EventCode` definidos en el componente compartido:

- `MOTION = 0x01`
- `DOOR_OPEN = 0x02`
- `DOOR_CLOSE = 0x03`
- `BUTTON_PRESS = 0x04`
- `BUTTON_RELEASE = 0x05`
- `TIMBRE = 0x06`
- `SMOKE = 0x07`
- `FLOOD = 0x08`
- `TAMPER = 0x09`
- `LOW_BATTERY = 0x0A`
- `WINDOW_OPEN = 0x0B`
- `WINDOW_CLOSE = 0x0C`
- `VIBRATION = 0x0D`
- `GAS_DETECTED = 0x0E`

Duraciones acústicas confirmadas:

- `MOTION`: `1000 ms`.
- `TIMBRE`: `500 ms`.
- `BUTTON_PRESS`: `500 ms`.
- `DOOR_OPEN`: `2000 ms`.
- `SMOKE`: `5000 ms`.
- `FLOOD`: `3000 ms`.
- `TAMPER` y los eventos sin política acústica tienen duración `0`.

### Topics MQTT exactos

Topics V4 de la central:

- `casa/iot/central/estado`
- `casa/iot/central/uptime`
- `casa/iot/central/ip`
- `casa/iot/alarma/modo/set`
- `casa/iot/alarma/modo/state`
- `casa/iot/alarma/bocina/set`
- `casa/iot/alarma/bocina/state`

Aliases V3:

- `casa/alarma/evento`
- `casa/alarma/timbre`
- `casa/alarma/estado`
- `casa/alarma/bocina/set`
- `casa/alarma/bocina/state`
- `casa/alarma/modo/set`
- `casa/alarma/modo/state`
- `casa/alarma/uptime`
- `casa/alarma/ip`

Telemetría remota: `casa/iot/device_%02X/<campo>`, donde `<campo>` puede ser `evento`, `uptime`, `rssi`, `heap`, `tx_count`, `ack_timeouts`, `queue_depth`, `fw_version`, `boot_reason`, `name`, `status`, `temperatura`, `humedad`, `state/motion`, `state/button`, `state/door`, `state/relay`, `state/smoke`, `state/alarm` o `state/flood`.

### MQTT y Home Assistant

- Primer intento MQTT: después de `1000 ms`.
- Modo `MODO_HA`: reintento cada `15000 ms`.
- Después de `3` fallos consecutivos en HA, vuelve a `MODO_LOCAL`.
- Modo LOCAL: sondeo MQTT cada `300000 ms` (`5 minutos`), con primer sondeo acelerado a `60000 ms` después del fallback.
- Si el buzzer está activo en LOCAL, el sondeo se pospone `30000 ms`.
- Timeout de socket PubSubClient: `2` segundos.
- Buffer PubSubClient: `768` bytes.
- `publicarDiscovery()` genera siete entidades: movimiento, timbre, online, switch de bocina, select de modo, uptime e IP.
- `publicarDiscovery()` usa `unique_id` con prefijo `central_alarma_`, device `central_iot`, nombre `Central Alarma IoT`, fabricante `Casero`, modelo `ESP8266 NodeMCU` y `sw_version` `V4`.

### Comandos MQTT

- Payload `ON` en el topic de bocina: activa el buzzer durante `1000 ms`.
- Payload `OFF`: apaga el buzzer.
- Payload `armado`: establece `modoAlarma = "armado"`.
- Payload `desarmado`: establece `modoAlarma = "desarmado"`.
- Otros payloads se ignoran.
- No hay `CMD_ID`, expiración, nonce, respuesta de aplicación ni deduplicación MQTT.

### Configuración externa

`secrets.h` debe proporcionar, como mínimo, `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_SERVER`, `MQTT_PORT`, `MQTT_USER`, `MQTT_PASSWORD`, `IOT_AUTH_KEY` e `IOT_AUTH_KEY_LEN`. El archivo real no está incluido en el repositorio; solo se encontró `secrets.h.template` y `.gitignore` excluye secretos.

## Comportamiento no obvio / invariantes

- La frontera efectiva es `IoTNode::_processIncoming()`: parseo, CRC, TLV, destino, autenticación, rechazo de `bootId == 0`, ACK, sesión, deduplicación y registry ocurren antes de `handleIoTPacket()`.
- Un paquete reliable duplicado recibe ACK protocolario, pero no vuelve a entregarse al callback de aplicación.
- Un paquete de una sesión anterior no debe actualizar endpoint ni mantener el remoto en estado vivo.
- Si `authEnabled` es falso, el provider de autenticación queda en `IoTAuthMode::DISABLED`, que es un bypass completo. Si es verdadero, la central exige autenticación mediante `IOT_AUTH_KEY`/`IOT_AUTH_KEY_LEN`.
- HMAC autentica integridad y autenticidad, pero no cifra el contenido; la implementación compartida usa el contrato de autenticación definido por `IoTAuth`.
- La central llama `node.loop()` antes de `manejarMQTT()` en cada vuelta; un evento UDP puede activar el buzzer antes de cualquier intento MQTT de esa misma iteración.
- El `Buzzer` tiene un único temporizador: una activación posterior reemplaza el plazo anterior; no existe cola acústica.
- `MOTION` y `DOOR_OPEN` solo activan buzzer cuando `modoAlarma == "armado"`. `TIMBRE`, `BUTTON_PRESS`, `SMOKE` y `FLOOD` tienen allowlist acústica independiente del modo.
- Códigos de evento desconocidos se registran y publican, pero no activan el buzzer.
- `TAMPER` se publica, pero no activa buzzer: su política acústica está pendiente.
- `EVENT_VALUE` ausente no invalida el evento: se usa `1`. En cambio, `EVENT_TYPE` ausente hace que el evento se descarte.
- En modo LOCAL, la alarma local continúa funcionando sin broker MQTT; con `mqttDisponible == false`, las publicaciones no se guardan en una cola offline y los eventos/telemetría transitorios se pierden.
- Las publicaciones MQTT no son confirmaciones de ejecución. El código no implementa un ACK de aplicación para comandos.
- La conexión `mqtt.connect()` es síncrona; `setSocketTimeout(2)` no demuestra que el loop completo sea no bloqueante.
- `STATE_REQUEST` es best-effort: se emite al conectar WiFi en `t=0 s`, `t=3 s` y `t=10 s`, con máximo `3` intentos, y se reinicia tras recuperación de MQTT. No garantiza respuesta de todos los nodos.
- Si WiFi cae, el código fuerza `mqttDisponible = false` y reinicia la ventana de sincronización al recuperar conectividad.
- `IoTStorage::begin()` no formatea automáticamente LittleFS. Si falla, la central continúa con defaults y `BOOT_ID` degradado, programa `retryMount()` cada `5 minutos` y no reactiva retrospectivamente la persistencia del `BOOT_ID` ya consumido.
- La configuración persistente tiene nombre histórico `config.json`, pero su formato real es `key=value` con CRC16. Esta central usa explícitamente `storage.config().authEnabled`; no aplica automáticamente todas las demás claves de configuración a sus variables globales.
- El comentario de `main.cpp` menciona envío de `CONFIG` vía MQTT, pero no existe ese flujo implementado en esta central.
- La compatibilidad V3/V4 es parcial: los estados centrales tienen fan-out V3/V4, pero los aliases V3 de eventos solo se publican para `MOTION` y `TIMBRE` positivos.

## Dependencias

### Dependencias directas

- Core Arduino ESP8266: `ESP8266WiFi`, `WiFiUdp`, `LittleFS`, `ArduinoOTA`, watchdog y tipos Arduino.
- `PubSubClient` `^2.8` para MQTT.
- `ArduinoJson` `^6.21.5` para MQTT Discovery.
- `lib/IoTProtocol/IoTProtocol`: wire format, TLV, CRC16 y tipos de mensaje.
- `lib/IoTProtocol/IoTNode`: UDP, ACK, reliable, deduplicación, registry y liveness.
- `lib/IoTProtocol/IoTAuth`: firma/verificación HMAC.
- `lib/IoTProtocol/IoTStorage`: LittleFS, configuración persistente y `BOOT_ID`.
- `lib/AlarmProfile/AlarmProfile`: códigos de evento y tags de estado.
- Headers locales `config.h`, `hal.h`, `logger.h`, `mqtt_manager.h`, `event_handler.h` y `mqtt_discovery.h`.

### Supuestos externos relevantes

- Existe un ESP8266 NodeMCU v2 con GPIO, WiFi y LittleFS funcionales.
- Existe configuración real de WiFi y, si se usa MQTT, un broker accesible con sus credenciales.
- Home Assistant solo es necesario para consumir MQTT Discovery; su ejecución no forma parte del componente.
- Los nodos remotos deben hablar el contrato wire `0x41`, usar `BOOT_ID` no cero y respetar los TLV esperados.
- La red debe permitir UDP en `4210`, MQTT según `MQTT_PORT` y OTA según el entorno configurado.
- El archivo real `secrets.h`, el broker, Home Assistant, el dispositivo físico y la credencial OTA no fueron verificados.

### Dependencias no confirmadas

- Entrega efectiva MQTT, comportamiento del broker, LWT y QoS en ejecución real.
- Compatibilidad exacta del build con la versión concreta del core ESP8266 instalada en el entorno del usuario.
- Respuesta física de buzzer/LED, recepción bajo carga y recuperación OTA.

## Estado y decisiones (si aplica)

- **Implementado y confirmado por inspección de código:** central V4.3 separada de legacy V3; UDP antes de MQTT; `STATE_REQUEST` best-effort; autenticación configurable; `BOOT_ID` desde `IoTStorage`; deduplicación `BOOT_ID + SEQ`; allowlist acústica; `TAMPER` sin acción acústica; MQTT LOCAL/HA; Discovery; estados remotos `ONLINE`, `STALE` y `OFFLINE`.
- **Implementado pero no verificado completamente:** compilación de los entry points reales contra ESP8266; HMAC y LittleFS en hardware; MQTT/HA, LWT y Discovery reales; tiempo de bloqueo de `mqtt.connect()`; OTA; UDP bajo carga; sincronización `STATE_REPORT`; comportamiento físico de buzzer/LED.
- **Propuesto/pendiente:** `CMD_ID` e idempotencia MQTT; expiración/nonce para comandos; resultados observables de `publish()`; QoS y semántica de entrega; backoff con jitter; cola MQTT offline; pruebas del presupuesto de buffer `768`; publicar disponibilidad solo por cambio; flujo completo `CONFIG`/`COMMAND`/`RESPONSE`; política acústica de `TAMPER`; eventual cifrado; promoción de V4 sobre V3.
- **Descartado o fuera de alcance:** no se implementan en este documento correcciones funcionales MQTT, `CMD_ID`, múltiples reliable en vuelo, rotación HMAC, HMAC de 8 bytes ni `PING`.

## Matriz mínima de trazabilidad

| ID | Afirmación condensada | Fuente/evidencia | Estado |
|---|---|---|---|
| REF-001 | `handleIoTPacket()` recibe solo paquetes que superaron la frontera de `IoTNode`. | `receptor_central_unificado/src/event_handler.cpp::handleIoTPacket`; `lib/IoTProtocol/IoTNode.cpp::_processIncoming` | `CONFIRMADO` |
| REF-002 | El wire protocol permanece en `IOT_PROTOCOL_VER = 0x41`, con payload máximo `64` bytes. | `lib/IoTProtocol/IoTProtocol.h` | `CONFIRMADO` |
| REF-003 | `EVENT_TYPE` es obligatorio; `EVENT_VALUE` ausente usa valor `1`. | `receptor_central_unificado/src/event_handler.cpp::handleIoTPacket` | `CONFIRMADO` |
| REF-004 | `MOTION` y `DOOR_OPEN` requieren `modoAlarma == "armado"`; `TAMPER` no activa buzzer. | `receptor_central_unificado/src/event_handler.cpp::debeActivarBocina` | `CONFIRMADO` |
| REF-005 | Los eventos transitorios no se encolan cuando MQTT está caído. | `receptor_central_unificado/src/event_handler.cpp`; `src/mqtt_manager.cpp` | `CONFIRMADO` |
| REF-006 | Los comandos MQTT no tienen `CMD_ID` ni deduplicación MQTT. | `receptor_central_unificado/src/mqtt_manager.cpp::mqttCallback`; auditoría MQTT | `CONFIRMADO` |
| REF-007 | `IoTStorage` evita formateo automático y conserva degradación de `BOOT_ID` tras retry. | `lib/IoTProtocol/IoTStorage.cpp`; `receptor_central_unificado/src/main.cpp::retryStorageIfNeeded` | `CONFIRMADO` |
| REF-008 | La compilación real ESP8266, broker, HA, OTA y hardware no están verificados en este documento. | Entorno disponible y validaciones registradas | `NO CONFIRMADO` |
| REF-009 | `CMD_ID`, cola MQTT offline, backoff con jitter y flujo `CONFIG`/`COMMAND`/`RESPONSE` son pendientes. | `docs/AUDITORIA_MQTT.md`; decisiones de la conversación | `PROPUESTO/PENDIENTE` |

## Fuera de alcance de este documento

- Implementación detallada de `emisor_pir_unificado` y `legacy`, salvo las reglas de compatibilidad necesarias.
- Cuerpos internos de `IoTProtocol`, `IoTNode`, `IoTAuth`, `IoTStorage` y `AlarmProfile`; solo se conserva su contrato que afecta a la central.
- Ejecución real en ESP8266, WiFi, UDP, MQTT, Home Assistant, OTA, LittleFS y GPIO.
- Verificación de `secrets.h` real, credenciales, broker, firewall y topología de red.
- Garantías de QoS, entrega MQTT, LWT o ejecución de comandos que no estén confirmadas por pruebas reales.
- Cambios funcionales o correcciones futuras; este archivo es una referencia, no una especificación de trabajo.
