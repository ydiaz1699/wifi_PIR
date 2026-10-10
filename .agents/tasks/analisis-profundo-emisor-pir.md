# Análisis Profundo de Código — `emisor_pir_unificado` (ESP8266 / IoTProtocol V4.3)

> Revisión **SOLO LECTURA**. No se modificó ningún archivo. Metodología: skill "Análisis Profundo de Código" (3 capas). Capa alcanzada: **CAPA 2 completa** (estático + dinámico leyendo el source de la librería y calculando tiempos reales) + **CAPA 3 en modo recomendación** (tests descritos, no ejecutados por no haber hardware).

---

## 1. Resumen ejecutivo

El emisor **no enlaza tal como está**: `loop()` llama a `manejarWiFi()`, función que **no existe** en este `env` (CONFIRMADO). Es el bug que bloquea todo lo demás: sin él no hay binario, y además arrastra que **no haya ninguna reconexión WiFi ni reinicialización de OTA**.

Más allá del error de enlace, la Capa 2 (lectura del source de `IoTProtocol`) revela que el **diseño del core es sólido** —firma HMAC centralizada sin doble firma, verificación antes de registry/ACK/dedup, cola con prioridades y buffering sin WiFi— pero que **la aplicación (main.cpp/ota.cpp) subutiliza o contradice ese diseño** en varios puntos, y que hay **bugs nuevos de timing y estado oculto** que una revisión superficial no vería:

- **Jitter de heartbeat con resta sin signo** → el heartbeat puede dispararse en ráfaga o retrasarse mucho (CONFIRMADO, bug nuevo en `IoTNode::_sendHeartbeat`).
- **Dedup liga sesión a BOOT_ID**: si el BOOT_ID queda **degradado/volátil** (fallo de LittleFS), tras un reinicio la central puede **descartar como replay** los primeros eventos del emisor (CONFIRMADO como mecanismo; impacto PROBABLE).
- **Un solo reliable en vuelo + sin manejo de reconexión**: con WiFi caído, un MOTION ocupa el canal reliable y **bloquea el envío** de los eventos siguientes; la cola es de 8 y los EVENT son NORMAL, no descartables, así que se **saturan y se empiezan a perder** por `queueDrops` (CONFIRMADO por lógica).
- **Validación de rango de config**: `IoTConfigHandler` **sí valida** `antireboteMs` (100–10000) y `heartbeatIntervalMs` (5000–600000) → **REFUTA** el "possible" de la revisión previa para la ruta CONFIG. Pero `IoTStorage::loadConfig()` **no revalida rangos** al leer de LittleFS → una config corrupta/manipulada en disco con `antirebote=0` sí entra (PROBABLE).

Respuesta corta a "¿está bien o no?": **el firmware del emisor no está bien todavía** — no compila por `manejarWiFi()` y, una vez resuelto eso, quedan defectos de timing/estado que afectan la fiabilidad de las detecciones. El core `IoTProtocol` está mejor diseñado que la capa de aplicación que lo usa.

---

## 2. Capa 1 — Análisis estático

### 2.1 Mapa de componentes

| Componente | Archivo | Rol |
|---|---|---|
| Aplicación emisor | `emisor_pir_unificado/src/main.cpp` | setup/loop, lectura PIR+timbre, callbacks |
| Config de hardware | `src/device_config.cpp`, `include/device_config.h` | pines, IPs, timings locales |
| OTA | `src/ota.cpp`, `include/ota.h` | ArduinoOTA |
| Logger | `include/logger.h` | macros `Serial.printf` |
| Core nodo | `lib/IoTProtocol/IoTNode.{h,cpp}` | cola FIFO, reliable, ACK, dedup, heartbeat, registry |
| Protocolo wire | `lib/IoTProtocol/IoTProtocol.{h,cpp}` | serialización, CRC16, TLV |
| Auth | `lib/IoTProtocol/IoTAuth.{h,cpp}` | HMAC-SHA256 truncado a 4 B |
| Config remota | `lib/IoTProtocol/IoTConfigHandler.{h,cpp}` | aplica CONFIG, valida rangos, RESPONSE |
| Persistencia | `lib/IoTProtocol/IoTStorage.{h,cpp}` | LittleFS: boot counter, config, auth key |
| Perfil de dominio | `lib/AlarmProfile/AlarmProfile.h` | enums EventCode/DeviceType/StateTag |
| Config compartida | `network_config.h`, `secrets.h(.template)` | gateway/subnet, credenciales, AUTH_KEY |

### 2.2 Flujo de datos (detección → central)

