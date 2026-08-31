# Registro canónico de bugs y correcciones

## Propósito y regla de evidencia

Este documento conserva síntomas, causas, soluciones históricas y reglas preventivas sin confundir una propuesta con una corrección verificada. El archivo histórico `_drafts/BUGS_FIXED.md` se mantiene como fuente de procedencia; este registro es la referencia actual para el estado.

La documentación no reemplaza al código ni a las pruebas. Los estados permitidos son:

```text
PROPUESTO → APLICADO → COMPILADO → VERIFICADO → VERIFICADO EN HARDWARE
```

También pueden usarse `RECHAZADO` y `FUERA DE ALCANCE`. Si una entrada tiene una corrección aplicada pero no una prueba, debe decir explícitamente `APLICADO; no verificado`, nunca simplemente “resuelto”.

**Advertencia global:** el análisis que originó este registro fue principalmente estático. No se realizaron en esta auditoría compilaciones, pruebas host, pruebas MQTT/Home Assistant, OTA ni pruebas hardware.

## BUG-001 — MQTT puede bloquear la recepción UDP

**Síntoma:** un TIMBRE o alarma puede no ser atendido mientras el receptor está dentro de `mqtt.connect()`.

**Causa:** en ESP8266, una conexión TCP/MQTT que no responde puede bloquear varios segundos. `setSocketTimeout()` no convierte el handshake inicial de `connect()` en no bloqueante.

**Medida existente:** V3 usa modos LOCAL/HA, backoff y prioridad de UDP; el modo LOCAL evita intentos MQTT continuos.

**Estado:** APLICADO parcialmente; no verificado exhaustivamente con la versión exacta de las dependencias.

**Regla:** no llamar a `connect()`, DNS, HTTP ni operaciones largas en el camino crítico del loop. Medir el tiempo real antes de cambiar la política.

**Prueba pendiente:** broker caído durante una ráfaga de UDP; registrar tiempo máximo del loop y recepción de TIMBRE/PIR.

## BUG-002 — PIR y TIMBRE se bloquean mutuamente

**Síntoma:** el segundo evento se pierde cuando el primero está esperando ACK.

**Causa:** la implementación antigua tenía un único estado/canal de transmisión (`txState`), por lo que un evento ocupaba el emisor completo.

**Medida existente:** V3.5 usa envío asíncrono y hasta cuatro eventos en vuelo, con ACK y reintentos en segundo plano.

**Estado:** APLICADO en el diseño/código V3; no verificado completamente en hardware.

**Regla:** cada sensor debe poder generar un evento sin esperar la confirmación de otro sensor.

**Prueba pendiente:** activar PIR y TIMBRE casi simultáneamente y comprobar recepción, ACK, reintentos y acción física de ambos.

## BUG-003 — PIR deja de detectar o responde con demasiada demora

**Síntoma:** tras una detección, el PIR parece no volver a responder; activaciones próximas pueden ignorarse.

**Causa:** se combinaban un antirrebote demasiado largo con el comportamiento del HC-SR501, cuya salida puede permanecer en `HIGH` durante segundos.

**Medida existente:** el diseño redujo el antirrebote y usa detección de flanco/temporización; el ajuste físico del módulo sigue siendo parte del comportamiento real.

**Estado:** APLICADO parcialmente; prueba con el HC-SR501 real pendiente.

**Regla:** tratar PIR como sensor de nivel con duración configurable, no como botón de flanco limpio. No añadir un bloqueo de varios segundos como antirrebote genérico.

**Prueba pendiente:** medir el tiempo `HIGH` del módulo, activaciones separadas y PIR sostenido mientras se pulsa TIMBRE.

## BUG-004 — `LOCAL` colisiona con una macro del SDK

**Síntoma:** un enum o identificador que usa `LOCAL` puede fallar al compilar.

**Causa:** el preprocesador del SDK ESP8266 puede definir `LOCAL` como macro.

**Corrección existente:** usar nombres específicos como `MODO_LOCAL` y `MODO_HA`.

**Estado:** APLICADO en el código actual según el análisis.

**Regla:** no usar `LOCAL`, `REMOTE` u otros nombres potencialmente definidos por el SDK como identificadores propios.

**Prueba pendiente:** conservar una compilación V3 de línea base para evitar regresiones.

## BUG-005 — OTA muestra “No response from device”

**Síntoma:** el upload OTA no recibe respuesta aunque el dispositivo esté encendido.

**Causa histórica propuesta:** el firewall del equipo desde el que se ejecuta PlatformIO bloquea conexiones entrantes o puertos altos usados durante la sesión OTA.

**Procedimiento conservado:** revisar firewall y rutas del entorno real antes de probar OTA. Los comandos históricos no son una garantía universal ni deben ejecutarse sin autorización.

