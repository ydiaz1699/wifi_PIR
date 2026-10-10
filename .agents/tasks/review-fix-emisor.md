# Reconexión WiFi no bloqueante, OTA al conectar y endurecimiento del protocolo IoT (B1–B15)

El cambio arregla el conjunto de bugs B1–B15 del emisor PIR unificado y de la librería compartida `lib/IoTProtocol`. En el firmware del emisor convierte la conexión WiFi en una máquina de estados no bloqueante que reinicializa OTA al conectar, elimina la espera bloqueante de `setup()`, corrige el flag de progreso de OTA y usa el enum numérico de reset como fuente primaria. En la librería compartida reordena `sendEvent()` para no quemar SEQ si el evento no se encola y marca los EVENT como URGENT, recentra el jitter del heartbeat sin underflow, revalida rangos de config al leer de LittleFS, devuelve `INVALID_VALUE` cuando ningún TLV de config aplica, y mejora la entropía del BOOT_ID volátil documentando su modo degradado. Fuera del código, consolida el puerto UDP en un único define, rota la clave HMAC en el `secrets.h` real (no versionado) y destrackea `network_config.h` dejando una plantilla versionada.

**Watch for:** nada bloqueante. La compilación de emisor y receptor fue verificada por el coder (evidencia en `verificacion-fix-emisor.md`), no por este review (el enunciado prohíbe re-ejecutar `pio run`). La compatibilidad del protocolo entre emisor y receptor se mantiene: misma clave única compartida, enum de prioridades y flags intactos, serialización sin cambios (confirmed). Observación menor no bloqueante: `setupOTA()` sigue siendo de un solo disparo por el flag `otaInicializada`, así que tras una caída real de WiFi OTA no se reinstala — comportamiento documentado y aceptado por el plan (confirmed).

**Verdict**: APPROVED

## High-level view

La reconexión WiFi ahora es una máquina de estados que corre en `loop()` vía `manejarWiFi()`, con backoff de ~5 s y transición explícita a `WL_CONNECTED` que dispara `setupOTA()`. La espera bloqueante de `setup()` y su log engañoso desaparecieron, de modo que el arranque no se cuelga 10 s cuando no hay red.

El flujo de SEQ del protocolo deja de tener huecos fantasma: `sendEvent()` reserva `_seq+1` pero solo lo confirma si `enqueue()` tiene éxito, y marca los EVENT como `IOT_FLAG_URGENT` para que una detección nunca sea descartada por telemetría. La aplicación captura el bool de `sendEvent()` y reporta pérdida real con `LOG_WARN` en vez de afirmar siempre "encolado".

El endurecimiento de almacenamiento/config comparte ahora una única fuente de rangos (`IOT_CFG_*` en `IoTProtocol.h`): `loadConfig()` sanea a defaults los valores fuera de rango leídos de LittleFS, y `handleConfig()` responde `INVALID_VALUE` cuando llegan TLV de config pero ninguno pasa validación. El heartbeat recentra su período en `_hbInterval ±5 s` guardando `ahora` y aplicando el jitter al umbral, con guarda anti-underflow.

La compatibilidad emisor↔receptor no se rompe: no cambian firmas públicas, el enum `Priority`/flags es el mismo, el formato wire queda intacto, y ambos incluyen el mismo `secrets.h` raíz por construcción. La postura de seguridad mejora: clave HMAC rotada solo en el archivo no versionado, `network_config.h` destrackeado con plantilla versionada, y `secrets.h.template` conserva su placeholder ASCII sin la clave real.

<details>
<summary>Issues (2)</summary>

1. **OTA de un solo disparo tras reconexión** — `setupOTA()` queda bloqueado por `otaInicializada` tras el primer `WL_CONNECTED`; si WiFi cae y vuelve, no se reinstala ArduinoOTA. No bloqueante: documentado y aceptado por el plan. Si en el futuro se requiere OTA tras caídas reales, habría que resetear el flag en la transición a desconectado.
2. **BOOT_ID volátil sigue siendo probabilístico** — en modo degradado (LittleFS no monta) el BOOT_ID puede repetirse y provocar descarte por replay de los primeros eventos. No bloqueante: la mejora de entropía reduce (no elimina) la colisión, y está documentado explícitamente. Acción solo si se decide persistencia garantizada.

</details>

<details>
<summary>Details</summary>

### Reconexión WiFi no bloqueante y arranque sin espera

`manejarWiFi()` en `main.cpp` es una máquina de estados que corre en `loop()`: si no hay conexión arranca `iniciarWiFi()` y, si ya está intentando, reintenta cada 5000 ms. No hay `delay()` ni bucle de espera, por lo que `loop()` sigue atendiendo sensores y OTA mientras WiFi negocia. La espera bloqueante `while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) { ... delay(100); }` de `setup()` se eliminó junto con el log `"OTA se inicializará al reconectar"`, que afirmaba un comportamiento que antes no existía (B1/B2/B4 confirmed).

