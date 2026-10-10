# Revisión de código — emisor_pir_unificado

Firmware ESP8266 (PlatformIO, board `d1_mini`) para un emisor PIR + timbre que se comunica con una central por **UDP** usando la librería propia `IoTProtocol` (V4.3), con autenticación HMAC-SHA256 truncada opcional y OTA vía ArduinoOTA. Revisión de solo lectura; no se modificó ningún archivo.

Nota sobre el enunciado: el pedido menciona MQTT, pero este proyecto **no usa MQTT**. La comunicación es UDP a través de `IoTNode`. Los `#define MQTT_*` de `secrets.h` no se referencian en el código del emisor (son herencia de otros subproyectos como `receptor_central_unificado`). Los hallazgos de "TLS en MQTT" no aplican; en su lugar se evalúa la postura de seguridad real (HMAC sobre UDP y OTA).

**Verdict**: NEEDS_CHANGES

---

## Resumen ejecutivo por severidad

### Crítico

1. **`manejarWiFi()` no está definida — el firmware no compila/enlaza** *(confirmado)*
   - Archivo: `src/main.cpp`, línea ~353 (dentro de `loop()`).
   - `loop()` llama a `manejarWiFi();` pero esa función no está definida en ninguno de los fuentes del proyecto (`main.cpp`, `device_config.cpp`, `ota.cpp`) ni en la librería compartida `lib/IoTProtocol`. Solo existe en otros subproyectos (`legacy/emisor_pir`, `receptor_central_unificado`, `legacy/receptor_bocina`), que no se compilan dentro de este `env`.
   - Verificado: listado completo de `src/` e `include/` (3 .cpp + 3 .h) y búsqueda de `manejarWiFi` en todo el repo y en `lib/`.
   - Consecuencia: error de enlace (`undefined reference to manejarWiFi()`). Tal como está, este `env` no produce binario.
   - Recomendación: implementar `manejarWiFi()` en este proyecto (reconexión no bloqueante con backoff) o eliminar la llamada si la reconexión se delega a otra parte. Dado el hallazgo HIGH-1, lo correcto es implementarla.

2. **Secretos reales versionados en texto plano en el árbol de trabajo** *(confirmado)*
   - Archivo: `secrets.h` (raíz del repo).
   - Contiene credenciales reales: `WIFI_PASSWORD "72994559"`, `MQTT_PASSWORD "3ePGMjOZQ6HsfCeYroaE7zPH"`, `MQTT_USER "admin"`, `OTA_PASSWORD "prueba159875"`, y el SSID real `"Diaz"`.
   - Verificado: `git ls-files secrets.h` y `git log --all --full-history -- secrets.h` no devuelven nada — el archivo **nunca** entró al índice ni al historial. El `.gitignore` (`secrets.h`, `**/secrets.h`) cumplió su función. Riesgo de exposición pública: bajo. Riesgo real: las credenciales están en claro en el disco de trabajo y se compilan embebidas en el binario, por lo que quien obtenga un `.bin` (p. ej. por OTA o volcado de flash) puede extraerlas.
   - Recomendación: dado que no están en el historial, no hace falta reescribirlo. Mantener solo `secrets.h.template` versionado (ya es el caso) y tratar los `.bin` como material sensible. Rotar la clave HMAC por el motivo del CRÍTICO-3.

3. **Clave HMAC compartida = valor por defecto del template** *(confirmado)*
   - Archivos: `secrets.h` e `secrets.h.template` (`IOT_AUTH_KEY`).
   - La clave en `secrets.h` es byte a byte idéntica a la del template: `{ 0x4D,0x79,0x49,0x6F,0x54,0x4B,0x65,0x79,0x53,0x65,0x63,0x72,0x65,0x74,0x21,0x21 }`, es decir el ASCII `"MyIoTKeySecret!!"`. Es una clave pública conocida (está en el template versionado), por lo que la autenticación HMAC no aporta seguridad real: cualquiera con el repo puede firmar paquetes válidos.
   - Recomendación: generar una clave aleatoria real (`openssl rand -hex 16`) por despliegue y mantenerla fuera del repo. El template debe traer un placeholder obvio, no una clave que parezca válida.

### Alto

