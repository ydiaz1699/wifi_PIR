# Plan de implementación — Arreglos del emisor PIR unificado (ESP8266 / IoTProtocol V4.3)

Este plan implementa los bugs catalogados B1–B15 del análisis profundo
(`.agents/tasks/analisis-profundo-emisor-pir.md`) y la revisión inicial
(`.agents/reviews/review-emisor-pir-unificado.md`). Está ordenado por prioridad:
**B1 → B6 → B2/B4 → B7 → B3 → B5 → B9/B11 → resto (B10, B12, B15, git)**.

## Reglas de ejecución (leer antes de empezar)

- **NO worktree.** Se trabaja directamente en `c:\Users\Alex\Desktop\1-Github\wifi_PIR` tal cual está.
- **NO commit ni push.** Todos los cambios quedan en el working tree. El único paso que invoca `git` es B-GIT (git rm --cached, sin commit).
- **Todo comentario de código y log en español.**
- **No transcribir valores de credenciales** a ningún informe/log. La clave HMAC de B6 sí se escribe en el `secrets.h` real (archivo no versionado) porque es el entregable de ese arreglo.

## Contexto verificado durante la exploración

- **Build system:** PlatformIO (`pio` 6.1.18 disponible). Emisor env = `emisor_unificado` (board `d1_mini`, `platform=espressif8266`, framework arduino). Receptor env = `receptor_central_unificado` (board `nodemcuv2`). Ambos usan `build_flags = -I../` y `lib_extra_dirs = ../lib`.
- **Librería compartida:** `lib/IoTProtocol` la compilan **emisor Y receptor**. Cualquier cambio de firma/enum (B7/B8) debe seguir compilando para el receptor → por eso la verificación incluye `pio run -e receptor_central_unificado`.
- **secrets.h es ÚNICO y compartido:** solo existe `c:\Users\Alex\Desktop\1-Github\wifi_PIR\secrets.h` (raíz). Tanto el emisor (`#include "secrets.h"` + `-I../`) como el receptor (`include/config.h` → `#include "secrets.h"` + `-I../`) resuelven **el mismo archivo raíz**. Por tanto, **editar ese único `secrets.h` deja al emisor y al receptor con los mismos 16 bytes por construcción** (no hay dos archivos que sincronizar). `git ls-files` confirma: `secrets.h` NO está versionado (solo `secrets.h.template`), y `network_config.h` SÍ está versionado (hay que destrackearlo en B-GIT).
- **network_config.h es ÚNICO y compartido** (`c:\Users\Alex\Desktop\1-Github\wifi_PIR\network_config.h`), versionado hoy. `PUERTO_UDP` de ese header solo lo usa `legacy/emisor_pir` (no se compila en estos env); el emisor unificado usa `UDP_PORT` de `device_config.cpp`. El receptor usa su propio `IOT_UDP_PORT`.
- **Verificación — herramientas:** `pio` está instalado; `g++` NO está instalado en esta máquina. Los checks host (`tests/host_compile/*`, `tests/storage_host/*`) usan `g++`. Por eso la verificación **primaria** de cada item es `pio run` (compila/enlaza el firmware real, que es la prueba de regresión de B1). Donde exista un check host unitario relevante (B5/B7/B9), se indica como verificación **preferida** y, si `g++` no está disponible, se cae al `pio run` correspondiente; el coder debe registrar si no pudo correr el check host por falta de `g++`.
- **Advertencia de banner:** `pio` imprime un aviso de "Obsolete PIO Core" y sale con código ≠0 en `--list-targets`; eso NO es un fallo de compilación. La señal de éxito/fallo real es el `SUCCESS`/`FAILED` de `pio run`.

### Comandos de verificación canónicos

```
# Emisor (prueba de regresión de B1 y de todo cambio de main.cpp/ota.cpp/device_config.* y lib):
pio run -e emisor_unificado -d emisor_pir_unificado

# Receptor (confirma que los cambios en lib/IoTProtocol siguen compilando para el receptor):
pio run -e receptor_central_unificado -d receptor_central_unificado
```

Checks host (preferidos para B5/B7/B9 si `g++` está disponible; hoy NO lo está):