```
PIR flanco subida (loop)
  → antirrebote local (storage.config().antireboteMs)
  → node.sendEvent(MOTION)
      → arma IoTPacket EVENT (ACK_REQUIRED|RELIABLE), seq=getNextSeq(), bootId
      → _prepareOutgoing(): si auth → signPacket() añade TLV AUTH_HMAC4 + flag
      → enqueue() en FIFO (prio NORMAL)
  → node.loop() en siguiente iteración
      → _processQueue(): si no hay reliable activo, promueve a canal reliable
      → _processReliable(): transmite por UDP, espera ACK con backoff
central → ACK (seq coincidente) → _handleAck() libera el canal
```

### 2.3 Suposiciones falsas / nombres engañosos detectados (Capa 1)

1. **`manejarWiFi()` — llamada fantasma (CONFIRMADO, CRÍTICO).** `main.cpp:loop()` la invoca; búsqueda en todo el repo (`grep manejarWiFi`) la encuentra **solo** en `receptor_central_unificado/src/main.cpp`, `legacy/emisor_pir`, `legacy/receptor_bocina` y `docs/ARCHITECTURE.md` — **nunca** en `emisor_pir_unificado`. Este `env` compila solo `src/*.cpp` de este proyecto + `lib/`. Resultado: `undefined reference to manejarWiFi()`. **No hay binario.**

2. **El log miente sobre OTA (CONFIRMADO).** `setup()` imprime "WiFi no disponible: OTA se inicializará al reconectar", pero no existe código que reintente `setupOTA()`. `handleOTA()` es no-op mientras `otaInicializada==false`. Comentario que contradice al código.

3. **`actualizacionEnProgreso` no vuelve a `false` en `onEnd` (CONFIRMADO).** `ota.cpp`: `onStart` lo pone `true`, `onError` lo pone `false`, pero `onEnd` **no**. Si `onEnd` se dispara sin reinicio, `otaEnProgreso()` queda `true` para siempre y `loop()` hace `return` eterno → sensores congelados.

4. **`detectBootReason()` parsea texto frágil (CONFIRMADO).** `std::strstr` sobre `ESP.getResetReason()`; el texto varía por versión de core. Degradación silenciosa a `UNKNOWN`. Existe la alternativa estable `ESP.getResetInfoPtr()->reason`.

5. **Dos fuentes de verdad para el puerto UDP (CONFIRMADO, menor).** `device_config.cpp` → `UDP_PORT=4210`; `network_config.h` → `PUERTO_UDP=4210` (este último no se usa en el emisor). Riesgo de divergencia futura.

6. **Comentario de versión desincronizado (CONFIRMADO, menor).** `device_config.h` dice "V4.1"; `main.cpp` dice "V4.3"; `FW_VERSION="4.3.0"`.

---

## 3. Capa 2 — Análisis dinámico (source de `IoTProtocol`)

### 3.1 Protocolo de decisión: qué parece vs qué hace cada función usada por el emisor

| Función (lib) | El nombre sugiere… | Qué hace realmente (evidencia) | Bloqueante / estado oculto |
|---|---|---|---|
| `IoTNode::sendEvent()` | "enviar evento" | **Encola**, no envía. Arma EVENT con `ACK_REQUIRED|RELIABLE`, añade `EVENT_TYPE/EVENT_VALUE/RSSI`; si falta cualquier TLV **aborta** (`return false`) y el evento se pierde silenciosamente. Firma vía `_prepareOutgoing`. | No bloqueante. Muta `_seq`, `_queue`, `_stats`. **Oculto:** si el payload no cabe, no encola. |
| `node.loop()` | "atender el nodo" | `_processIncoming` (drena hasta 8 datagramas), `_processReliable`, `_processQueue`, heartbeat, y `_updateDeviceStates` cada 10 s. | No bloqueante (acotado por `IOT_MAX_RX_PER_LOOP`). Estado: cola, canal reliable, registry, RTT. |
| `node.sendDirect()` | "enviar directo" | UDP sin cola/ACK. **Retorna sin hacer nada si `WiFi.status()!=WL_CONNECTED`** → ACKs, heartbeats y STATE_REPORT se **tiran** sin WiFi. | No bloqueante. Firma antes de serializar. |
| `node.enableHeartbeat()` | "activar heartbeat" | Fija intervalo y pone `_lastHb=millis()`. | — |
| `node.sendHello()` | "enviar HELLO" | Encola HELLO reliable; si faltan TLV **críticos** (type/name/bootId) **descarta**; FW_VERSION/BOOT_REASON son opcionales. | Muta `_seq`, cola. |
| `IoTAuth::signPacket()` | "firmar" | **Idempotente**: si ya hay un `AUTH_HMAC4` y es el último TLV, recalcula sobre él sin duplicar; si hay >1 auth TLV o uno no final, **falla**. | Muta payload y flags. HMAC sobre header(11B)+payload-sin-auth. |
| `IoTAuth::verifyPacket()` | "verificar" | Exige que el `AUTH_HMAC4` sea **el último** TLV y único; compara en **tiempo constante**. Sin flag + sin auth TLV → pasa solo si `!_required`. | Sin estado mutable. |
| `IoTConfigHandler::handleConfig()` | "aplicar config" | Extrae TLV, **valida rangos**, persiste si hubo cambios, envía RESPONSE, invoca callback, y **al final** puede `delay(100)+ESP.restart()`. | **Bloqueante en reboot** (`delay(100)` + reset). |
| `IoTStorage::getBootId()` | "obtener boot id" | **Incrementa y persiste** el contador; si LittleFS falla devuelve un **ID volátil** (`micros()^analogRead`) y marca `isBootIdPersistent()=false`. | Side effect: escribe en FS. Llamar **una sola vez**. |

