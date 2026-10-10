# PROTOCOL — Reglas para trabajar con la IA en este repo

Convenciones que un asistente debe respetar al modificar `wifi_PIR`. No son el
protocolo wire (eso está en `ARCHITECTURE.md`), sino las reglas de interacción.

## Compatibilidad wire — no romper

- El formato wire es **V4.1 (`IOT_PROTOCOL_VER = 0x41`)** y lo comparten emisor
  y receptor. Cualquier cambio de header, orden de campos o semántica TLV rompe
  la interoperabilidad: **ambos nodos deben cambiarse a la vez** y versionarse.
- Los valores numéricos de `AlarmProfile` (`EventCode`, `DeviceType`,
  `StateTag`) y de `TlvTag`/`MsgType` son contrato wire. No renumerar ni
  reutilizar valores sin versionar explícitamente el contrato.
- Nuevos `EventCode`/`DeviceType`/`StateTag` del dominio alarma van en
  `lib/AlarmProfile/AlarmProfile.h`; `IoTProtocol.h` conserva solo el
  vocabulario del core (ver `docs/ROADMAP.md`).

## Firma centralizada

- La firma HMAC se hace **solo** en `IoTNode` (`_prepareOutgoing`). No firmar
  manualmente en la aplicación: produciría doble TLV `AUTH_HMAC4`.
- La verificación va en `IoTNode` **antes** de registry/ACK/dedup; no repetirla
  en el handler de la aplicación.

## Reglas de tiempo real (ESP8266)

- Nunca bloquear el `loop()` del receptor más de ~100 ms.
- Los sensores del emisor deben ser independientes entre sí.
- `WiFiClient::connect()` bloquea ~5 s en ESP8266: no usarlo como "test rápido".
- No usar `LOCAL` como identificador (macro del SDK ESP8266).
- La reconexión WiFi debe ser **no bloqueante** (máquina de estados, patrón del
  receptor); el emisor ya lo hace tras el fix B1/B2/B4.

## Secretos — fuera de git

- `secrets.h` y `network_config.h` NO se versionan (están en `.gitignore`). Solo
  se versionan sus `.template`.
- `IOT_AUTH_KEY` debe ser idéntica en emisor y receptor; en este repo hay un
  único `secrets.h` en la raíz incluido por ambos, así que coinciden por
  construcción.
- La contraseña OTA va en la variable de entorno `WIFI_PIR_OTA_PASSWORD` (env
  OTA) y en `OTA_PASSWORD` de `secrets.h`. Nunca commitear credenciales ni
  binarios (`*.bin` también están ignorados).
- Al editar o mostrar estos archivos, **nunca** transcribir el valor de una
  credencial real a documentación, logs ni informes.

## Flujo de verificación esperado

Tras un cambio: `pio run -e <env>` del subproyecto afectado (y del otro si se
tocó `lib/`), y los checks host de `tests/` si hay `g++`. Ver `TESTING.md`.