```
# IoTNode (B5 heartbeat, B7 cola):
g++ -std=c++14 -Wall -Wextra -Itests/host_compile/compat -Ilib/IoTProtocol -Ilib/AlarmProfile \
  tests/host_compile/check.cpp tests/host_compile/compat/arduino_stub.cpp \
  lib/IoTProtocol/IoTProtocol.cpp lib/IoTProtocol/IoTNode.cpp -o wifi_pir_host_check && ./wifi_pir_host_check
# Esperado: "OK: host compile check wifi_PIR"

# IoTStorage (B9 rangos en loadConfig):
g++ -std=c++14 -Wall -Wextra -Itests/storage_host/compat -Ilib/IoTProtocol \
  tests/storage_host/storage_check.cpp tests/storage_host/compat/storage_stub.cpp \
  lib/IoTProtocol/IoTProtocol.cpp lib/IoTProtocol/IoTStorage.cpp -o wifi_pir_storage_check && ./wifi_pir_storage_check
# Esperado: "OK: storage hardening check wifi_PIR"
```

> Si se añaden casos de prueba a estos checks (opcional, recomendado), eliminar cualquier binario temporal (`wifi_pir_*`) tras ejecutarlos; no versionarlos.

---

# Items del plan

- [ ] 1. **(B1+B2+B4) Reconexión WiFi no bloqueante + reinicialización de OTA + setup no bloqueante** en el emisor.
      Qué hacer: implementar `manejarWiFi()` en `emisor_pir_unificado/src/main.cpp` siguiendo el patrón del receptor (`receptor_central_unificado/src/main.cpp`): máquina de estados con `wifiConectando`/`ultimoIntentoWiFi` (ya existen las variables y `iniciarWiFi()`), que al estar desconectado reintente `WiFi.begin()` con backoff (reintento cada ~5000 ms vía `iniciarWiFi()`), y detecte la transición a `WL_CONNECTED`. **Diferencia respecto al receptor:** al pasar a conectado, el emisor debe llamar a `setupOTA()` para (re)inicializar OTA tras cada reconexión (`setupOTA()` ya es idempotente: hace `return` si `otaInicializada` o si no hay WiFi, así que debe permitir re-inicializar — ver nota). En `setup()`, **eliminar** la espera bloqueante `while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) { ESP.wdtFeed(); delay(100); }` y la llamada condicional a `setupOTA()`/el `LOG_WARN("WiFi no disponible: OTA se inicializará al reconectar")`; dejar que `loop()`→`manejarWiFi()` gestione la conexión. `node.begin()` y el resto de setup se ejecutan sin esperar WiFi (el core tolera encolar sin red). Corregir el log de setup para que no afirme algo falso (quitar el mensaje engañoso; el nuevo comportamiento sí reconecta y reinicializa OTA de verdad).
      Decisión de diseño: `manejarWiFi()` vive en este proyecto (no se comparte con el receptor), replicando el patrón ya probado del receptor pero añadiendo la llamada a `setupOTA()` en la transición a conectado, que es justo lo que faltaba (B2). Rationale: es el fix mínimo que resuelve el error de enlace (B1), la ausencia de reconexión/OTA (B2) y el arranque bloqueante (B4) sin tocar el core.
      Nota sobre `setupOTA()`: hoy `setupOTA()` tiene guarda `if (otaInicializada || WiFi.status() != WL_CONNECTED) return;`. Como se llama desde la transición a conectado, en el primer arranque inicializa OTA correctamente. Si se desea reinicializar tras una caída real de WiFi (ArduinoOTA sigue "begun"), basta con llamarlo en cada transición a conectado; mantener la guarda `otaInicializada` evita doble `ArduinoOTA.begin()`. No es necesario cambiar `ota.cpp` para esto (el flag `otaInicializada` persiste), así que la reconexión no reinstala OTA pero tampoco lo rompe; documentar en comentario que OTA se inicializa una vez al primer `WL_CONNECTED`.
      Files: `emisor_pir_unificado/src/main.cpp` (añadir `manejarWiFi()`, modificar `setup()`).
      Verify: `pio run -e emisor_unificado -d emisor_pir_unificado` → `SUCCESS` (antes fallaba con `undefined reference to manejarWiFi()`; que compile y enlace es la regresión de B1). Revisar que `loop()` sigue llamando `manejarWiFi()` y `handleOTA()`.

