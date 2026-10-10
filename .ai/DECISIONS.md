# DECISIONS — Decisiones de diseño

Decisiones reales del sistema, con su justificación. Fuentes: headers de
`lib/IoTProtocol/`, `docs/` y los informes de `.agents/`.

## UDP + HMAC en vez de TLS

- Transporte **UDP** sobre LAN WiFi de confianza, no TCP/TLS. Motivo: latencia
  baja, sin handshake, footprint mínimo en ESP8266, y entrega fiable resuelta en
  la capa de aplicación (ACK + reliable + dedup) en vez de la de transporte.
- La integridad/autenticidad se cubre con **HMAC-SHA256**, no con cifrado: la
  confidencialidad del contenido no es un objetivo (ver `IoTAuth.h`).

## HMAC truncado a 4 bytes

- Se trunca el HMAC-SHA256 a **4 bytes** (`IOT_HMAC_TRUNC_SIZE`). 2^32
  combinaciones bastan para impedir inyección casual de un vecino en LAN, con
  payload mínimo (el paquete máximo es 80 B). No pretende resistir a un atacante
  dedicado; es una decisión consciente para el modelo de amenaza doméstico.

## Replay cubierto por SEQ + BOOT_ID + dedup (no por HMAC)

- El anti-replay no es criptográfico: lo da la **ventana de deduplicación** (8
  SEQ por `(src, bootId)` con bitmap). `BOOT_ID` distingue sesiones tras
  reinicio (cuando SEQ vuelve a 1). Documentado explícitamente en `IoTAuth.h`.
- Mejora futura (V4.4): replay protection adicional con `UPTIME_SEC`.

## Un solo paquete reliable en vuelo

- `ReliableChannel` admite **1** reliable a la vez. Simplifica el tracking de
  ACK y el backoff. Contrapartida conocida: un reliable sin ACK bloquea la
  promoción de los siguientes; por eso los EVENT se marcaron `URGENT` (no
  descartables) tras el fix B7.

## BOOT_ID persistente en LittleFS

- `BOOT_ID` sale de un **contador incremental** persistido (`/iot/boot_count`),
  no de un random, para garantizar sesiones distintas entre reinicios. Si
  LittleFS falla, se degrada a un ID volátil y se marca
  `isBootIdPersistent() == false` (modo degradado explícito, no silencioso).

## Secretos fuera de git / clave HMAC compartida

- `secrets.h` y `network_config.h` **no se versionan** (`.gitignore`); solo sus
  `.template`. Evita filtrar WiFi/MQTT/HMAC/OTA al repo.
- La `IOT_AUTH_KEY` es **compartida e idéntica** en emisor y receptor (clave
  simétrica para HMAC). En este repo hay un único `secrets.h` en la raíz que
  ambos incluyen, así que coinciden por construcción.
- OTA usa una credencial independiente (`OTA_PASSWORD` / variable de entorno
  `WIFI_PIR_OTA_PASSWORD`), no reutiliza la clave HMAC. OTA opera sobre LAN de
  confianza y **sin firma de imagen** (modelo de amenaza documentado).

## Fuente única de verdad para constantes

- Puerto UDP en `IOT_UDP_PORT_DEFAULT` (4210), referenciado por `UDP_PORT` del
  emisor y `PUERTO_UDP` legacy (consolidado en B15).
- Rangos de config (`IOT_CFG_*`) definidos una sola vez en `IoTProtocol.h` y
  usados tanto por `IoTConfigHandler` (ruta CONFIG) como por `IoTStorage`
  (revalidación al leer de disco).