La transición a `WL_CONNECTED` llama a `setupOTA()`. `setupOTA()` es idempotente por el flag `otaInicializada`, así que OTA se instala una sola vez en el primer `WL_CONNECTED` y las reconexiones posteriores no lo rompen ni lo reinstalan. La contrapartida: tras una caída real de WiFi en la que ArduinoOTA quede en mal estado, `setupOTA()` no lo reinstalará porque `otaInicializada` sigue en `true` (confirmed). El plan reconoce y acepta este límite — no es bloqueante — pero es el punto a revisar si en el futuro se exige OTA resiliente a desconexiones: bastaría bajar `otaInicializada` en la transición a desconectado.

### SEQ sin huecos fantasma y EVENT no descartables

`sendEvent()` ya no consume SEQ antes de saber si el evento se encola. Reserva `uint32_t reservedSeq = _seq + 1` (con el mismo salto de cero que `getNextSeq()`), construye el paquete con esa SEQ, y solo hace `_seq = reservedSeq` cuando `enqueue()` devuelve `true`. Si falta un TLV o la cola rechaza el paquete, `_seq` no avanza, de modo que la central no ve un hueco de secuencia atribuible a un paquete que nunca salió (B7/B8 confirmed). `enqueue()` y `_prepareOutgoing()` no tocan `_seq`, así que no hay doble consumo.

```
reservedSeq = _seq + 1   ──> pkt.seq = reservedSeq
                              enqueue(pkt) ─ true ─> _seq = reservedSeq   (commit)
                                           └ false ─> _seq intacto        (sin fantasma)
```

Los EVENT ahora llevan `IOT_FLAG_ACK_REQUIRED | IOT_FLAG_RELIABLE | IOT_FLAG_URGENT`. En `enqueue()` bajo `DROP_OLDEST_BG`, un paquete URGENT solo puede desalojar víctimas `BACKGROUND`; si la cola está llena de URGENT no encuentra víctima y devuelve `false`. Es el comportamiento correcto: una detección no se descarta para hacer sitio a telemetría, y la pérdida en saturación extrema se reporta en vez de ocultarse. En `loop()`, MOTION y TIMBRE capturan el bool y emiten `LOG_WARN("... NO encolado: cola llena, evento perdido")` cuando falla, en lugar del `LOG_INFO` incondicional anterior (confirmed).

### Jitter de heartbeat recentrado sin underflow

`_sendHeartbeat()` guardaba `_lastHb = ahora + jitter`, lo que sesgaba el período a ~55–64 s y, con jitter negativo cerca del arranque, podía provocar underflow de `unsigned long` y suspender el heartbeat ~49 días. Ahora `_lastHb = ahora` siempre, y el jitter (`_hbJitter`, nuevo miembro privado) se aplica al umbral de disparo: `umbral = (long)_hbInterval + _hbJitter`, acotado a `>= 0`. El período queda centrado en `_hbInterval ±5 s` y la guarda anti-underflow cubre el caso de arranque temprano (B5 confirmed). `enableHeartbeat()` inicializa `_hbJitter = 0` para que el primer período use el intervalo exacto.

### Revalidación de rangos y respuesta INVALID_VALUE

Los límites de config viven ahora en un solo sitio (`IOT_CFG_ANTIREBOTE_*` / `IOT_CFG_HEARTBEAT_*` en `IoTProtocol.h`), consumidos tanto por `IoTConfigHandler` como por `IoTStorage`, evitando que una ruta acepte lo que la otra rechaza. `loadConfig()` revalida `antireboteMs` (100–10000) y `heartbeatIntervalMs` (5000–600000) tras verificar checksum/esquema; un archivo con checksum válido pero `antirebote=0` (satura la cola) o `heartbeat=0` (tormenta de heartbeats) se sanea a los defaults de `_setDefaults()` en vez de aceptarse (B9 confirmed).

`handleConfig()` lleva un contador `cfgTlvsPresentes` que cuenta los TLV de config recibidos independientemente del rango. El resultado base pasa a `INVALID_VALUE` cuando `cfgTlvsPresentes > 0 && changesApplied == 0`, es decir, cuando el cliente mandó parámetros pero todos cayeron fuera de rango; un paquete con solo `REBOOT`/`RESET_STATS`/`VERSION` sigue respondiendo `OK`, y un fallo de `saveConfig()` sigue pisando con `FAIL` (B10 confirmed).

### Causa de reinicio por enum numérico

`detectBootReason()` usa `ESP.getResetInfoPtr()->reason` como fuente primaria, mapeando `REASON_WDT_RST`/`REASON_SOFT_WDT_RST`→WATCHDOG, `REASON_EXCEPTION_RST`→CRASH, `REASON_SOFT_RESTART`→SOFTWARE_RESET, `REASON_DEEP_SLEEP_AWAKE`→DEEP_SLEEP, `REASON_EXT_SYS_RST`→POWER_ON. El antiguo parseo de texto se extrajo a `bootReasonFromText()` y queda como fallback cuando el enum no es concluyente (`REASON_DEFAULT_RST` o valor no mapeado), eliminando la degradación silenciosa a UNKNOWN por cambios de formato del texto (B12 confirmed).

### BOOT_ID volátil: modo degradado documentado y más entropía