- [ ] 2. **(B6, SEGURIDAD) Rotar la clave HMAC en el `secrets.h` real** (único, compartido emisor+receptor).
      Qué hacer: en `c:\Users\Alex\Desktop\1-Github\wifi_PIR\secrets.h` (archivo NO versionado) reemplazar el valor de `IOT_AUTH_KEY` por los 16 bytes nuevos: `{ 0x35, 0x80, 0xDC, 0xBD, 0xDE, 0xE4, 0x3A, 0x75, 0x25, 0x59, 0x10, 0x88, 0xCF, 0x4F, 0x90, 0x80 }` y mantener `IOT_AUTH_KEY_LEN 16`. **No tocar `secrets.h.template`**: debe conservar el placeholder ASCII actual (`MyIoTKeySecret!!`) — NO poner la clave real en el template.
      Qué consume cada firmware: el **emisor** y el **receptor** incluyen ambos el MISMO `secrets.h` raíz (resuelto por `-I../`). Al editar ese único archivo, ambos quedan con los mismos 16 bytes automáticamente; no hay un segundo secrets.h que sincronizar. Confirmar en el propio archivo tras editar que `IOT_AUTH_KEY_LEN` sigue siendo 16 y que el array tiene exactamente 16 bytes.
      Files: `c:\Users\Alex\Desktop\1-Github\wifi_PIR\secrets.h` (editar; NO versionado). `secrets.h.template` queda intacto.
      Verify: `pio run -e emisor_unificado -d emisor_pir_unificado` y `pio run -e receptor_central_unificado -d receptor_central_unificado` → ambos `SUCCESS` (el array de 16 bytes compila en ambos). `git status --short` debe seguir SIN listar `secrets.h` (sigue ignorado). No imprimir el valor de la clave en logs.

- [ ] 3. **(B7+B8) Propagar fallo de encolado y dar prioridad no descartable a MOTION/TIMBRE; no consumir SEQ fantasma.**
      Qué hacer, en dos frentes (core + aplicación):
      (a) **Core `lib/IoTProtocol/IoTNode.cpp` — `sendEvent()`**: hoy consume `getNextSeq()` antes de poder saber si `enqueue` tendrá éxito, y `enqueue` ya se retorna, pero el emisor ignora el bool. Reordenar para **no gastar SEQ si el evento no se encola**: construir el paquete usando una SEQ reservada localmente (p. ej. calcular el candidato con `_seq + 1` sin mutar `_seq`), y solo confirmar/avanzar `_seq` cuando `enqueue` devuelva true; si `enqueue` falla, no avanzar `_seq` (revertir). Alternativa equivalente aceptada: separar "reservar seq" de "commit seq" con un helper privado. Mantener `getNextSeq()` público intacto (lo usan HELLO, heartbeat, STATE_REPORT, etc.). Añadir el flag de prioridad **URGENT** a los EVENT para que no sean descartables por overflow: en `sendEvent()`, `pkt.flags = IOT_FLAG_ACK_REQUIRED | IOT_FLAG_RELIABLE | IOT_FLAG_URGENT;`. Con URGENT, `enqueue` bajo `DROP_OLDEST_BG` sigue sin encontrar víctima BACKGROUND cuando la cola está llena de URGENT, por lo que devolverá false y **se perderá** igual en saturación extrema; eso es correcto y ahora **se reporta** (ver (b)). URGENT garantiza que un EVENT nunca sea descartado para hacer sitio a telemetría y que se promueva antes en la cola.
      (b) **Aplicación `emisor_pir_unificado/src/main.cpp` — `loop()`**: `sendEvent(...)` devuelve bool; capturarlo y loguear la verdad. Reemplazar los dos `LOG_INFO("MOTION encolado (q=%d)"...)` / `LOG_INFO("TIMBRE encolado...")` por: si true → `LOG_INFO` de encolado real; si false → `LOG_WARN("MOTION NO encolado: cola llena, evento perdido (q=%d)")` (ídem TIMBRE). No cambiar la lógica de antirrebote.
      Compatibilidad receptor: `sendEvent` ya devuelve bool (firma no cambia); añadir `IOT_FLAG_URGENT` no cambia la firma ni el enum. El receptor no llama a `sendEvent` para MOTION/TIMBRE, pero compila la misma lib → verificar que sigue enlazando.
      Decisión de diseño: se elige URGENT (ya existe en el enum `Priority`/flags, `IOT_FLAG_URGENT=0x04`) en vez de ampliar `IOT_QUEUE_SIZE`, porque no cambia el footprint de RAM ni el formato wire y expresa la semántica correcta (detecciones no son descartables). Rationale: cambio acotado, sin tocar el protocolo.
      Files: `lib/IoTProtocol/IoTNode.cpp` (`sendEvent`), `emisor_pir_unificado/src/main.cpp` (`loop`).
      Verify: preferido `tests/host_compile/check.cpp` (contiene `check_node_api` que valida `sendEvent`/cola) con g++ → `OK: host compile check wifi_PIR`. Si no hay g++: `pio run -e emisor_unificado -d emisor_pir_unificado` y `pio run -e receptor_central_unificado -d receptor_central_unificado` → ambos `SUCCESS`. (Recomendado opcional: añadir a `check.cpp` un caso que encole 9 EVENT URGENT sin WiFi y verifique que el 9º devuelve false y `queueDrops` crece; limpiar el binario tras correrlo.)