**Estado:** APLICADO documentalmente; no verificado en el entorno real.

**Regla:** separar la configuración de firewall del firmware y registrar interfaz, red, regla aplicada y resultado de cada prueba OTA.

**Prueba pendiente:** ejecutar OTA desde el PC real, con firewall documentado, y confirmar también recuperación ante fallo.

## BUG-006 — `espClient.connect()` usado como prueba rápida bloquea el loop

**Síntoma:** `verificarTCPBroker()` parecía un health check breve pero detenía el procesamiento durante aproximadamente varios segundos.

**Causa:** `WiFiClient::connect()` puede bloquear durante el establecimiento TCP; configurar el timeout de lectura no resuelve ese handshake.

**Corrección existente:** retirar el test TCP de la ruta crítica y usar backoff/modo LOCAL.

**Estado:** APLICADO como regla preventiva; no se considera una garantía de tiempo sin medición.

**Regla:** nunca probar disponibilidad de MQTT con un `connect()` síncrono en cada iteración del loop.

**Prueba pendiente:** revisar el código actual y medir cualquier intento de conexión según la versión exacta del core ESP8266.

## BUG-007 — UDP no funciona entre repetidor, router o segmentos distintos

**Síntoma histórico:** ping funciona, pero el receptor no recibe UDP cuando emisor y receptor atraviesan ciertos equipos de red.

**Causa propuesta:** aislamiento de clientes, filtrado UDP, ARP o NAT entre AP/repetidor/router. El material histórico menciona B622, doble NAT y DMZ, pero el repositorio no demuestra esa topología.

**Medidas posibles:** verificar AP/client isolation, modo bridge, gateways, DHCP, subredes, rutas, firewall y accesibilidad del puerto UDP.

**Estado:** PROPUESTO / no verificado; no tratar las IPs, B622 o DMZ del draft como hechos del proyecto.

**Regla:** no cambiar firmware ni arquitectura de red basándose solo en un documento histórico. Medir la topología real primero.

**Prueba pendiente:** registrar gateway y subnet en cada dispositivo, capturar tráfico UDP y confirmar ruta de ida y vuelta.

## BUG-008 — deduplicación incorrecta después de reiniciar el emisor

**Síntoma:** el primer paquete después de un reinicio puede confundirse con un paquete anterior si reutiliza `SEQ`.

**Causa:** usar solo secuencia o una ventana sin distinguir la sesión de arranque.

**Situación real:** el fix histórico de V4.1 ya introdujo un `BOOT_ID` nuevo por arranque dentro de `IoTNode`, lo que separa sesiones aunque el valor sea generado por el nodo. Sin embargo, `IoTStorage::getBootId()` implementa además un contador persistente y los firmwares V4 no lo pasan al nodo; esa persistencia es un endurecimiento posterior todavía pendiente.

**Estado:** APLICADO parcialmente y no verificado para la separación de sesiones; BOOT_ID persistente integrado desde `IoTStorage`: PROPUESTO / pendiente V4.

**Corrección prevista:** obtener una vez el `BOOT_ID` persistente al arrancar, pasarlo explícitamente a `IoTNode`, definir fallback si LittleFS falla y probar cambio de sesión, rollover y ventana de deduplicación.

**Regla:** distinguir el fix de identidad por sesión del endurecimiento de persistencia. La deduplicación debe considerar sesión (`BOOT_ID`) y secuencia; la existencia de una API de storage no demuestra que esté conectada al flujo real.

**Prueba pendiente:** dos arranques consecutivos, contador corrupto, LittleFS no montado, mismo `SEQ` con BOOT_ID distinto y retransmisión duplicada.

## BUG-009 — `AUTH_KEY` expuesta en código versionado

**Síntoma:** una clave HMAC puede quedar hardcodeada en un `.cpp`/`.h` versionado o en el historial Git.

**Causa:** copiar secretos durante el desarrollo sin separar configuración privada y plantilla pública.

**Medida existente:** `secrets.h` está separado de `secrets.h.template` y se excluye mediante `.gitignore`, según la estructura analizada.

**Estado:** APLICADO como prevención; auditoría histórica de Git pendiente.

**Regla:** no imprimir, commitear ni incluir claves reales en firmware/documentación/logs. Un `.gitignore` no elimina una exposición pasada.

**Prueba pendiente:** revisar el historial con búsquedas de rutas y nombres de clave; si hubo exposición, rotar la clave y documentar solo el hecho, nunca el valor.

## BUG-010 — autenticación tardía o con decisiones duplicadas

**Síntoma:** la política de auth puede aceptar/rechazar de forma inconsistente o permitir efectos internos antes de descartar un paquete inválido.