### 3.2 Pregunta 1 — Encolado sin WiFi, reenvío al reconectar, límites de cola

**CONFIRMADO (con matices importantes):**

- La **cola FIFO** tiene capacidad `IOT_QUEUE_SIZE = 8` (`IoTNode.h`). `sendEvent/enqueue` **sí encolan sin WiFi** (no comprueban estado de red al encolar). Correcto.
- El envío real sí comprueba WiFi: `_processQueue` solo transmite no-reliable si `WL_CONNECTED`; `_processReliable` difiere (`nextRetryAt = ahora + 1000`) si no hay WiFi; `sendDirect` retorna si no hay WiFi.
- **Problema de interacción (CONFIRMADO):** solo **un paquete reliable en vuelo** (`ReliableChannel`, 1 slot). Los EVENT del PIR/timbre son `ACK_REQUIRED|RELIABLE`. Mientras el primer MOTION no reciba ACK, `_processReliable` lo **reintenta hasta `IOT_MAX_RETRIES=5`** y `_processQueue` **no promueve** ningún otro evento (`if (_reliable.active) return;`). Con WiFi caído, cada evento nuevo se **acumula en la cola de 8**.
- **Política de overflow (CONFIRMADO):** default `DROP_OLDEST_BG`. Los EVENT son **NORMAL**, no BACKGROUND. Cuando la cola se llena de EVENT NORMAL, `enqueue` busca una víctima BACKGROUND, **no la encuentra**, hace `_stats.queueDrops++` y **devuelve false** → **el evento se pierde** y el `LOG_INFO("MOTION encolado q=%d")` del emisor miente (reporta encolado aunque la función devolvió false; `sendEvent` ignora el retorno).
- **Overflow silencioso de SEQ en el emisor (sutil):** `sendEvent` llama a `getNextSeq()` **aunque luego `enqueue` falle**. Es decir, un evento perdido **igual consume un número de secuencia**. Esto por sí mismo no rompe el dedup (la ventana admite huecos), pero sesga `TX_COUNT`/estadísticas y complica la correlación.

**Respuesta:** sí hay buffering sin WiFi, pero el **límite real efectivo es 1 reliable + 7 en cola**, y con la central inalcanzable **los eventos NORMAL se descartan** una vez llena la cola (no hay reintento infinito del buffer completo, solo del único paquete en vuelo). **Sí se pierden eventos** en caída prolongada.

### 3.3 Pregunta 2 — Timings reales (fórmulas desde el source)

**Antirrebote PIR (CONFIRMADO):** `main.cpp:loop()` usa `storage.config().antireboteMs`. Default = `200 ms` (`IoTStorage::_setDefaults`). Condición: `if (ahora - ultimaDeteccionPIR > antirebotePIR)`. Antirrebote efectivo PIR = **200 ms** (hasta que una CONFIG lo cambie en 100–10000). Nota: `device_config.cpp` define `ANTIREBOTE_PIR_MS=200` pero **no se usa** en el loop (se usa el de storage); otro nombre engañoso.

**Antirrebote timbre (CONFIRMADO):** `ANTIREBOTE_TIMBRE_MS = 800 ms` (constante local, `device_config.cpp`). No viene de config remota — correcto y documentado en el código.