- [ ] 4. **(B3) OTA `onEnd` debe bajar el flag de progreso.**
      Qué hacer: en `emisor_pir_unificado/src/ota.cpp`, dentro del callback `ArduinoOTA.onEnd([]() {...})`, poner `actualizacionEnProgreso = false;` **antes** del `LOG_INFO("OTA finalizada; el equipo se reiniciará")`. Así, si `onEnd` se dispara sin reinicio efectivo, `otaEnProgreso()` no queda atascado en `true` y `loop()` no congela los sensores.
      Files: `emisor_pir_unificado/src/ota.cpp`.
      Verify: `pio run -e emisor_unificado -d emisor_pir_unificado` → `SUCCESS`. Revisar que `onStart` sigue poniendo `true`, `onError` y `onEnd` ponen `false`.

- [ ] 5. **(B5) Jitter de heartbeat sin sesgo ni underflow en `IoTNode::_sendHeartbeat`.**
      Qué hacer: en `lib/IoTProtocol/IoTNode.cpp`, `_sendHeartbeat()`. Hoy:
      `if (ahora - _lastHb < _hbInterval) return; long jitter = random(-5000, 5000); _lastHb = ahora + jitter;`
      Corregir para que `_lastHb` guarde **`ahora`** (no `ahora + jitter`), eliminando el sesgo acumulado y el riesgo de underflow de `unsigned long`. Aplicar el jitter al **umbral de comparación**, no al timestamp almacenado. Implementación propuesta: mantener un miembro/estático con el próximo umbral efectivo, o comparar contra `_hbInterval + jitter` acotado. Enfoque mínimo y seguro: guardar `_lastHb = ahora;` y mover el jitter a la condición de disparo del siguiente ciclo, con guarda anti-underflow: al calcular cualquier `ahora + jitter`, si `jitter < 0 && (unsigned long)(-jitter) > ahora` entonces `jitter = 0`. Resultado esperado: período efectivo centrado en `_hbInterval` ±5 s, sin underflow aunque `millis()` sea pequeño al arranque.
      Decisión de diseño: se elige "almacenar `ahora` y aplicar jitter al umbral" (recomendación principal del análisis) porque re-centra el período en 60 s en vez de sesgarlo a 55–64 s. La guarda anti-underflow es defensa en profundidad para el caso borde de arranque temprano. No cambia la firma ni el wire.
      Files: `lib/IoTProtocol/IoTNode.cpp` (`_sendHeartbeat`). Si se necesita un miembro nuevo para el umbral, declararlo en `lib/IoTProtocol/IoTNode.h` (privado); mantener compat del receptor.
      Verify: preferido `tests/host_compile/check.cpp` con g++ (compila `IoTNode`) → `OK`. Si no hay g++: `pio run -e emisor_unificado` y `pio run -e receptor_central_unificado` → ambos `SUCCESS`. (Recomendado opcional: añadir en `check.cpp` un caso que, con `random` mockeado a −5000 y `_lastHb` cerca de 0, confirme que el próximo heartbeat no se suspende ~49 días.)