1. **No hay reconexión WiFi real; OTA no se reinicializa tras reconectar** *(confirmado)*
   - Archivo: `src/main.cpp`, `setup()` (línea ~300) y `loop()` (línea ~353).
   - `setupOTA()` solo se invoca una vez en `setup()` y únicamente si el WiFi conectó dentro de los 10 s iniciales. En `loop()` solo se llama `handleOTA()`, que no hace nada si `otaInicializada == false` (ver `ota.cpp`). Si el WiFi no conecta al arranque, OTA **nunca** se inicializa, aun cuando el WiFi vuelva más tarde. El log de setup dice "OTA se inicializará al reconectar", pero no existe código que lo haga.
   - Sumado al CRÍTICO-1 (`manejarWiFi()` ausente), hoy no hay ninguna lógica de reconexión. Cuando se implemente `manejarWiFi()`, debe además llamar a `setupOTA()` al recuperar la conexión (patrón ya usado en `receptor_central_unificado`).
   - Recomendación: implementar reconexión no bloqueante que reintente `WiFi.begin()` con backoff y, al pasar a `WL_CONNECTED`, llame a `setupOTA()`.

2. **Espera de WiFi bloqueante de hasta 10 s en `setup()`** *(confirmado)*
   - Archivo: `src/main.cpp`, línea ~306 (`while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000)`).
   - Bloquea el arranque hasta 10 s. Alimenta el watchdog con `ESP.wdtFeed()` dentro del bucle, así que no se reinicia, pero retrasa el inicio de `IoTNode`, del heartbeat y del monitoreo de sensores. Un evento PIR durante ese arranque se pierde.
   - Recomendación: hacer el arranque no bloqueante. `IoTNode` ya tolera encolar sin WiFi ("Buffering sin WiFi"), así que no es necesario esperar la conexión antes de `node.begin()`. Dejar que `manejarWiFi()` gestione la conexión en `loop()`.

### Medio

1. **OTA sin control de versión/rollback ni validación de origen más allá de la contraseña** *(confirmado)*
   - Archivo: `src/ota.cpp`.
   - ArduinoOTA con contraseña protege contra flasheos casuales en la LAN, pero no valida firma del binario ni versión; cualquiera con la contraseña (y la contraseña está en `secrets.h`, ver CRÍTICO-2) puede subir firmware arbitrario. Es aceptable para una red doméstica de confianza, pero conviene documentarlo como límite explícito.
   - Recomendación: documentar el modelo de amenaza (LAN de confianza). Si se requiere más, considerar firma de imagen. Como mínimo, asegurar que `OTA_PASSWORD` sea fuerte y única por dispositivo.

2. **`onError` de OTA limpia el flag pero el estado puede quedar inconsistente** *(likely)*
   - Archivo: `src/ota.cpp`, callbacks `onStart`/`onEnd`/`onError`.
   - `actualizacionEnProgreso` se pone en `true` en `onStart` y en `false` en `onError`, pero `onEnd` no lo baja (asume reinicio inmediato). Si por alguna razón `onEnd` se dispara sin reiniciar, el flag queda en `true` y `otaEnProgreso()` haría que `loop()` haga `return` para siempre, congelando los sensores. Es un caso borde (ArduinoOTA normalmente reinicia tras `onEnd`), de ahí "likely" y no "confirmado".
   - Recomendación: bajar `actualizacionEnProgreso` también en `onEnd` antes del reinicio, por robustez.

3. **`detectBootReason()` depende de substrings frágiles de `ESP.getResetReason()`** *(likely)*
   - Archivo: `src/main.cpp`, línea ~95.
   - La detección se basa en `strstr` sobre el texto de `getResetReason()`, que varía entre versiones del core ESP8266 y es sensible a mayúsculas/formato (ya se intenta cubrir "wdt"/"WDT"/"Watchdog"). Un cambio de core puede degradar la clasificación a `UNKNOWN` silenciosamente. No es un bug activo, pero es mantenimiento frágil.
   - Recomendación: donde sea posible usar `ESP.getResetInfoPtr()->reason` (enum numérico estable) en lugar de parsear texto.

4. **Antirrebote del PIR viene de config remota sin validación de rango visible en el emisor** *(possible)*
   - Archivo: `src/main.cpp`, `loop()` (línea ~360) usa `storage.config().antireboteMs` para el PIR.
   - Si la config remota puede fijar `antireboteMs = 0` o un valor enorme, el filtrado de rebote se vuelve inefectivo o bloquea detecciones. No se ve validación de rango en el emisor; podría estar en `IoTConfigHandler`/`IoTStorage` (no revisado a fondo). Marcado "possible": requiere revisar `IoTConfigHandler.cpp` para confirmar si valida rangos.
   - Recomendación: confirmar que la capa de config valida rangos de `antireboteMs` y `heartbeatIntervalMs`; si no, acotar en el emisor.