**Heartbeat (CONFIRMADO + BUG):** intervalo default `60000 ms`. `IoTNode::_sendHeartbeat`:
```
if (ahora - _lastHb < _hbInterval) return;       // dispara a los 60 s
long jitter = random(-5000, 5000);               // −5000..+4999 ms
_lastHb = ahora + jitter;                         // <-- BUG
```
Fórmula del próximo disparo: `siguiente = _lastHb + _hbInterval = (ahora + jitter) + 60000`.
- Con `jitter` positivo (p. ej. +4000): siguiente ≈ ahora+64000 → período ~**64 s**. OK.
- Con `jitter` negativo (p. ej. −5000): `_lastHb = ahora−5000`. En el siguiente loop, `ahora' − _lastHb = ahora' − (ahora−5000) ≈ 5000 ≥ 0` pero **ya parte con 5 s "adelantados"**, de modo que el período efectivo baja a ~**55 s**. El rango de período real es **~55–64 s**, no 60±5 centrado. Más grave: como `_lastHb` guarda `ahora+jitter` en vez de `ahora`, el jitter **no se re-centra**; el sesgo se **acumula** levemente arranque a arranque. **Intención vs realidad:** el comentario dice "±5 s para distribuir tráfico", pero la implementación sesga el período y, si `millis()` estuviera cerca de 0 al inicio con jitter negativo, `ahora+jitter` podría **envolver** (unsigned underflow de `unsigned long`), dando un `_lastHb` enorme → heartbeat **suspendido ~49 días**. Es un caso borde real tras un arranque muy temprano. **Confianza: CONFIRMADO (lógica), impacto POSIBLE para el underflow / PROBABLE para el sesgo de período.**

**Ventana de dedupe (CONFIRMADO):** `IOT_DEDUP_WINDOW = 8`. El dedup vive en el **receptor** (central), no en el emisor; el emisor nunca se "traga" sus propias detecciones por dedup. Pero: la central liga la ventana a `(srcId, bootId)`. Si el emisor **reinicia con BOOT_ID repetido** (modo degradado, ver 3.6) y la central aún tiene la sesión anterior con `seqHighest` alto, los primeros SEQ del emisor reiniciado (que vuelven a 1) caen en `age >= IOT_DEDUP_WINDOW` → **descartados como replay** (`_isDuplicate` retorna true, `_stats.replays++`). **Una detección legítima puede perderse** tras un reinicio degradado. Confianza: mecanismo CONFIRMADO; que ocurra depende de colisión de BOOT_ID → PROBABLE en modo degradado.

**Backoff de reliable (CONFIRMADO):** `_calcBackoff(attempt)` = `BASE·1.5^attempt`, saturado a `MAX`. Con `BASE=300`, `MAX=2000`:
- attempt 0 → 300 ms
- 1 → 450 ms
- 2 → 675 ms
- 3 → ~1012 ms
- 4 → ~1518 ms (el 5.º intento usaría ~2000, pero `maxAttempts=5` corta antes).
Tiempo total aproximado para declarar `ackTimeout` ≈ 300+450+675+1012+1518 ≈ **~3.95 s** desde el primer envío (más el tiempo de espera de cada ACK). Durante esos ~4 s el canal reliable está **ocupado** y ningún otro evento se transmite.

**Espera WiFi bloqueante en setup (CONFIRMADO):** `while(status!=CONNECTED && millis()-t0<10000){ wdtFeed(); delay(100); }` → hasta **10 s** de arranque bloqueado (100 iteraciones de 100 ms). No dispara watchdog (se alimenta), pero retrasa `node.begin()`, heartbeat y lectura de sensores. Un flanco PIR durante esos 10 s **se pierde** (el loop aún no corre).

### 3.4 Pregunta 3 — ¿`IoTConfigHandler` valida rangos? ¿Qué pasa con 0 o valores enormes?

**REFUTA el "possible" de la revisión previa para la ruta CONFIG (CONFIRMADO):**
- `CFG_HEARTBEAT_MS`: acepta solo `5000 ≤ v ≤ 600000`. Fuera de rango → **no aplica** el cambio (silencioso).
- `CFG_ANTIREBOTE_MS`: acepta solo `100 ≤ v ≤ 10000`. `0` o valores enormes → **ignorados**, se conserva el valor previo.