- [ ] 6. **(B9) `IoTStorage::loadConfig()` debe revalidar rangos al leer de LittleFS.**
      Qué hacer: en `lib/IoTProtocol/IoTStorage.cpp`, `loadConfig()`, tras parsear `candidate` y validar checksum/esquema (justo antes de `_config = candidate;`), revalidar con los MISMOS límites que `IoTConfigHandler`: `antireboteMs` en [100, 10000], `heartbeatIntervalMs` en [5000, 600000]. Si un valor cae fuera de rango, **sanear a default** (`antireboteMs=200`, `heartbeatIntervalMs=60000`, que es lo que fija `_setDefaults()`), en vez de aceptar un `antirebote=0`/`hb=0` que satura la cola o dispara tormentas de heartbeat. Mantener constantes de rango en un solo sitio si es simple (p. ej. defines locales en la lib reutilizables); no es obligatorio deduplicar con `IoTConfigHandler`, pero sí usar los mismos números.
      Files: `lib/IoTProtocol/IoTStorage.cpp` (`loadConfig`).
      Verify: preferido `tests/storage_host/storage_check.cpp` con g++ → `OK: storage hardening check wifi_PIR`. Si no hay g++: `pio run -e emisor_unificado` y `pio run -e receptor_central_unificado` → ambos `SUCCESS`. (Recomendado opcional: añadir en `storage_check.cpp` un caso que guarde una config con `antirebote_ms=0` + checksum válido y verifique que `loadConfig()` la sanea a 200; limpiar binario tras correr.)

- [ ] 7. **(B11) Documentar el modo degradado de BOOT_ID volátil y, si es viable, aumentar su entropía.**
      Qué hacer: en `lib/IoTProtocol/IoTStorage.cpp`, `_volatileBootId()`, añadir un comentario (español) que explique el modo degradado: cuando `!isBootIdPersistent()`, el BOOT_ID puede repetirse entre reinicios, y la central puede descartar como *replay* los primeros eventos del emisor reiniciado (ventana de dedup ligada a `(src, bootId)`). Si es viable sin romper el protocolo, mejorar la entropía del id volátil combinando más fuentes (p. ej. `micros()`, varias lecturas de `analogRead(A0)`, `ESP.getCycleCount()` y/o `ESP.getChipId()`), manteniendo el tipo `uint16_t` y la garantía `id != 0`. **No rediseñar el protocolo** ni cambiar la ventana de dedup. Dejar claro en el comentario que esto reduce (no elimina) la probabilidad de colisión.
      Files: `lib/IoTProtocol/IoTStorage.cpp` (`_volatileBootId`), opcionalmente comentario de contexto en `IoTStorage.h` cerca de `isBootIdPersistent()`.
      Verify: `pio run -e emisor_unificado` y `pio run -e receptor_central_unificado` → ambos `SUCCESS`. Si se usó `ESP.getChipId()`/`ESP.getCycleCount()`, confirmar que existen en el core (son estándar del core ESP8266) y que el host stub de `tests/storage_host` sigue compilando (si no, acotar las fuentes a `micros()`+`analogRead` para no romper el check host).

- [ ] 8. **(B10) `IoTConfigHandler::handleConfig` debe responder `INVALID_VALUE` cuando nada se aplica por rango.**
      Qué hacer: en `lib/IoTProtocol/IoTConfigHandler.cpp`, `handleConfig()`. Hoy el `result` solo pasa a `FAIL` si falla `saveConfig()`; si todos los TLV llegan fuera de rango, `changesApplied==0` pero el RESPONSE sigue siendo `OK`. Distinguir el caso "llegaron TLV de config pero ninguno fue válido por rango": si el paquete trae al menos un TLV de configuración (CFG_HEARTBEAT_MS / CFG_ANTIREBOTE_MS / CFG_DEVICE_NAME / CFG_AUTH_ENABLE) y `changesApplied == 0`, responder `ResultCode::INVALID_VALUE`. Si el paquete no trae ningún TLV de config aplicable (p. ej. solo `CFG_REBOOT`/`CFG_RESET_STATS`/`CFG_VERSION`), mantener el comportamiento actual. Para detectarlo, llevar un contador `cfgTlvsPresentes` que se incremente cuando el TLV existe (independiente de si pasó el rango) para heartbeat/antirebote/name/auth. Ajustar: `result = (cfgTlvsPresentes > 0 && changesApplied == 0) ? INVALID_VALUE : OK` (y seguir pisando con `FAIL` si `saveConfig` falla).
      Files: `lib/IoTProtocol/IoTConfigHandler.cpp` (`handleConfig`).
      Verify: `pio run -e emisor_unificado` y `pio run -e receptor_central_unificado` → ambos `SUCCESS`. Revisar que `_sendResponse` recibe el nuevo `result`.

