# ARCHITECTURE

Arquitectura del sistema y del protocolo `IoTProtocol`. Resumen para asistentes;
el detalle extenso vive en [`docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md) —
no duplicar, referenciar.

## Capas

1. **Aplicación** (`emisor_pir_unificado/`, `receptor_central_unificado/`):
   `setup()`/`loop()`, lectura de sensores, callbacks, MQTT, OTA.
2. **Perfil de dominio** (`lib/AlarmProfile/AlarmProfile.h`): vocabulario de la
   alarma (`EventCode`, `DeviceType`, `StateTag`). Los valores numéricos son
   contrato wire; no renumerar sin versionar.
3. **Core reutilizable** (`lib/IoTProtocol/`): nodo, serialización, auth,
   storage, config. No conoce la semántica de la alarma.

## Formato wire (V4.1 / 0x41)

Fuente: `lib/IoTProtocol/IoTProtocol.h`.

```
MAGIC(2=0xA5,0x5A) VER(1) TYPE(1) SRC(1) DST(1) BOOT_ID(2) SEQ(4) FLAGS(1) PAY_LEN(1) PAYLOAD(N) CRC16(2)
```

- Header 14 B + payload (máx 64 B) + CRC16 = máx 80 B (`IOT_MAX_PACKET`).
- Payload en **TLV**: `TAG(1) LEN(1) VALUE(N)`, validación estricta de tamaño.
- `SEQ` de 32 bits; `BOOT_ID` de 16 bits resuelve "reinicio + SEQ vuelve a 1".
- CRC16-CCITT sobre todo el paquete; `iot_deserialize` exige `len == expected`,
  versión major compatible y CRC válido.

Tipos de mensaje (`MsgType`): `EVENT 0x01`, `COMMAND 0x03`, `RESPONSE 0x04`,
`ACK 0x10`, `HEARTBEAT 0x11`, `STATE_REPORT 0x13`, `STATE_REQUEST 0x14`,
`HELLO 0x20`, `HELLO_ACK 0x21`, `CONFIG 0x30`, `ERROR_MSG 0xE0`.

## IoTNode — nodo de comunicación

Fuente: `lib/IoTProtocol/IoTNode.{h,cpp}`.

- **Cola FIFO** `IOT_QUEUE_SIZE = 8`, con 3 prioridades: `URGENT > NORMAL >
  BACKGROUND`. Buffering sin WiFi: se encola aunque no haya red y se envía al
  reconectar.
- **Un solo paquete reliable en vuelo** (`ReliableChannel`). Simplifica el
  tracking de ACK; implica que un reliable sin ACK bloquea la promoción de los
  siguientes hasta que termine o expire.
- **ACK vs RESPONSE**: `ACK = recibí` (protocolo); `RESPONSE = ejecuté`
  (aplicación). ACK ligado a la sesión `BOOT_ID` conocida del remoto.
- **Backoff reliable**: `_calcBackoff(k) = 300·1.5^k` saturado a 2000 ms;
  `IOT_MAX_RETRIES = 5`. Tiempo total hasta `ackTimeout` ≈ 300+450+675+1012+1518
  ≈ **~3.95 s**.
- **Deduplicación** (en el receptor): ventana `IOT_DEDUP_WINDOW = 8` por
  `(src, bootId)` con bitmap; descarta duplicados y replays fuera de ventana.
- **Heartbeat**: intervalo por defecto 60 s con jitter ±5 s (tras el fix B5, el
  jitter se aplica al umbral de comparación, sin sesgo ni underflow).
- **Recepción acotada**: `IOT_MAX_RX_PER_LOOP = 8` datagramas por `loop()` para
  no monopolizar.
- **Registry de remotos**: `IOT_MAX_REMOTES = 8`, estados
  `ONLINE/STALE/OFFLINE` (umbrales 90 s / 180 s).

## Autenticación (HMAC)

Fuente: `lib/IoTProtocol/IoTAuth.{h,cpp}`.

- HMAC-SHA256 truncado a **4 bytes** (BearSSL del core ESP8266), añadido como
  TLV `AUTH_HMAC4` (`0xF0`) y flag `IOT_FLAG_AUTHENTICATED` (`0x10`).
- Firma **centralizada e idempotente** en `IoTNode` (`_prepareOutgoing` →
  `signPacket`); la aplicación no firma manualmente (evita doble TLV).
- Verificación **antes** de registry, ACK, dedup y callback (`_verifyIncoming`).
  Comparación en tiempo constante.
- No cifra (confidencialidad no es objetivo) y no cubre replay (eso lo hace
  SEQ + BOOT_ID + ventana de dedup). Ver `DECISIONS.md`.

## Persistencia (BOOT_ID y config)

Fuente: `lib/IoTProtocol/IoTStorage.{h,cpp}`.

- `/iot/boot_count` (contador incremental → `BOOT_ID`), `/iot/config.json`
  (key=value + CRC16), `/iot/auth.key`.
- `getBootId()` incrementa y persiste una vez por arranque; si LittleFS falla,
  devuelve un ID volátil y `isBootIdPersistent() == false` (modo degradado).
- `loadConfig()` revalida rangos tras checksum (fix B9). Montaje no destructivo
  con reintento (`retryMount`), sin formatear automáticamente.

## Config remota

Fuente: `lib/IoTProtocol/IoTConfigHandler.{h,cpp}`. La central envía `CONFIG`
con TLV; el nodo valida rangos (`IOT_CFG_*` en `IoTProtocol.h`), persiste y
responde `RESPONSE` (`OK` / `INVALID_VALUE` tras fix B10). Parámetros:
heartbeat, antirebote, nombre, auth on/off, reboot, reset stats.

## Flujo detección → central

```
PIR flanco subida → antirrebote (storage.config().antireboteMs)
  → node.sendEvent(MOTION)  // arma EVENT URGENT+ACK+RELIABLE, firma si auth
  → encola FIFO
node.loop() → _processQueue → _processReliable → UDP → espera ACK con backoff
central → ACK (SEQ coincidente) → _handleAck libera el canal
```