**Pero (matiz importante, PROBABLE):**
1. **Silencio engañoso:** si todos los valores llegan fuera de rango, `changesApplied` puede quedar en 0 pero el **RESPONSE sigue siendo `OK`** (el `result` solo es `FAIL` si falló `saveConfig`). La central no se entera de que su cambio fue rechazado por rango. Debería devolver `INVALID_VALUE`.
2. **`IoTStorage::loadConfig()` NO revalida rangos (CONFIRMADO):** al leer de LittleFS, `antirebote_ms`/`heartbeat_ms` se parsean con `val.toInt()` y se aceptan si el **CRC16 del archivo** cuadra. No hay chequeo de rango. Un archivo `/iot/config.json` corrupto pero con checksum coherente (o manipulado en flash) con `antirebote_ms=0` **sí entra** → antirrebote PIR efectivo = 0 → cada flanco alto del PIR genera un EVENT, **saturando la cola** (ver 3.2). Confianza: mecanismo CONFIRMADO; depende de corrupción/manipulación → PROBABLE.
3. **`heartbeat_ms=0` por esa vía** → en `_sendHeartbeat`, `ahora-_lastHb < 0` nunca se cumple (unsigned), dispara **cada loop** → tormenta de heartbeats BACKGROUND.

### 3.5 Pregunta 4 — Orden HMAC, doble firma, replay/nonce

**Orden y doble firma (CONFIRMADO correcto):**
- La firma se centraliza en `IoTNode` vía `_prepareOutgoing` → `signPacket`. El emisor **no** firma manualmente (el comentario en `sendStateReport` lo dice explícitamente). `signPacket` es **idempotente**: detecta un `AUTH_HMAC4` ya presente y recalcula sobre él sin duplicar; si hubiera >1 auth TLV o uno no final, falla. **No hay doble firma.** El HMAC cubre `header(11B) sin el flag AUTHENTICATED + payload sin el propio TLV auth`, y en verificación se exige que el TLV auth sea **el último y único**. Diseño robusto. CONFIRMADO.
- **Verificación antes de efectos (CONFIRMADO):** `_processIncoming` llama a `_verifyIncoming` **antes** de registry, ACK, dedup y callback. Un rechazo no produce ningún efecto. Correcto.

**Replay (CONFIRMADO: cubierto por SEQ+BOOT_ID+ventana, NO por HMAC):**
- No hay nonce ni contador criptográfico; el anti-replay es la **ventana de dedup** (`_isDuplicate`) ligada a `(src, bootId, seq)` con bitmap de 8. El propio `IoTAuth.h` lo documenta ("NO previene replay; cubierto por SEQ+BOOT_ID+dedup window").
- **Debilidad real (PROBABLE):** HMAC truncado a **4 bytes** = 2^32. Para LAN doméstica es defendible (lo dice el header), pero no resiste un atacante dedicado. Y, sobre todo, la **clave compartida es la del template** (ver 4.x) → el HMAC **no aporta seguridad** hoy.
- **Caso de bootstrap del ACK (sutil, CONFIRMADO como diseño):** `_handleAck` permite que, en bootstrap (sin BOOT_ID remoto conocido), un ACK **establezca** la sesión del remoto. Esto es intencional y está acotado (reliable activo + SRC/SEQ coincidentes + ya pasó auth), pero amplía ligeramente la superficie: con auth **deshabilitado** (default de fábrica, `authEnabled=false`), un ACK falsificado con SEQ adivinado podría confirmar un reliable. Confianza del riesgo: POSIBLE (requiere auth off + adivinar SEQ/puerto).

### 3.6 Pregunta 5 — Bloqueos en el camino crítico (perder flanco PIR / watchdog)