- [ ] 9. **(B12) `detectBootReason()` debe usar `ESP.getResetInfoPtr()->reason` como fuente primaria.**
      Qué hacer: en `emisor_pir_unificado/src/main.cpp`, `detectBootReason()`. Usar el enum numérico estable `ESP.getResetInfoPtr()->reason` (valores del core: `REASON_DEFAULT_RST`/`REASON_WDT_RST`/`REASON_EXCEPTION_RST`/`REASON_SOFT_WDT_RST`/`REASON_SOFT_RESTART`/`REASON_DEEP_SLEEP_AWAKE`/`REASON_EXT_SYS_RST`) y mapearlo a `BootReason`. Mantener el `strstr(ESP.getResetReason())` existente **solo como fallback** cuando el enum no determine una causa clara (p. ej. mapear a `UNKNOWN` → intentar el texto). Comentario en español explicando la jerarquía.
      Decisión de diseño: el enum numérico es estable entre versiones del core; el texto queda como red de seguridad. Rationale: elimina la degradación silenciosa a UNKNOWN por cambios de formato del texto.
      Files: `emisor_pir_unificado/src/main.cpp` (`detectBootReason`).
      Verify: `pio run -e emisor_unificado -d emisor_pir_unificado` → `SUCCESS` (confirma que los símbolos `REASON_*` y `getResetInfoPtr()` existen en el core instalado para este env).

- [ ] 10. **(B15) Consolidar el puerto UDP en una sola fuente y alinear comentarios de versión a V4.3.**
      Qué hacer, dos partes:
      (a) **Puerto UDP:** definir el valor del puerto una sola vez y referenciarlo desde ambos sitios. Opción elegida: en `emisor_pir_unificado/src/device_config.cpp` dejar `UDP_PORT = 4210` como la fuente para el emisor, y en `network_config.h` hacer que `PUERTO_UDP` referencie el mismo literal mediante un `#define` compartido o un comentario que marque explícitamente que `network_config.h` solo lo usa `legacy/`. Como `UDP_PORT` es un `const` en `.cpp` (no usable en el inicializador de otro header), la consolidación concreta es: añadir un único `#define IOT_UDP_PORT_DEFAULT 4210` (p. ej. en `network_config.h`/`network_config.h.template`) y usarlo tanto para `PUERTO_UDP` (`const unsigned int PUERTO_UDP = IOT_UDP_PORT_DEFAULT;`) como de referencia documentada para `UDP_PORT` en `device_config.cpp` (comentario que apunte al define). Objetivo: una sola fuente de verdad (4210) y sin divergencia silenciosa. No romper `legacy/emisor_pir` (sigue leyendo `PUERTO_UDP`).
      (b) **Versión:** en `emisor_pir_unificado/include/device_config.h` cambiar el comentario de cabecera de "V4.1" a "V4.3" para alinearlo con `main.cpp` (V4.3) y `FW_VERSION="4.3.0"`.
      Files: `emisor_pir_unificado/include/device_config.h` (comentario de versión), `emisor_pir_unificado/src/device_config.cpp` (comentario/define del puerto), `c:\Users\Alex\Desktop\1-Github\wifi_PIR\network_config.h` (define compartido) — ver también B-GIT para el `.template`.
      Verify: `pio run -e emisor_unificado -d emisor_pir_unificado` → `SUCCESS`. (El receptor no usa `PUERTO_UDP`, pero correr `pio run -e receptor_central_unificado` si se tocó algo incluido por él.)