`_volatileBootId()` ahora combina `micros()`, `ESP.getCycleCount()`, `ESP.getChipId()` y cuatro lecturas del ADC flotante, plegando los 32 bits a `uint16_t` con la garantía `id != 0`. El comentario explica el modo degradado: cuando `!isBootIdPersistent()` el BOOT_ID puede repetirse entre reinicios y la central puede descartar como replay los primeros eventos (SEQ bajos) hasta superar la ventana de dedup. La mejora reduce la probabilidad de colisión sin tocar el protocolo ni la ventana; no la elimina, y el comentario lo deja claro (B11 confirmed, no bloqueante).

### Postura de seguridad: credenciales no filtradas a archivos versionados

`git ls-files` confirma que lo único trackeado en esta familia es `secrets.h.template` y `network_config.h.template`; ni `secrets.h` ni `network_config.h` aparecen (confirmed). `secrets.h.template` conserva su `IOT_AUTH_KEY` placeholder `{0x4D,0x79,...,0x21,0x21}`, que es el ASCII "MyIoTKeySecret!!" — no la clave real (confirmed). `network_config.h` figura como `D` en el índice (destrackeado vía `git rm --cached`, archivo local intacto) y `network_config.h.template` como `A`. La rotación de la clave HMAC vive solo en el `secrets.h` raíz no versionado, compartido por emisor y receptor vía `-I../`, por lo que ambos quedan con los mismos 16 bytes por construcción. No hay commit ni push; los cambios están en el working tree (confirmed).

### Consolidación del puerto UDP y versión

El puerto se define una sola vez como `IOT_UDP_PORT_DEFAULT 4210` en `network_config.h.template`, consumido por `PUERTO_UDP` (legacy) y referenciado por `UDP_PORT` en `device_config.cpp`, que ahora incluye `network_config.h`. El comentario de cabecera de `device_config.h` se alineó de V4.1 a V4.3 (B15 confirmed). Nota de contexto: `UDP_PORT = IOT_UDP_PORT_DEFAULT` exige que `device_config.cpp` resuelva `network_config.h`; el coder reporta compilación exitosa del emisor, lo que confirma que el include resuelve con los `build_flags`/`lib_extra_dirs` del proyecto.

### Compatibilidad del protocolo y evidencia de compilación

No hay cambios de firma pública (`sendEvent` ya devolvía bool), ni del enum `Priority`/flags (`IOT_FLAG_URGENT=0x04` ya existía y mapea a `Priority::URGENT`), ni del formato de serialización. La librería compartida la compilan ambos firmwares; el coder registró `pio run -e emisor_unificado` y `pio run -e receptor_central_unificado` ambos con `[SUCCESS]`/`1 succeeded`, con el código de salida 1 atribuido al banner "Obsolete PIO Core" y no a un fallo de build. Conforme al enunciado, este review no re-ejecutó `pio run`; se apoya en esa evidencia y en la lectura del diff (confirmed por lectura; compilación not verified por este review por diseño).

**No probado:** los checks host unitarios (`tests/host_compile`, `tests/storage_host`) no se ejecutaron porque `g++` no está disponible en la máquina; B5/B7/B9 se verificaron por compilación `pio` + lectura de código, no por sus pruebas unitarias dedicadas. El comportamiento en runtime de la reconexión WiFi y de OTA sobre hardware real no se validó (fuera del alcance de un review estático).

</details>

<details>
<summary>Mapa de archivos</summary>

- `emisor_pir_unificado/include/device_config.h` — comentario de versión V4.1 → V4.3.
- `emisor_pir_unificado/src/device_config.cpp` — `UDP_PORT` referencia `IOT_UDP_PORT_DEFAULT`; incluye `network_config.h`.
- `emisor_pir_unificado/src/main.cpp` — `manejarWiFi()` no bloqueante + OTA al conectar; `setup()` sin espera bloqueante; `detectBootReason()` por enum numérico; MOTION/TIMBRE reportan pérdida real.
- `emisor_pir_unificado/src/ota.cpp` — `onEnd` baja `actualizacionEnProgreso` antes del log.
- `lib/IoTProtocol/IoTConfigHandler.cpp` — `cfgTlvsPresentes`; respuesta `INVALID_VALUE`; rangos vía defines.
- `lib/IoTProtocol/IoTNode.cpp` — `sendEvent()` reserva/commit de SEQ + flag URGENT; heartbeat recentrado con `_hbJitter`.
- `lib/IoTProtocol/IoTNode.h` — nuevo miembro privado `_hbJitter`.
- `lib/IoTProtocol/IoTProtocol.h` — defines `IOT_CFG_*` (fuente única de rangos).
- `lib/IoTProtocol/IoTStorage.cpp` — `loadConfig()` sanea rangos; `_volatileBootId()` con más entropía y modo degradado documentado.
- `network_config.h` — destrackeado (`git rm --cached`), permanece local.
- `network_config.h.template` — plantilla versionada con `IOT_UDP_PORT_DEFAULT`.

Diff completo: `git diff HEAD` en la raíz del repo.

</details>