**CONFIRMADO — bloqueos existentes:**
1. **`setup()`**: `delay(100)` inicial + bucle de espera WiFi de **hasta 10 s** (`delay(100)`×100). Antes de que `loop()` corra. Un flanco PIR aquí **se pierde**.
2. **`IoTConfigHandler::handleConfig`**: en reboot, `delay(100)` + `ESP.restart()`. Es terminal (reinicia), aceptable.
3. **Camino crítico del loop (riesgo de perder flanco, CONFIRMADO):** el PIR se lee por **polling de nivel** (`digitalRead` + flanco por software `pirActual && !pirAnterior`), **no por interrupción**. Si una iteración de `loop()` tarda más que el pulso del PIR, se pierde el flanco. ¿Qué puede alargar el loop?
   - `node.loop()` drena hasta **8 datagramas UDP** y, por cada uno, puede calcular **HMAC-SHA256** (verify) + enviar auto-ACK (otro HMAC en sign). En ESP8266 a 80 MHz, 8×(verify+sign) de SHA256 es del orden de **unidades de ms**, no decenas; no debería tragarse un pulso PIR típico (los HC-SR501 mantienen el pulso cientos de ms–segundos). Riesgo bajo con hardware PIR estándar, pero **real para un timbre de pulsación muy corta** si coincide con una ráfaga UDP. Confianza: POSIBLE.
   - **Watchdog:** `ESP.wdtEnable(8000)` (8 s) y `ESP.wdtFeed()` al inicio del loop y dentro del bucle de setup. Ningún camino identificado excede 8 s **salvo** el escenario de underflow de `_lastHb` (no cuelga el loop, solo suspende heartbeat) y el `otaEnProgreso()` pegado en `true` (ver 2.3-#3), que **sí** congela los sensores indefinidamente pero **sigue alimentando el watchdog** (el `return` es después de `wdtFeed()`), así que **no reinicia** — queda colgado "vivo". Confianza: CONFIRMADO el mecanismo, PROBABLE que ocurra (depende de `onEnd` sin reset).

---

## 4. Bugs por capa (tabla consolidada)

| # | Bug | Capa | Archivo / símbolo | Severidad | Confianza |
|---|---|---|---|---|---|
| B1 | `manejarWiFi()` llamada pero no definida → no enlaza | 1 | `main.cpp:loop()` | Crítico | **CONFIRMADO** |
| B2 | Sin reconexión WiFi; OTA nunca se reinicializa si WiFi tardó >10 s | 1/2 | `main.cpp:setup/loop`, `ota.cpp` | Alto | **CONFIRMADO** |
| B3 | `actualizacionEnProgreso` no baja en `onEnd` → posible congelado | 1 | `ota.cpp:onEnd` | Alto | **CONFIRMADO** |
| B4 | Espera WiFi bloqueante hasta 10 s en setup → se pierden flancos tempranos | 2 | `main.cpp:setup` | Alto | **CONFIRMADO** |
| B5 | Jitter de heartbeat con `ahora+jitter` → sesgo de período y posible underflow | 2 | `IoTNode::_sendHeartbeat` | Medio | **CONFIRMADO** (impacto underflow POSIBLE) |
| B6 | Clave HMAC = template versionado → auth sin valor real | 1/2 | `secrets.h` vs `secrets.h.template` | Crítico (seguridad) | **CONFIRMADO** |
| B7 | Cola NORMAL se satura y descarta EVENT con WiFi caído; `sendEvent` ignora el `false` | 2 | `IoTNode::enqueue/sendEvent`, `main.cpp` | Alto | **CONFIRMADO** |
| B8 | `getNextSeq()` consumido aunque `enqueue` falle → SEQ "fantasma" | 2 | `IoTNode::sendEvent` | Bajo | **CONFIRMADO** |
| B9 | `loadConfig()` no revalida rangos de disco → `antirebote=0`/`hb=0` entran | 2 | `IoTStorage::loadConfig` | Medio | **CONFIRMADO** (explotación PROBABLE) |
| B10 | CONFIG fuera de rango responde `OK` igual (no `INVALID_VALUE`) | 2 | `IoTConfigHandler::handleConfig` | Bajo | **CONFIRMADO** |
| B11 | Reinicio con BOOT_ID degradado → central descarta primeros eventos como replay | 2 | `IoTStorage::_volatileBootId` + `IoTNode::_isDuplicate` | Medio | **PROBABLE** |
| B12 | `detectBootReason()` por `strstr` frágil | 1 | `main.cpp:detectBootReason` | Bajo | **CONFIRMADO** |
| B13 | HMAC 4 B + bootstrap-ACK: superficie de replay con auth OFF | 2 | `IoTAuth`, `IoTNode::_handleAck` | Bajo | **POSIBLE** |
| B14 | PIR por polling (no IRQ): ráfaga UDP + HMAC puede perder un pulso corto | 2/3 | `main.cpp:loop`, `node.loop` | Bajo | **POSIBLE** |
| B15 | Puerto UDP y versión duplicados/desincronizados | 1 | `device_config.*`, `network_config.h` | Bajo | **CONFIRMADO** |

> La revisión previa queda así: HIGH-1/HIGH-2/CRÍTICO-1..3/MED (onEnd, detectBootReason) **CONFIRMADOS**; el "possible" de rangos de `antireboteMs` **REFUTADO para la ruta CONFIG** (sí valida) y **re-confirmado en la ruta `loadConfig()`** (no valida). Bugs **nuevos** aportados por Capa 2: B5, B7, B8, B9, B10, B11, B13, B14.

---

## 5. Cadenas causales de los bugs principales

**Cadena A — "no arranca":** `manejarWiFi()` indefinida (B1) → error de enlace → sin binario. Aunque se defina copiándola del receptor, si no llama a `setupOTA()` al reconectar (B2) y no se vuelve no bloqueante el setup (B4), el emisor queda sin OTA tras caídas y pierde eventos al arranque.

**Cadena B — "pérdida de eventos en caída de red":** WiFi cae → `_processReliable` difiere, el MOTION ocupa el único canal reliable (B7) → eventos nuevos se acumulan en cola de 8 → al llenarse, `enqueue` devuelve `false` para NORMAL (no hay víctima BACKGROUND) → `sendEvent` **ignora** el `false` y loguea "encolado" (B7) → eventos perdidos + SEQ fantasma (B8). Al volver WiFi, si el reinicio ocurrió con BOOT_ID degradado (B11), los primeros eventos se descartan como replay en la central.

**Cadena C — "congelado silencioso":** OTA termina y `onEnd` no baja el flag (B3) → `otaEnProgreso()` queda `true` → `loop()` hace `return` antes de leer sensores, pero sigue alimentando el watchdog → equipo "vivo" pero **sordo**, sin reinicio automático.

**Cadena D — "heartbeat errático":** `_lastHb = ahora + jitter` (B5) → período sesgado 55–64 s; en arranque muy temprano con jitter negativo, underflow de `unsigned long` → heartbeat suspendido ~49 días → la central marca el emisor STALE/OFFLINE pese a estar operativo.

---

## 6. Timing calculado — resumen con fórmulas

| Magnitud | Fórmula (source) | Valor |
|---|---|---|
| Antirrebote PIR efectivo | `storage.config().antireboteMs`, default `_setDefaults`=200 | **200 ms** (rango CONFIG 100–10000) |
| Antirrebote timbre | `ANTIREBOTE_TIMBRE_MS` | **800 ms** |
| Heartbeat período real | `_hbInterval + jitter_acumulado`, `jitter∈[−5000,+4999]` | **~55–64 s** (sesgado; underflow en caso borde) |
| Backoff reliable intento k | `300·1.5^k` sat. a 2000 | 300/450/675/1012/1518 ms |
| Tiempo a `ackTimeout` | Σ backoff, `maxAttempts=5` | **~3.95 s** canal ocupado |
| Espera WiFi en setup | `100 ms × ≤100` | **≤10 s** bloqueante |
| Dedup ventana (central) | `IOT_DEDUP_WINDOW` bitmap 8 | **8 SEQ** por `(src,bootId)` |
| RX acotado por loop | `IOT_MAX_RX_PER_LOOP` | **8 datagramas/iteración** |
| Watchdog | `ESP.wdtEnable(8000)` | **8 s** |
| Capacidad de cola | `IOT_QUEUE_SIZE` | **8** (1 reliable en vuelo + resto) |

---

## 7. Capa 3 — Tests incrementales recomendados (no ejecutados)

Como es revisión estática sin hardware, se describen tests que **aíslan una variable** cada uno:

1. **B1 (enlace):** compilación host del `env` emisor (`tests/host_compile/`) o `pio run -e emisor_unificado`. **Esperado:** hoy falla con `undefined reference to manejarWiFi`. Es la prueba de regresión del fix.
2. **B5 (heartbeat jitter):** test unitario host de `_sendHeartbeat` inyectando `millis()`/`random()` mockeados; iterar con jitter=−5000 y con `ahora` pequeño. **Esperado tras fix:** período ∈ [55,65] s sin underflow; `_lastHb` debe guardar `ahora`, no `ahora+jitter`.
3. **B7 (saturación de cola):** test host: encolar 10 EVENT NORMAL sin WiFi (`WiFi.status` mock = no conectado). **Esperado:** `queueDrops` crece a partir del 9.º y `sendEvent` **debe** propagar el fallo (hoy lo ignora).
4. **B9 (rangos en loadConfig):** generar `/iot/config.json` con `antirebote_ms=0` y checksum válido; `loadConfig()`. **Esperado tras fix:** rechazo o saneo a un mínimo seguro.
5. **B11 (replay por BOOT_ID degradado):** simular dos sesiones del emisor con el mismo bootId volátil y SEQ reiniciado contra `_isDuplicate` de la central. **Esperado:** documentar/mitigar el descarte de los primeros eventos.
6. **B3 (OTA onEnd):** test de la máquina de estados OTA: disparar `onEnd` sin reset y verificar que `otaEnProgreso()` vuelve a `false`.
7. **B14 (flanco PIR vs ráfaga UDP):** banco con pulso de timbre corto (p. ej. 20–50 ms) mientras se inyectan 8 datagramas con auth: medir si se detecta el flanco. Confirma si conviene pasar PIR/timbre a `attachInterrupt`.

---

## 8. Fix propuesto por bug (sin aplicar)

- **B1 / B2 / B4:** implementar en este proyecto `manejarWiFi()` **no bloqueante** (patrón de `receptor_central_unificado/src/main.cpp`): máquina `conectando/conectado` con backoff en `WiFi.begin()`, y al transicionar a `WL_CONNECTED` llamar a `setupOTA()`. Quitar el `while(...10000)` de setup y dejar que `loop()` gestione la conexión; `node.begin()` ya tolera encolar sin WiFi.
- **B3:** en `onEnd`, poner `actualizacionEnProgreso = false` antes del log/reinicio.
- **B5:** usar `_lastHb = ahora;` y aplicar el jitter al **umbral** (p. ej. comparar contra `_hbInterval + jitter`), o `_lastHb = ahora + (jitter>0?jitter:0)`; además `jitter` acotado para que `ahora+jitter` nunca haga underflow (guardas `if (jitter < 0 && (unsigned long)(-jitter) > ahora) jitter = 0;`).
- **B6:** generar `IOT_AUTH_KEY` aleatoria por despliegue (`openssl rand -hex 16`), fuera del repo; dejar en el template un placeholder evidente (no una clave que parezca válida). Rotar la clave actual y tratar los `.bin` como material sensible.
- **B7 / B8:** `sendEvent` debe **propagar** el resultado de `enqueue` y loguear fallo real; considerar prioridad **URGENT** para MOTION/TIMBRE (no descartables) o aumentar `IOT_QUEUE_SIZE`. No consumir `getNextSeq()` hasta confirmar encolado (o reservar/revertir).
- **B9:** en `loadConfig()`, revalidar rangos de `heartbeatIntervalMs` y `antireboteMs` tras parsear (reutilizar los límites de `IoTConfigHandler`), saneando a defaults si están fuera.
- **B10:** si todos los valores llegan fuera de rango, responder `ResultCode::INVALID_VALUE` en lugar de `OK`.
- **B11:** si `!storage.isBootIdPersistent()`, preferir un BOOT_ID volátil con mayor entropía y/o que la central tolere un "resync" explícito; documentar el modo degradado. Priorizar que LittleFS esté formateado en primera instalación.
- **B12:** usar `ESP.getResetInfoPtr()->reason` (enum numérico) como fuente primaria; `strstr` solo como fallback.
- **B13:** mantener auth **habilitado** en producción; documentar que con auth OFF el bootstrap-ACK amplía la superficie.
- **B14:** evaluar `attachInterrupt` para PIR/timbre si se requieren pulsos cortos fiables; como mínimo, mantener el loop corto.
- **B15:** consolidar el puerto UDP en una sola constante y alinear los comentarios de versión a V4.3.

---

## 9. Observaciones positivas (confirmadas en el source)

- Frontera de auth **bien diseñada**: firma centralizada idempotente, verificación antes de registry/ACK/dedup, comparación HMAC en tiempo constante.
- Serialización/CRC16 y TLV con **validación estricta de tamaño** (`getTLV_*` exige longitud exacta; `iot_deserialize` exige `len==expectedLen` y CRC).
- `saveConfig()` **atómico** vía temporal + `rename()`.
- `STATE_REPORT` aborta si el payload queda incompleto (no envía datos parciales).
- Montaje de LittleFS **no destructivo** con reintento y modo degradado explícito (`isBootIdPersistent`).
- Watchdog habilitado y alimentado; sincronización del estado lógico de sensores al arranque para no emitir flanco falso.

---

## 10. Conclusión

El **core `IoTProtocol` V4.3 está mejor que la capa de aplicación**. El emisor **no compila** por `manejarWiFi()` (B1) y, resuelto eso, arrastra defectos de **reconexión/OTA** (B2–B4), **fiabilidad de entrega bajo caída de red** (B7–B8–B11), **timing de heartbeat** (B5) y **seguridad real nula** por la clave de template (B6). Ninguno exige rediseñar el protocolo; son fixes acotados en `main.cpp`, `ota.cpp` y validaciones puntuales en `IoTStorage`/`IoTConfigHandler`. Prioridad sugerida: **B1 → B6 → B2/B4 → B7 → B3 → B5 → B9/B11**.

> Nota de cumplimiento: la igualdad de la clave HMAC con el template se verificó comparando el bloque normalizado de ambos archivos (resultado: idénticos) **sin transcribir el valor de la credencial** a este informe.