- [ ] 11. **(B-GIT) Destrackear `network_config.h`, crear `network_config.h.template`, verificar índice. SIN commit.**
      Qué hacer (ejecutar desde `c:\Users\Alex\Desktop\1-Github\wifi_PIR`):
      1. `git rm --cached network_config.h` — destrackea sin borrar el archivo local (queda en el working tree, ahora ignorado por `.gitignore` que ya lista `network_config.h`).
      2. Crear `c:\Users\Alex\Desktop\1-Github\wifi_PIR\network_config.h.template` **versionado**, con el MISMO contenido efectivo que `network_config.h` actual (gateway `192.168.0.1`, subnet `255.255.255.0`, `PUERTO_UDP 4210` — o el `#define IOT_UDP_PORT_DEFAULT 4210` consolidado en B15), pero como plantilla: añadir arriba un comentario en español explicando que hay que copiarlo a `network_config.h` (que está en `.gitignore`) antes de compilar.
      3. Verificar: `git ls-files network_config.h` → **no debe devolver nada**; `git ls-files network_config.h.template` → debe aparecer; `git ls-files secrets.h` → **sigue sin aparecer** (NO hacer `git rm` de `secrets.h`).
      Restricciones: NO reescribir historial, NO `git commit`, NO `git push`, NO `git rm` de `secrets.h`.
      Files: crea `c:\Users\Alex\Desktop\1-Github\wifi_PIR\network_config.h.template`; `network_config.h` permanece local (destrackeado).
      Verify: `git status --short` muestra `network_config.h` como `D` en index/`??` según corresponda y `network_config.h.template` como nuevo (`A`/`??`), sin tocar `secrets.h`. `git ls-files | Select-String "secrets|network_config"` → debe listar `secrets.h.template` y `network_config.h.template`, pero NO `network_config.h` ni `secrets.h`.

- [ ] 12. **Verificación final integral.**
      Qué hacer: compilar ambos firmwares completos para confirmar que todos los cambios (incluida la lib compartida) enlazan en emisor y receptor, y confirmar el estado de git.
      Files: ninguno (solo verificación).
      Verify:
      - `pio run -e emisor_unificado -d emisor_pir_unificado` → `SUCCESS`.
      - `pio run -e receptor_central_unificado -d receptor_central_unificado` → `SUCCESS`.
      - (Si `g++` disponible) los tres checks host → `OK: host compile check wifi_PIR`, `OK: storage hardening check wifi_PIR` (y `OK: HMAC vector check wifi_PIR` si hay OpenSSL). Limpiar binarios temporales generados.
      - `git status --short` y `git ls-files | Select-String "secrets|network_config"` coherentes con B-GIT (sin `secrets.h` ni `network_config.h` versionados; con ambos `.template`). Sin commits ni push.

---

## Notas / supuestos

- **Único secrets.h / network_config.h compartidos:** el enunciado habla de "los secrets.h reales del emisor Y del receptor". En este repo esos no son dos archivos: es un único `secrets.h` raíz incluido por ambos vía `-I../`. Editarlo una vez satisface el requisito de "mismos 16 bytes en ambos" por construcción. Igual para `network_config.h`. Si en el futuro se separan por subproyecto, habría que replicar la clave en cada uno.
- **g++ ausente:** la máquina tiene `pio` pero no `g++`, así que los checks host unitarios (ideales para B5/B7/B9) pueden no ejecutarse. La verificación primaria y suficiente para "no romper" es `pio run` de ambos env (compila la lib real + firmware). El coder debe registrar si no pudo correr un check host por falta de `g++`, en vez de afirmar que pasó.
- **B13/B14 no están en el alcance de arreglo de esta tarea** (son POSIBLE y de diseño/hardware); el análisis los deja como documentación. No se incluyen items para ellos salvo que el enunciado lo pida (no lo pide).
- **Reversibilidad:** todos los cambios son locales y de bajo riesgo salvo B-GIT (`git rm --cached`), que es reversible (`git add network_config.h` restauraría el tracking) y no borra el archivo local. No hay operaciones destructivas de datos ni de historial.