### Bajo

1. **`delay(100)` tras `Serial.begin` y `delay(100)` por iteración en el bucle de espera** *(confirmado)*
   - Archivo: `src/main.cpp`, línea ~286 y ~307. Menor; el de setup es inofensivo, el del bucle de espera desaparece al hacer el arranque no bloqueante (HIGH-2).

2. **Comentario de versión desincronizado** *(confirmado)*
   - `include/device_config.h` dice "V4.1", `main.cpp` dice "V4.3", `FW_VERSION = "4.3.0"`. Alinear comentarios para evitar confusión.

3. **`UDP_PORT` duplicado** *(confirmado)*
   - `device_config.cpp` define `UDP_PORT = 4210` y `network_config.h` define `PUERTO_UDP = 4210`. Dos fuentes de verdad para el mismo puerto. Consolidar en una.

4. **Casteo de RSSI a `int8_t`** *(confirmado, menor)*
   - `src/main.cpp`, `sendStateReport()`: `(int8_t)WiFi.RSSI()`. RSSI en dBm suele estar en rango negativo pequeño (-30 a -90), cabe en `int8_t`, así que es correcto en la práctica; solo dejar comentado el supuesto.

---

## Observaciones positivas

- Buena higiene de secretos a nivel de configuración: `.gitignore` cubre `secrets.h` y `network_config.h`, existe `secrets.h.template`, y la contraseña OTA de `platformio.ini` se toma de variable de entorno (`${sysenv.WIFI_PIR_OTA_PASSWORD}`) en vez de hardcodearse en el env de upload.
- El estado lógico de los sensores se sincroniza con el nivel físico al arranque (`pirAnterior`/`timbreAnterior`) para no emitir un flanco falso en el primer loop.
- `STATE_REPORT` arma el payload con verificación de cada TLV y aborta el envío si quedó incompleto, en lugar de mandar datos parciales.
- Watchdog habilitado (`ESP.wdtEnable(8000)`) y alimentado en `loop()` y en el bucle de espera de WiFi.
- Monitoreo de salud (heap mínimo histórico) y reintento no destructivo del montaje de LittleFS con backoff.
- La frontera de autenticación está bien pensada a nivel de diseño: firma centralizada en `IoTNode`, sin doble firma, verificación antes de registry/ACK/dedup.
- `platformio.ini` fija `platform`, `board` y `framework` correctos para un Wemos D1 mini (ESP8266).

---

## Nota sobre pinned de librerías (build)

`platformio.ini` no declara `lib_deps` con versiones pinneadas; usa `lib_extra_dirs = ../lib` para las librerías propias (`IoTProtocol`, `AlarmProfile`) y depende del framework Arduino de la plataforma `espressif8266` sin fijar versión de plataforma. ArduinoOTA, ESP8266WiFi y LittleFS vienen del core, por lo que la versión efectiva la determina la versión de `platform = espressif8266` instalada. Recomendación (bajo/medio): pinnear la versión de la plataforma (p. ej. `platform = espressif8266@4.2.1`) para builds reproducibles, dado que el core determina el comportamiento de WiFi/OTA/LittleFS.

---

## Resumen de acciones (checklist)

1. [Crítico] Implementar `manejarWiFi()` en este proyecto (o quitar la llamada). Hoy no enlaza.
2. [Crítico] `secrets.h` no está en el historial (verificado), pero las credenciales van embebidas en el `.bin`: tratar los binarios como material sensible.
3. [Crítico] Reemplazar la `IOT_AUTH_KEY` por una clave aleatoria real; dejar un placeholder en el template.
4. [Alto] Añadir reconexión WiFi no bloqueante que reinvoque `setupOTA()` al recuperar conexión.
5. [Alto] Volver no bloqueante la espera de WiFi en `setup()`.
6. [Medio] Bajar `actualizacionEnProgreso` en `onEnd`; documentar modelo de amenaza OTA.
7. [Medio] Usar `getResetInfoPtr()->reason` en vez de parsear texto; validar rangos de config.
8. [Bajo] Alinear comentarios de versión, consolidar el puerto UDP duplicado.
9. [Build] Pinnear versión de `platform = espressif8266`.