**Causa histórica:** había decisiones distribuidas entre `event_handler.cpp` e `IoTAuth`; además, según el análisis actual, `_processIncoming()` puede actualizar registry, responder ACK o deduplicar antes de que los callbacks verifiquen HMAC.

**Situación real:** la decisión de auth del firmware está centralizada en `auth.verifyPacket()`/`_required`, pero la verificación llega desde callbacks después de que `IoTNode` puede actualizar registry, responder ACK o deduplicar. La unificación de decisión y el orden de efectos son propiedades distintas.

**Estado:** APLICADO parcialmente y no verificado para la decisión única; autenticación temprana dentro del núcleo: PROPUESTO / pendiente V4.

**Corrección prevista:**

```text
recibir → parsear/CRC → verificar auth → anti-replay/dedup
→ actualizar registry según política → ACK válido → callback
```

La política debe decidir si paquetes inválidos reciben ACK; la recomendación inicial es no reconocerlos.

**Regla:** una propiedad de seguridad debe tener un único punto de decisión y ejecutarse antes de ACK, registry, deduplicación y lógica de aplicación.

**Prueba pendiente:** HMAC incorrecto, HMAC ausente, duplicado válido, paquete fuera de orden y comprobación observable del orden de efectos.

## BUG-011 — discovery MQTT publica `null` en vez de JSON

**Síntoma:** el log indica que discovery se publicó, pero el dispositivo no aparece en Home Assistant; el payload retained de los topics de configuración es literalmente `null`.

**Causa:** construir un `StaticJsonDocument` vacío usando `doc.as<JsonObject>()`, que no inicializa el documento como objeto. Las asignaciones posteriores se descartan silenciosamente.

**Corrección observada en el código actual:** inicializar una sola vez y reutilizar el objeto:

```cpp
StaticJsonDocument<768> doc;
JsonObject o = doc.to<JsonObject>();
llenar(o);
agregarDevice(o);
```

**Estado:** APLICADO en el código actual; no VERIFICADO en broker/Home Assistant.

**Regla:** usar `.to<JsonObject>()` o `.to<JsonArray>()` para inicializar un documento vacío; reservar `.as<T>()` para leer/castear un valor ya existente.

**Prueba pendiente:** inspeccionar el payload retained real, validar JSON, comprobar topics y confirmar entidades en Home Assistant. No crear todavía un changelog V3.5.2 como versión verificada.

## Matriz de cierre

| Bug | Código/documentación actual | Prueba faltante | No afirmar todavía |
|---|---|---|---|
| BUG-001 | Mitigación V3 | caída de broker y métrica de loop | “MQTT nunca bloquea” |
| BUG-002 | diseño V3 asíncrono | simultaneidad en hardware | “100% probado” |
| BUG-003 | reglas y correcciones parciales | HC-SR501 real | “PIR resuelto en toda configuración” |
| BUG-004 | nombres corregidos | compilación de línea base | “ningún SDK futuro colisionará” |
| BUG-005 | procedimiento OTA | prueba en red real | “OTA garantizado” |
| BUG-006 | regla preventiva | auditoría de rutas actuales | “connect es no bloqueante” |
| BUG-007 | hipótesis de red | medición de topología | IPs/DMZ como hechos |
| BUG-008 | storage desconectado del nodo | reinicios y LittleFS | “BOOT_ID persistente implementado” |
| BUG-009 | prevención de secretos | auditoría Git | “nunca hubo exposición” |
| BUG-010 | auth tardía según análisis | prueba de orden de efectos | “auth unificada y temprana” |
| BUG-011 | `to<JsonObject>()` presente | retained MQTT + HA | “V3.5.2 verificada” |

## Procedencia y documentos relacionados

- `_drafts/BUGS_FIXED.md`: fuente histórica de BUG-001…BUG-010.
- `_drafts/bugs.md`: fuente histórica de BUG-011 y de hipótesis de topología.
- [`ANALISIS_INICIAL_HALLAZGOS.md`](ANALISIS_INICIAL_HALLAZGOS.md): matriz H-001…H-018 y método de evidencia.
- [`PLAN_EJECUCION_FUTURA.md`](PLAN_EJECUCION_FUTURA.md): fases para corregir BUG-008 y BUG-010 y validar bugs operativos.
- [`universal-protocol/INFORME_DRAFTS_RESTANTES.md`](universal-protocol/INFORME_DRAFTS_RESTANTES.md): auditoría completa de los cinco drafts restantes.

La ausencia de una marca de “VERIFICADO” es intencional: conserva las soluciones sin convertir documentación histórica en evidencia de pruebas que no se ejecutaron.
