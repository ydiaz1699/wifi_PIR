# TESTING — Cómo verificar

## 1. Compilación por entorno (verificación primaria)

Es la red de seguridad principal: compila y enlaza el firmware real + la lib
compartida.

```
cd emisor_pir_unificado        && pio run -e emisor_unificado
cd receptor_central_unificado  && pio run -e receptor_central_unificado
```

Si tocas `lib/IoTProtocol`, compila **ambos** entornos (el receptor usa la misma
lib). Señal de éxito: `[SUCCESS]` / `1 succeeded`. `pio run` puede salir con
exit code 1 por el banner "Obsolete PIO Core"; eso **no** es fallo de build.

Referencia de tamaños esperados (último informe): emisor RAM ~42 %, Flash ~35 %;
receptor RAM ~45 %, Flash ~36 %.

## 2. Checks host (g++)

`tests/host_compile/` reusa las fuentes reales de `lib/` con stubs mínimos de
Arduino/ESP8266/WiFiUDP/BearSSL en `compat/`. Instrucciones completas en
`tests/host_compile/README.md`.

> **Nota:** en la máquina de desarrollo actual `g++` **no está instalado**, así
> que estos checks no se han podido ejecutar. Instalar g++ para habilitarlos.

Qué validan:
- `check.cpp`: conversiones de `AlarmProfile`, serialización/deserialización,
  TLV, rechazo de CRC/paquetes truncados, `IoTNode::begin/sendHello/sendEvent`,
  nombre de dispositivo de 23 caracteres. Esperado: `OK: host compile check wifi_PIR`.
- `check_auth.cpp` (requiere OpenSSL, `-lcrypto`): vector HMAC conocido.
  Esperado: `OK: HMAC vector check wifi_PIR`.
- `tests/storage_host/storage_check.cpp`: hardening de `IoTStorage` con LittleFS
  en memoria (truncamiento, fallo de escritura). Esperado:
  `OK: storage hardening check wifi_PIR`.

Un `FAIL:` o exit code distinto de 0 indica problema. Los warnings de `memset`
en `IoTNode.cpp` no son fallos.

## 3. Entorno `native` de PlatformIO

`tests/platformio.ini` + `tests/test/test_protocol/` para tests unitarios en PC
(`pio test` desde `tests/`). También hay un simulador Python en
`tests/python/test_iot_simulator.py`.

## 4. Tests incrementales recomendados (requieren hardware/host)

Del análisis profundo (`.agents/tasks/analisis-profundo-emisor-pir.md`, §7).
Cada uno aísla una variable:

- **B1** — enlace del env emisor (hoy pasa; antes fallaba por `manejarWiFi`).
- **B5** — jitter de heartbeat: período ∈ [55, 65] s sin underflow.
- **B7** — saturación de cola: encolar >8 EVENT sin WiFi; `sendEvent` debe
  propagar el fallo (`queueDrops` crece).
- **B9** — rangos en `loadConfig`: `config.json` con `antirebote_ms=0` y checksum
  válido debe sanearse/rechazarse.
- **B3** — OTA `onEnd` sin reset: `otaEnProgreso()` debe volver a `false`.
- **B11** — replay por BOOT_ID degradado: dos sesiones con mismo bootId volátil.
- **B14** — flanco de timbre corto durante ráfaga UDP con auth.
