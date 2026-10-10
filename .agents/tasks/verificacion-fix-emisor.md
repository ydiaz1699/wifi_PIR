# Verificación — Arreglos del emisor PIR unificado (IoTProtocol V4.3)

Primera iteración (no existía `review-fix-emisor.json`). Se implementaron todos
los items del plan `plan-fix-emisor.md` (B1–B12, B15 y B-GIT). Sin commit ni push:
los cambios quedan en el working tree.

## Entorno de verificación

- **PlatformIO Core 6.1.18**: DISPONIBLE. Es la verificación primaria (compila y
  enlaza el firmware real + la lib compartida).
- **g++**: NO DISPONIBLE en esta máquina (`g++` no se reconoce como comando).
  Por tanto NO se pudieron ejecutar los checks host unitarios
  (`tests/host_compile/*`, `tests/storage_host/*`), que dependen de g++. La
  verificación de B5/B7/B9 se realizó mediante `pio run` de ambos entornos
  (que compilan la lib real) y por lectura del código.

## Comandos ejecutados y resultados

### 1. Compilación del emisor (regresión de B1: `manejarWiFi()`)

```
cwd: emisor_pir_unificado
pio run -e emisor_unificado
```

Resultado: **SUCCESS** — `Linking .pio\build\emisor_unificado\firmware.elf` OK,
`[SUCCESS] Took 47.66 seconds`, `1 succeeded`.
RAM 42.4% (34708/81920), Flash 35.0% (366036/1044464).

Antes de este arreglo el enlace fallaba con `undefined reference to manejarWiFi()`.
Ahora enlaza: `manejarWiFi()` está implementado en `src/main.cpp` y `loop()` sigue
llamando a `manejarWiFi()` y `handleOTA()`. **B1 resuelto** (regresión superada).

> Nota: `pio run` termina con exit code 1 por el aviso de "Obsolete PIO Core" del
> banner de la herramienta; NO es un fallo de compilación. La señal real es el
> `[SUCCESS]` y `1 succeeded` que imprime el propio build.

### 2. Compilación del receptor (no romper la lib compartida)

```
cwd: receptor_central_unificado
pio run -e receptor_central_unificado
```

Resultado: **SUCCESS** — `Linking ... firmware.elf` OK,
`[SUCCESS] Took 48.28 seconds`, `1 succeeded`.
RAM 44.8% (36696/81920), Flash 36.1% (376808/1044464).

Confirma que los cambios en `lib/IoTProtocol` (B5 heartbeat, B7+B8 sendEvent/URGENT,
B9 loadConfig, B10 handleConfig, B11 volatile boot id, y los nuevos defines de rango
en `IoTProtocol.h`) siguen compilando y enlazando para el receptor. No hubo cambios
de firma pública ni de enum que rompan al receptor.

## Clave HMAC (B6) — IOT_AUTH_KEY emisor vs receptor

En este repo existe **un único `secrets.h`** en la raíz (NO versionado), incluido
tanto por el emisor (`#include "secrets.h"` + `-I../`) como por el receptor
(`receptor_central_unificado/include/config.h` → `#include "secrets.h"` + `-I../`).
`file_search` confirma que no hay otro `secrets.h` por subproyecto.

Por tanto, al editar ese único archivo con la nueva clave de 16 bytes
(`IOT_AUTH_KEY_LEN 16`), emisor y receptor quedan con exactamente los mismos
16 bytes **por construcción** (no hay dos archivos que sincronizar).

Resultado: **coinciden**. (No se transcribe el valor de la clave.)

El `secrets.h.template` versionado conserva su placeholder ASCII original; NO se
puso la clave real en el template.

## Estado de git (B-GIT) — SIN commit

Comandos ejecutados desde la raíz:

```
git rm --cached network_config.h      -> rm 'network_config.h' (destrackeado, archivo local intacto)
git add network_config.h.template     -> plantilla versionada creada
```

Verificación de índice:

- `git ls-files network_config.h`           -> (vacío)  ✅ ya no versionado
- `git ls-files network_config.h.template`  -> aparece  ✅ versionado
- `git ls-files secrets.h`                  -> (vacío)  ✅ sigue sin versionarse
- `git ls-files | Select-String "secrets|network_config"`:
  - `network_config.h.template`  ✅
  - `secrets.h.template`         ✅
  - (NO aparecen `network_config.h` ni `secrets.h`)  ✅

`git status --short` relevante:
```
 M emisor_pir_unificado/include/device_config.h
 M emisor_pir_unificado/src/device_config.cpp
 M emisor_pir_unificado/src/main.cpp
 M emisor_pir_unificado/src/ota.cpp
 M lib/IoTProtocol/IoTConfigHandler.cpp
 M lib/IoTProtocol/IoTNode.cpp
 M lib/IoTProtocol/IoTNode.h
 M lib/IoTProtocol/IoTProtocol.h
 M lib/IoTProtocol/IoTStorage.cpp
D  network_config.h
A  network_config.h.template
```

No se reescribió historial, no se hizo `git rm` de `secrets.h`, no hubo commit ni push.

## Resumen de arreglos por item

- **B1+B2+B4**: `manejarWiFi()` implementado (máquina de estados no bloqueante con
  backoff ~5 s vía `iniciarWiFi()`); llama a `setupOTA()` al transicionar a
  `WL_CONNECTED` (idempotente). Eliminada la espera bloqueante `while(... 10000)` de
  `setup()` y el log engañoso de "OTA se inicializará al reconectar".
- **B3**: `onEnd` en `ota.cpp` pone `actualizacionEnProgreso = false` antes del log.
- **B5**: `_lastHb = ahora` (sin sesgo); jitter aplicado al umbral de comparación con
  guarda anti-underflow (umbral acotado a ≥0). Nuevo miembro privado `_hbJitter`.
- **B6**: clave HMAC rotada en el `secrets.h` real; template intacto.
- **B7+B8**: `sendEvent()` reserva SEQ sin consumirlo y solo avanza `_seq` si
  `enqueue()` tiene éxito (sin SEQ fantasma); EVENT marcado `IOT_FLAG_URGENT`
  (no descartable); `main.cpp` captura el bool y loguea MOTION/TIMBRE perdido con
  `LOG_WARN` cuando la cola está llena.
- **B9**: `loadConfig()` revalida antirebote (100–10000) y heartbeat (5000–600000)
  tras checksum, saneando a defaults; constantes centralizadas en `IoTProtocol.h`.
- **B10**: `handleConfig()` responde `INVALID_VALUE` si llegaron TLV de config pero
  ninguno fue válido por rango (`cfgTlvsPresentes > 0 && changesApplied == 0`).
- **B11**: comentario del modo degradado del BOOT_ID volátil y su efecto de replay en
  la central; entropía aumentada (`micros()` + `ESP.getCycleCount()` +
  `ESP.getChipId()` + varias lecturas de `analogRead(A0)`), manteniendo `uint16_t` y `id != 0`.
- **B12**: `detectBootReason()` usa `ESP.getResetInfoPtr()->reason` como fuente
  primaria y el texto de `getResetReason()` solo como fallback.
- **B15**: `device_config.h` corregido a V4.3; puerto UDP consolidado en
  `IOT_UDP_PORT_DEFAULT` (network_config.h / .template), usado por `PUERTO_UDP` y
  `UDP_PORT`.
- **B-GIT**: `network_config.h` destrackeado, `network_config.h.template` creado y
  versionado; `secrets.h` sigue sin versionarse.
