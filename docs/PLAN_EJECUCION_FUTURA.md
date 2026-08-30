# Plan de ejecución futura — wifi_PIR

> **Documento canónico para continuar el proyecto sin consultar archivos históricos o borradores.**
>
> Este documento reúne el estado técnico conocido, las decisiones que deben conservarse, los problemas detectados, el orden de ejecución recomendado, los criterios de aceptación y el trabajo futuro. Una LLM que inicie una sesión nueva debe leer primero este archivo y después inspeccionar el código actual antes de modificarlo.

**Repositorio:** `ydiaz1699/wifi_PIR`
**Proyecto:** red doméstica de sensores y alarma sobre ESP8266
**Fecha de consolidación:** 2026-08-29
**Estado de este plan:** aprobado como guía de ejecución futura; no implica que las fases estén implementadas.

---

## 1. Cómo debe usar este documento una LLM futura

Antes de cambiar código:

1. Leer este documento completo o, como mínimo, las secciones 2, 3, 4, 6 y 7.
2. Ejecutar `git status --short` y conservar cualquier cambio previo del usuario.
3. Leer los archivos reales relacionados con la fase que se va a ejecutar. Este plan describe la intención, pero el código es la fuente de verdad de las firmas, nombres y estructuras actuales.
4. No asumir que una tarea está implementada porque aparezca en la documentación. Verificarla en el código y mediante compilación o prueba.
5. No mezclar V3 de producción con V4 de desarrollo salvo que una tarea lo pida expresamente.
6. No eliminar ni sobrescribir funcionalidad de producción para acelerar el desarrollo de V4.
7. Ejecutar primero las comprobaciones pequeñas y después las pruebas completas.
8. Actualizar este documento, `CHANGELOG.md`, `ROADMAP.md` o la documentación correspondiente cuando una fase se complete. La documentación no debe afirmar que una función está terminada si no existe una prueba que lo confirme.

### Regla de trabajo principal

La versión V3.5.1 es la alarma operativa y debe permanecer estable. La versión V4.3 es el laboratorio de la biblioteca `IoTProtocol`. La ruta segura es:

```text
preservar V3 → crear pruebas host para V4 → corregir seguridad V4
→ validar con simulador/hardware → documentar → añadir sensores
→ evaluar una futura V5
```

No se debe reescribir todo el repositorio para resolver una sola incidencia de V4.

---

## 2. Estado actual que debe conservarse

### 2.1 V3.5.1 — producción

Directorios principales:

```text
emisor_pir/
receptor_bocina/
```

Características actuales:

- Protocolo de texto sobre UDP.
- El emisor envía mensajes como:

  ```text
  PIR01|5|TIMBRE
  ```

- El receptor responde con ACK:

  ```text
  OK|5
  ```

- El emisor usa envío inmediato, cola de eventos en vuelo y confirmación asíncrona.
- Hay hasta cuatro eventos simultáneos en el emisor V3.
- El receptor procesa varios datagramas por ciclo mediante un drain loop, con límite de ocho paquetes por ciclo.
- El ACK se envía antes de comprobar duplicados, incluso si el paquete ya fue procesado.
- La deduplicación mantiene una ventana circular de ocho identificadores por emisor.
- PIR y TIMBRE tienen caminos de procesamiento independientes.
- El receptor tiene modo local y modo Home Assistant/MQTT.
- UDP tiene prioridad sobre MQTT.
- La bocina utiliza actualmente `timedOn()` y `isOn()`.
- El modo LOCAL debe continuar funcionando aunque MQTT esté caído.

Archivos V3 relevantes:

```text
emisor_pir/src/main.cpp
emisor_pir/include/device_config.h
receptor_bocina/src/main.cpp
receptor_bocina/src/alarma.cpp
receptor_bocina/src/mqtt_cliente.cpp
receptor_bocina/src/mqtt_discovery.cpp
receptor_bocina/src/hal.cpp
receptor_bocina/include/hal.h
receptor_bocina/src/state_machine.cpp
```

### 2.2 V4.3 — desarrollo

Directorios principales:

```text
lib/IoTProtocol/
emisor_pir_v4/
receptor_central_v4/
```

Características actuales:

- Protocolo binario propio sobre UDP.
- Cabecera fija, TLV, CRC16, prioridades, ACK, heartbeat, discovery y registry.
- `IoTNode` dispone de cola FIFO de ocho entradas.
- Solo hay un paquete reliable en vuelo a la vez.
- Se usa una ventana de deduplicación de ocho secuencias por dispositivo remoto.
- Existe `IoTStorage` con LittleFS para contador de arranque y configuración.
- Existe `IoTAuth` basado en HMAC-SHA256 truncado a cuatro bytes.
- Existe configuración remota básica mediante `IoTConfigHandler`.
- Existe seguimiento `ONLINE`, `STALE` y `OFFLINE`.
- Existe medición de RTT y estadísticas.
- Existe heartbeat enriquecido.
- Existe watchdog en los firmwares V4.

Archivos V4 relevantes:

```text
lib/IoTProtocol/IoTProtocol.h
lib/IoTProtocol/IoTProtocol.cpp
lib/IoTProtocol/IoTPacket.*
lib/IoTProtocol/IoTNode.h
lib/IoTProtocol/IoTNode.cpp
lib/IoTProtocol/IoTAuth.h
lib/IoTProtocol/IoTAuth.cpp
lib/IoTProtocol/IoTStorage.h
lib/IoTProtocol/IoTStorage.cpp
lib/IoTProtocol/IoTConfigHandler.h
lib/IoTProtocol/IoTConfigHandler.cpp
emisor_pir_v4/src/main.cpp
receptor_central_v4/src/main.cpp
receptor_central_v4/src/event_handler.cpp
```

### 2.3 Diferencia crítica entre V3 y V4

No confundir estas dos afirmaciones:

- V3 sí es la versión de producción.
- V4 tiene documentación de funciones avanzadas, pero algunas todavía son parciales o solo están conectadas en una dirección.

El trabajo futuro debe verificar cada característica. En particular, la documentación actual presenta BOOT_ID persistente y autenticación unificada como parte de V4.3, pero el código todavía no cumple completamente esas dos propiedades.

---

## 3. Reglas técnicas que no deben romperse

### 3.1 Reglas del loop

1. El loop del receptor no debe bloquearse más de aproximadamente 100 ms.
2. Nunca usar `WiFiClient::connect()` como prueba rápida de disponibilidad: en ESP8266 puede bloquear cerca de cinco segundos.
3. Nunca hacer una conexión TCP o DNS en cada iteración del loop.
4. UDP debe procesarse antes que MQTT.
5. Las funciones de recepción no deben esperar ACK de forma síncrona.
6. Si una operación potencialmente bloqueante es inevitable, ejecutarla solo con temporización, fuera de la ruta crítica y, cuando corresponda, con la bocina apagada.
7. No usar `delay()` en el loop de sensores. Solo se permiten retardos muy pequeños cuando sean indispensables para un protocolo de hardware.

### 3.2 Reglas de sensores

1. PIR no se comporta como un botón: el HC-SR501 puede permanecer en `HIGH` durante varios segundos.
2. No aplicar un antirrebote de varios segundos al PIR.
3. Para el PIR se recomienda detectar el cambio de estado y controlar el tiempo mínimo entre eventos con `millis()`.
4. El timbre debe poder generar un evento aunque el PIR esté activo.
5. PIR y TIMBRE no deben compartir una máquina de estados bloqueante.
6. Un emisor no debe esperar la confirmación de un evento para leer el otro sensor.

### 3.3 Reglas de nombres, red y secretos

1. No usar `LOCAL` como identificador o enum si puede colisionar con macros del SDK ESP8266. Usar nombres como `MODO_LOCAL`, `MODO_HA` o equivalentes específicos.
2. Las credenciales deben vivir en `secrets.h`, que no se versiona.
3. El repositorio solo debe contener `secrets.h.template` con valores ficticios.
4. No imprimir claves ni incluirlas en logs.
5. No hardcodear IPs de broadcast. Usar `WiFi.broadcastIP()` cuando se necesite broadcast local.
6. Confirmar la topología real antes de modificar gateway, subnet, broker o DMZ.
7. No asumir que una IP descrita en un documento es correcta si no coincide con el código o con una medición de red.

### 3.4 Reglas del wire format V4

Para V4.x, el wire format se considera congelado mientras no se decida una nueva versión mayor.

No cambiar silenciosamente:

- El orden de campos de la cabecera.
- El tamaño de los campos existentes.
- El significado de una bandera ya publicada.
- El tamaño máximo de payload sin actualizar todos los consumidores.

Se pueden añadir tipos nuevos compatibles, siempre que los nodos antiguos puedan ignorarlos con seguridad. Un cambio incompatible exige incrementar `PROTOCOL_MAJOR` y documentar la migración.

### 3.5 Regla de cambios

Antes de editar un archivo:

```bash
git status --short
```

Después de cada unidad lógica:

```bash
git diff --check
git diff --stat
```

No hacer commit automáticamente. El responsable del repositorio debe revisar los cambios antes de publicar.

---

## 4. Estado real de los problemas importantes

Esta sección reemplaza la memoria de los informes históricos. El estado indicado debe verificarse de nuevo antes de ejecutar una fase.

### BUG-001 — MQTT puede bloquear la recepción

**Estado:** parcialmente mitigado en V3, no demostrado de forma exhaustiva.

Medidas existentes:

- El receptor procesa primero la alarma/UDP.
- Hay modo LOCAL sin MQTT continuo.
- La reconexión MQTT tiene backoff y fallos controlados.

Riesgo restante:

- La conexión inicial MQTT puede bloquear.
- La cifra de aproximadamente cinco segundos depende de la versión exacta del core ESP8266 y de PubSubClient.

Acción futura:

- Medir el tiempo real en la versión usada.
- Mantener MQTT fuera del camino crítico.
- No reintroducir `verificarTCPBroker()` ni una prueba TCP equivalente.

### BUG-002 — PIR y TIMBRE se bloqueaban mutuamente

**Estado:** integrado en V3.5.1 en diseño, pendiente de pruebas hardware exhaustivas.

Evidencia esperada:

- El emisor tiene múltiples eventos en vuelo.
- La recepción y confirmación son asíncronas.
- El receptor procesa varios paquetes por ciclo.

Prueba requerida:

- Activar PIR y timbre casi simultáneamente.
- Confirmar que ambos eventos se reciben, se reconocen y producen sus acciones correctas.

### BUG-003 — pérdida de eventos por procesar un solo paquete

**Estado:** integrado en V3 y parcialmente reflejado en V4.

La función V3 `manejarAlarma()` utiliza un drain loop con límite de ocho paquetes por ciclo. Mantener el límite para evitar monopolizar el loop.

### BUG-004 — uso de `LOCAL` como identificador

**Estado:** corregido en el código actual.

No reintroducir identificadores genéricos que puedan ser macros del SDK.

### BUG-005 — firewall para OTA

**Estado:** documentado, no verificado en el entorno real.

Las reglas de firewall de Windows son una condición de la red de desarrollo, no una propiedad garantizada por el firmware. Deben probarse en el equipo desde el que se hará OTA.

### BUG-006 — supuesto test TCP no bloqueante

**Estado:** el enfoque debe considerarse prohibido.

No asumir que modificar el timeout del socket hace no bloqueante a `connect()` en ESP8266. La solución es evitar el test TCP en la ruta crítica y usar el modo LOCAL/HA con backoff.

### BUG-007 — repetidor, doble NAT, B622 y DMZ

**Estado:** externo al repositorio y no verificado.

No cambiar direcciones de red basándose únicamente en una propuesta documental. Verificar:

- Gateway de cada ESP8266.
- Subnet de cada segmento.
- DHCP y reservas.
- Ruta entre el segmento de sensores y el NAS/broker.
- IP real del broker MQTT.
- Destino de la DMZ, si existe.
- Accesibilidad UDP al puerto configurado.

### BUG-008 — BOOT_ID y reinicios

**Estado:** pendiente en V4.

El problema es que `IoTStorage::getBootId()` existe, pero `IoTNode::begin()` genera un BOOT_ID aleatorio y los firmwares V4 llaman a `node.begin()` sin pasar el ID persistente.

Consecuencia:

- La intención de distinguir un reinicio de una retransmisión existe, pero la fuente persistente no está conectada.

Corrección prevista:

- Añadir una API explícita, por ejemplo:

  ```cpp
  void begin(uint16_t persistentBootId);
  ```

- Mantener `begin()` sin argumentos solo como compatibilidad o fallback documentado.
- En cada firmware V4:

  ```cpp
  uint16_t bootId = storage.getBootId();
  node.begin(bootId);
  ```

- Definir qué ocurre si LittleFS no monta: usar un fallback no persistente y registrar claramente que la protección disminuye.

### BUG-009 — secretos en el repositorio

**Estado:** estructura preventiva integrada; exposición histórica no auditada.

Existe `secrets.h.template` y `secrets.h` está excluido por `.gitignore`. Esto no demuestra que nunca haya existido una clave en el historial Git.

Acción futura si se requiere auditoría:

```bash
git log --all -- secrets.h
 git log -S'IOT_AUTH_KEY' --all -- .
```

Si hubo exposición real, rotar la clave y no limitarse a borrar el archivo actual.

### BUG-010 — autenticación antes de ACK y deduplicación

**Estado:** pendiente; la documentación de “auth logic unificada” es incorrecta respecto al código actual.

En `IoTNode::_processIncoming()` se ejecutan, en esencia, estas operaciones antes del callback:

1. Deserializar.
2. Actualizar remoto.
3. Responder ACK si corresponde.
4. Comprobar duplicado.
5. Llamar al handler.

La verificación HMAC ocurre en callbacks de los firmwares. Por tanto, un paquete inválido puede producir efectos secundarios antes de ser rechazado.

Corrección prevista:

```text
recibir datagrama
→ deserializar y validar estructura/CRC
→ verificar autenticación
→ validar anti-replay/deduplicación
→ enviar ACK según la política definida
→ actualizar registry
→ despachar al handler
```

Debe decidirse si los paquetes inválidos reciben ACK. La recomendación inicial es no reconocer un paquete que no pasó autenticación, salvo que exista una razón de interoperabilidad documentada.

### BUG-011 — ArduinoJson en discovery

**Estado:** corregido en el código actual.

`receptor_bocina/src/mqtt_discovery.cpp` usa `doc.to<JsonObject>()`. La documentación histórica debe describirlo como fix aplicado, no como tarea pendiente.

Pendiente documental:

- Verificar que los mensajes retained llegan al broker.
- Confirmar que Home Assistant registra las entidades.
- Registrar payload y resultado en el changelog, sin incluir secretos.

---

## 5. Decisiones que deben tomarse antes de implementar

### 5.1 Política de autenticación

La autenticación actual es opcional mediante `authEnabled`. Antes de endurecerla hay que decidir:

- Si todos los nodos V4 deben usar la misma clave.
- Si la central puede aceptar temporalmente paquetes sin HMAC durante una migración.
- Cómo se provisiona una clave nueva.
- Qué ocurre cuando un dispositivo tiene una clave incorrecta.
- Si se requiere autenticación también para ACK, HELLO, heartbeat y STATE_REPORT.
- Si el receptor debe responder algo ante un paquete inválido.

Recomendación:

1. Mantener un modo opcional solo durante la migración.
2. Añadir logs genéricos sin revelar material sensible.
3. Hacer obligatorio HMAC en una configuración de producción explícita.
4. No afirmar que hay confidencialidad: HMAC autentica/integriza, pero no cifra.

### 5.2 Tamaño del HMAC

El código actual utiliza cuatro bytes. El diseño futuro propone ocho bytes.

Antes de cambiarlo:

- Confirmar el nivel de amenaza real.
- Confirmar el impacto en el payload máximo.
- Cambiar nombres engañosos como `AUTH_HMAC4` si el tamaño pasa a ocho.
- Definir compatibilidad entre nodos de cuatro y ocho bytes.
- Actualizar todos los tests, comentarios y documentación.

Recomendación para una versión nueva:

```cpp
#define IOT_TLV_AUTH_HMAC 0xF0
#define IOT_HMAC_TRUNC_SIZE 8
```

No reutilizar el nombre `IOT_TLV_AUTH_HMAC4` para ocho bytes salvo que exista una razón de compatibilidad documentada.

### 5.3 Anti-replay y paquetes fuera de orden

No implementar simplemente “rechazar toda secuencia menor o igual” sin decidir la semántica UDP.

Hay dos opciones:

**Opción A — orden estricto:**

- Rechaza cualquier secuencia vieja.
- Es simple.
- Puede rechazar paquetes legítimos que lleguen fuera de orden.

**Opción B — ventana anti-replay:**

- Mantiene una secuencia máxima y una bitmask/ventana.
- Acepta paquetes nuevos dentro de la ventana si no se han visto.
- Rechaza duplicados y paquetes demasiado antiguos.
- Es más adecuada para UDP, pero requiere más RAM y pruebas.

La opción recomendada para V4 es mantener una ventana explícita compatible con el diseño actual, asociada a `BOOT_ID` y dispositivo remoto.

### 5.4 Arquitectura V5

No comenzar una reescritura arquitectónica antes de una evaluación comparativa.

Se deben comparar como mínimo:

1. Protocolo propio actual con TLV.
2. CBOR.
3. MessagePack.
4. Protobuf/nanopb.
5. CoAP.
6. MQTT-SN.
7. UDP propio con DTLS u OSCORE.
8. Combinaciones: transporte estándar + modelo propio o viceversa.

La decisión debe considerar:

- RAM y flash del ESP8266.
- Tamaño de paquetes.
- Latencia.
- Facilidad de depuración.
- Seguridad disponible.
- Compatibilidad con Home Assistant.
- Dependencias de PlatformIO.
- Migración desde V4.
- Mantenimiento a largo plazo.

No asumir que una biblioteca propia es universal solo porque usa TLV. El código actual de `IoTNode` está acoplado a `WiFiUDP`, Arduino/ESP8266 y tipos de aplicación como `EventCode`.

---

## 6. Plan de ejecución ordenado

Las fases deben ejecutarse en este orden. No saltar directamente a nuevos sensores o cifrado antes de cerrar seguridad y pruebas básicas.

---

### Fase 0 — Preparación y línea base

**Objetivo:** conocer exactamente el estado antes de tocar V4.

#### Pasos

1. Crear una rama de trabajo:

   ```bash
   git status --short
   git switch -c future/v4-hardening
   ```

   Si hay cambios locales, no cambiar de rama hasta que el responsable decida cómo conservarlos.

2. Leer:

   ```text
   README.md
   docs/ARCHITECTURE.md
   docs/CHANGELOG.md
   docs/ROADMAP.md
   network_config.h
   lib/IoTProtocol/IoTProtocol.h
   lib/IoTProtocol/IoTNode.h
   lib/IoTProtocol/IoTNode.cpp
   lib/IoTProtocol/IoTAuth.h
   lib/IoTProtocol/IoTAuth.cpp
   lib/IoTProtocol/IoTStorage.h
   lib/IoTProtocol/IoTStorage.cpp
   emisor_pir_v4/src/main.cpp
   receptor_central_v4/src/main.cpp
   receptor_central_v4/src/event_handler.cpp
   ```

3. Buscar conexiones reales:

   ```bash
   grep -R "node.begin\|getBootId\|verifyPacket\|signPacket\|_processIncoming\|isDuplicate" -n \
     lib emisor_pir_v4 receptor_central_v4
   ```

4. Compilar la V3 de producción sin modificarla:

   ```bash
   pio run -d emisor_pir
   pio run -d receptor_bocina
   ```

5. Compilar los dos firmwares V4:

   ```bash
   pio run -d emisor_pir_v4
   pio run -d receptor_central_v4
   ```

6. Registrar en una tabla:

   - Board.
   - Versión del framework ESP8266.
   - Versión de PubSubClient.
   - Versión de ArduinoJson.
   - Tamaño de firmware.
   - RAM libre al arrancar.
   - Puerto UDP.
   - IP configurada.
   - Si auth está activa.

#### Criterios de aceptación

- V3 compila antes de comenzar.
- V4 compila antes de comenzar o queda registrado el fallo existente.
- No hay cambios funcionales en V3.
- Se conoce la versión exacta de las dependencias.

---

### Fase 1 — Crear infraestructura de pruebas host

**Objetivo:** poder validar serialización, CRC, TLV, deduplicación, cola y HMAC sin depender inmediatamente del hardware.

#### Estructura prevista

```text
tests/
├── platformio.ini
├── test_protocol.cpp
├── test_dedup.cpp
├── test_auth.cpp
└── test_queue.cpp
```

La configuración puede iniciar con:

```ini
[env:native_test]
platform = native
test_framework = unity
build_src_filter = -<*> +<../lib/IoTProtocol/>
```

La configuración exacta debe ajustarse a las dependencias reales de PlatformIO. No copiar ciegamente un ejemplo si la biblioteca incluye cabeceras específicas de ESP8266 que no compilan en host.

#### Tests mínimos

1. Serializar y deserializar un paquete válido.
2. Corromper un byte y comprobar fallo de CRC.
3. Usar longitud incorrecta y comprobar rechazo.
4. Usar una versión mayor incompatible y comprobar rechazo.
5. TLV con longitud mayor al payload restante y comprobar rechazo.
6. TLV `uint16` con longitud tres y comprobar rechazo.
7. Mismo `SEQ` y `BOOT_ID` detectado como duplicado.
8. Mismo `SEQ` con distinto `BOOT_ID` aceptado como nuevo arranque.
9. Insertar nueve secuencias y verificar la política de expulsión de la ventana.
10. Firmar y verificar HMAC con la misma clave.
11. Verificar HMAC con clave incorrecta y obtener `false`.
12. Verificar paquete sin auth con `_required=false` y `_required=true`.
13. Llenar la cola y verificar la política de overflow.
14. Confirmar que prioridad URGENT se despacha antes que BACKGROUND.
15. Confirmar que un payload en el límite máximo no desborda buffers.
16. Confirmar que un payload con TLV de auth malformado se rechaza.

#### Criterios de aceptación

- Los tests pueden ejecutarse con un solo comando.
- Cada fallo identifica la propiedad violada.
- No se usan credenciales reales.
- Los tests no dependen de WiFi físico.
- Los casos de límites de enteros y payload están cubiertos.

---

### Fase 2 — Integrar BOOT_ID persistente

**Objetivo:** que un reinicio legítimo no se confunda con una retransmisión del mismo dispositivo.

#### Archivos previstos

```text
lib/IoTProtocol/IoTNode.h
lib/IoTProtocol/IoTNode.cpp
emisor_pir_v4/src/main.cpp
receptor_central_v4/src/main.cpp
```

#### Diseño recomendado

Añadir una sobrecarga o reemplazo explícito:

```cpp
void begin();
void begin(uint16_t persistentBootId);
```

La implementación con argumento debe:

1. No volver a generar un valor aleatorio.
2. Rechazar o normalizar el valor cero según la regla del protocolo.
3. Guardar `_bootId` antes de empezar a enviar.
4. Reiniciar `_seq` de forma coherente con el modelo de deduplicación.
5. Abrir el puerto UDP como la implementación actual.

En cada firmware:

```cpp
uint16_t bootId = storage.getBootId();
node.begin(bootId);
```

No llamar a `storage.getBootId()` varias veces durante el mismo arranque: cada llamada incrementa el contador y podría crear identificadores inconsistentes.

#### Casos de error

- LittleFS no monta.
- Archivo de contador inexistente.
- Archivo de contador corrupto.
- Contador alcanza el límite de 16 bits.
- El valor calculado es cero.
- Se reinicia el dispositivo inmediatamente después de guardar.

#### Criterios de aceptación

- Dos arranques consecutivos con LittleFS producen BOOT_ID diferentes.
- El BOOT_ID no cambia durante un mismo arranque.
- El nodo registra el BOOT_ID usado.
- Un fallo de almacenamiento queda explícito en logs.
- La deduplicación acepta la primera secuencia de un nuevo BOOT_ID.
- Los tests cubren el wrap y el valor cero.

---

### Fase 3 — Reordenar la recepción para autenticar antes de producir efectos

**Objetivo:** impedir que paquetes no autenticados modifiquen registry, ACK, deduplicación o lógica de aplicación.

#### Orden recomendado de `_processIncoming()`

```text
1. parsePacket()
2. comprobar tamaño máximo
3. leer datagrama
4. deserializar
5. validar destino
6. verificar CRC y estructura
7. verificar autenticación
8. comprobar anti-replay/deduplicación
9. registrar remote si la política lo permite
10. enviar ACK válido
11. procesar HELLO/ACK
12. llamar al handler
```

La posición exacta de ACK para duplicados debe decidirse:

- Un duplicado válido y autenticado debe recibir ACK otra vez para detener retransmisiones.
- Un paquete inválido no debería recibir ACK.
- Un paquete autenticado pero duplicado no debe volver a disparar la aplicación.

#### API posible

Hay dos diseños aceptables:

**Diseño A — autenticación como callback de `IoTNode`:**

```cpp
using IoTPacketVerifier = bool (*)(const IoTPacket& pkt);
void onPacketVerifier(IoTPacketVerifier verifier);
```

`IoTNode` verifica antes de sus efectos internos.

**Diseño B — `IoTNode` recibe un objeto de seguridad:**

```cpp
void setAuth(IoTAuth* auth);
```

`IoTNode` posee solo una dependencia abstracta o bien una interfaz mínima, evitando conocer secretos directamente.

La decisión debe considerar el acoplamiento y el tamaño de firmware. No copiar una API del diseño histórico sin comprobar que coincide con las clases actuales.

#### Criterios de aceptación

- Un paquete con HMAC incorrecto no genera ACK.
- Un paquete sin HMAC se acepta o rechaza según `_required`.
- Un paquete inválido no actualiza `lastSeen` ni registry, salvo que se documente una política distinta.
- Un paquete válido duplica ACK, pero no duplica efectos de aplicación.
- Los callbacks ya no necesitan repetir la decisión de autenticación.
- Los tests comprueban el orden de efectos, no solo el valor final.

---

### Fase 4 — Decidir y aplicar anti-replay

**Objetivo:** proteger contra paquetes capturados y reproducidos, sin romper UDP fuera de orden legítimo.

#### Diseño recomendado

Extender `RemoteDevice` con información suficiente para el estado anti-replay, por ejemplo:

```cpp
uint32_t highestSeq;
uint32_t replayMask;
uint32_t lastUptime;
uint16_t bootId;
```

La estructura exacta puede variar. La regla debe ser:

- Si cambia `BOOT_ID`, iniciar una nueva ventana.
- Si la secuencia es más nueva, avanzar la ventana.
- Si está dentro de la ventana y ya se vio, rechazar como duplicado.
- Si está fuera de la ventana por antigua, rechazar como replay.
- Si llega fuera de orden pero dentro de la ventana y no se vio, aceptar.

El campo `UPTIME_SEC` puede servir como señal adicional, pero no debe sustituir al control de secuencia:

- Uptime menor con el mismo BOOT_ID puede indicar replay o rollover.
- Uptime menor con BOOT_ID nuevo puede ser un reinicio legítimo.
- Se puede permitir una tolerancia de aproximadamente cinco segundos por jitter, pero debe probarse.

#### No hacer

No implementar únicamente:

```cpp
if (seq <= lastSeq) reject;
```

sin documentar que se está imponiendo orden estricto. UDP puede entregar paquetes fuera de orden.

#### Pruebas

- Duplicado exacto.
- Paquete antiguo dentro de ventana.
- Paquete antiguo fuera de ventana.
- Paquete nuevo después de una secuencia atrasada.
- Paquete fuera de orden dentro de ventana.
- Reinicio con nuevo BOOT_ID.
- Rollover del contador.
- Cambio de dispositivo con el mismo `SEQ`.

#### Criterios de aceptación

- Un replay capturado fuera de ventana no activa la alarma.
- Un paquete nuevo de un reinicio legítimo sí se acepta.
- Un paquete fuera de orden válido no se descarta injustificadamente si se eligió ventana.
- La RAM usada por dispositivo está documentada.

---

### Fase 5 — Revisar HMAC y compatibilidad

**Objetivo:** implementar una política de autenticación coherente y medible.

#### Tareas

1. Decidir si el tamaño sigue siendo cuatro bytes o pasa a ocho.
2. Si pasa a ocho:
   - Cambiar el nombre del tag a uno que no diga HMAC4.
   - Actualizar comentarios.
   - Actualizar capacidad máxima de payload.
   - Actualizar todos los tests.
3. Asegurar que el HMAC se calcula sobre exactamente:

   ```text
   version + type + src + dst + bootId + seq + flags sin auth + payload sin TLV auth
   ```

4. Verificar que el TLV de auth no pueda aparecer varias veces de forma ambigua.
5. Rechazar el paquete si el flag indica auth pero el TLV falta.
6. Rechazar el paquete si el TLV tiene longitud incorrecta.
7. Mantener comparación en tiempo constante.
8. Definir la política para paquetes sin auth cuando `_required` es falso.
9. Confirmar que ACK, HELLO, heartbeat y STATE_REPORT se firman cuando la política lo exige.
10. No guardar la clave en `IoTStorage` si ya existe una provisión segura en `secrets.h`, salvo que se diseñe explícitamente el mecanismo de almacenamiento.

#### Criterios de aceptación

- Firma y verificación coinciden entre emisor y central.
- Un byte modificado en header o payload invalida el HMAC.
- Una clave incorrecta invalida el paquete.
- Reordenar TLV después de firmar invalida el paquete si el orden forma parte del payload.
- No aparecen claves en el binario de logs, documentación ni repositorio.
- Existe una estrategia de rotación de claves documentada.

---

### Fase 6 — Validar V4 con simulador UDP

**Objetivo:** probar el receptor y el protocolo sin tener todos los ESP8266 conectados.

#### Archivos previstos

```text
tools/iot_simulator.py
tools/README.md
```

#### Funcionalidad mínima

El simulador debe:

- Serializar el wire format V4.
- Calcular CRC16-CCITT igual que C++.
- Enviar datagramas UDP al puerto configurado.
- Escuchar ACK.
- Simular `MOTION`, `TIMBRE`, temperatura y heartbeat.
- Aceptar un BOOT_ID y una secuencia configurables.
- Usar HMAC cuando se solicite, sin mostrar la clave.

Opciones previstas:

```text
--device PIR01
--event MOTION
--loss 20
--delay 50
--duplicate
--corrupt
--replay
--heartbeat
```

El nombre exacto de los argumentos puede cambiar, pero debe documentarse en `tools/README.md`.

#### Escenarios

1. Paquete válido.
2. CRC corrupto.
3. Longitud incorrecta.
4. Duplicado.
5. Paquete fuera de orden.
6. Replay antiguo.
7. HMAC inválido.
8. HMAC ausente con auth requerida.
9. Pérdida de paquetes.
10. Retransmisión y ACK repetido.
11. Dos sensores simultáneos.
12. Reinicio con BOOT_ID nuevo.
13. Heartbeat y transición ONLINE → STALE → OFFLINE.

#### Criterios de aceptación

- El receptor no se bloquea mientras recibe una ráfaga.
- Un paquete corrupto no dispara la alarma.
- Un duplicado produce como máximo un efecto de aplicación.
- Un ACK válido detiene la retransmisión.
- El simulador puede reproducir un caso fallido con un comando documentado.

---

### Fase 7 — Validar hardware V4

**Objetivo:** confirmar que las garantías de host se mantienen en ESP8266 real.

#### Pruebas del emisor

- Arranque con LittleFS vacío.
- Reinicio normal.
- Reinicio durante transmisión.
- Pérdida temporal de WiFi.
- PIR sostenido en HIGH.
- PIR y timbre simultáneos.
- Cola llena.
- Broker MQTT apagado.
- Auth activada y desactivada.
- Clave incorrecta.

#### Pruebas de la central

- Arranque sin broker.
- Broker disponible después del arranque.
- Broker que desaparece durante una alarma.
- Ráfaga de ocho o más datagramas.
- Paquetes duplicados.
- Paquetes con CRC incorrecto.
- Paquetes con HMAC inválido.
- Transiciones de estado remoto.
- OTA solo después de verificar firewall y estabilidad.

#### Métricas a registrar

- Tiempo de cada iteración del loop.
- Tiempo máximo de procesamiento de un paquete.
- RTT mínimo, máximo y promedio.
- Número de retransmisiones.
- Memoria libre antes y después de activar auth.
- Tamaño final del firmware.
- Pérdida de eventos bajo carga.

#### Criterios de aceptación

- El loop no supera el límite acordado durante operación normal.
- V3 sigue funcionando sin depender de V4.
- No hay watchdog reset en las pruebas previstas.
- Los resultados están registrados y no solo observados informalmente.

---

### Fase 8 — Mejorar la sirena V3

**Objetivo:** diferenciar el patrón de MOTION de la señal corta de TIMBRE sin bloquear la recepción.

Esta fase es independiente de la seguridad V4 y debe conservar V3 como producción.

#### Diseño propuesto

Extender `Buzzer` con un patrón no bloqueante:

```cpp
void sirenOn(unsigned long onMs, unsigned long offMs, uint8_t cycles);
bool isBusy() const;
```

Campos posibles:

```cpp
bool _patternActive;
bool _outputState;
unsigned long _phaseDeadline;
unsigned long _onMs;
unsigned long _offMs;
uint8_t _cyclesRemaining;
```

La función `loop()` debe avanzar entre fases usando comparación segura de `millis()`:

```cpp
if ((long)(millis() - _phaseDeadline) >= 0) {
    // cambiar de fase
}
```

No usar `millis() >= deadline` como única comparación si se quiere manejar rollover correctamente.

#### Patrón propuesto

- `MOTION`: cuatro segundos totales, alternando aproximadamente 200 ms encendido y 200 ms apagado.
- `TIMBRE`: señal continua corta de 500 ms mediante `timedOn(500)`.
- Los valores deben quedar en configuración, no duplicados en varios archivos.

#### Decisiones que deben cerrarse

1. Si TIMBRE interrumpe una sirena MOTION.
2. Si una nueva MOTION reinicia o extiende el patrón.
3. Qué ocurre con `OFF` manual durante una sirena.
4. Si MQTT publica el estado durante la fase apagada del patrón.
5. Cómo se interpreta `isBusy()` frente a `isOn()`.
6. Qué prioridad tienen eventos SMOKE/FLOOD si se añaden.

#### Cambios probables

```text
receptor_bocina/include/hal.h
receptor_bocina/src/hal.cpp
receptor_bocina/include/config.h
receptor_bocina/src/config.cpp
receptor_bocina/src/alarma.cpp
receptor_bocina/src/mqtt_cliente.cpp
```

#### Criterios de aceptación

- El patrón no usa `delay()`.
- UDP sigue procesándose durante todas las fases.
- TIMBRE no se pierde mientras MOTION está activo.
- El estado manual no queda atascado.
- El comportamiento tras rollover de `millis()` está probado.

---

### Fase 9 — Documentación y consistencia

**Objetivo:** que el proyecto no vuelva a depender de memoria externa o borradores sin versionar.

#### Correcciones documentales

1. Crear `docs/BUGS_FIXED.md` si se decide conservar un registro histórico separado.
2. Corregir el enlace del README si el documento tendrá otro nombre.
3. Actualizar `docs/CHANGELOG.md` solo con correcciones verificadas.
4. Mantener `docs/ARCHITECTURE.md` alineado con el código.
5. Marcar claramente V3 como producción y V4 como desarrollo hasta que exista una decisión de promoción.
6. Documentar cualquier cambio de wire format con versión mayor.
7. Añadir al README una referencia a este plan:

   ```markdown
   [Plan de ejecución futura](docs/PLAN_EJECUCION_FUTURA.md)
   ```

8. No documentar B622, DMZ, IPs nuevas o comportamiento de hardware sin verificación.

#### Criterios de aceptación

- Todos los enlaces del README existen.
- Las versiones documentadas coinciden con el código.
- Cada tarea futura tiene estado: `pendiente`, `en progreso`, `verificada` o `rechazada`.
- No hay una afirmación de “implementado” sin evidencia.

---

## 7. Trabajo posterior a la estabilización de V4

Estas tareas no deben adelantarse a las fases de seguridad y pruebas.

### 7.1 Tests permanentes y CI

Cuando los tests host existan:

1. Ejecutarlos en cada cambio de `lib/IoTProtocol`.
2. Compilar V3 y V4.
3. Ejecutar `git diff --check`.
4. Considerar una comprobación de tamaño de firmware y RAM.
5. No permitir que un cambio de biblioteca rompa V3 sin una decisión explícita.

### 7.2 Nuevo sensor de temperatura DHT22

Crear:

```text
emisor_temp_v4/
├── platformio.ini
├── include/device_config.h
├── src/device_config.cpp
└── src/main.cpp
```

Propuesta:

- D1 Mini.
- DHT22 en D4.
- ID `0x40`.
- Tipo `TEMP_SENSOR`.
- Lectura cada 30 segundos.
- `MsgType::DATA`.
- Temperatura como entero escalado por diez.
- Humedad como entero escalado por diez.
- Heartbeat cada 60 segundos.
- HELLO al arrancar.
- Datos de baja prioridad si no requieren ACK.

Antes de afirmar que el receptor no necesita cambios, comprobar que `event_handler.cpp` realmente procesa los TLV de temperatura y humedad.

### 7.3 Sensor de puerta

Crear `emisor_puerta_v4/`.

Propuesta:

- Reed switch en D2 con `INPUT_PULLUP`.
- Cerrado = LOW.
- Abierto = HIGH.
- `DOOR_OPEN` y `DOOR_CLOSE`.
- Eventos reliable con ACK.
- `STATE_REPORT` al arrancar.
- Deep sleep solo como optimización posterior, no en la primera implementación.

### 7.4 Actuador de relé

Crear `actuador_relay_v4/`.

Propuesta:

- Relé en D1.
- Tipo `RELAY`.
- Recibir `COMMAND` con `CMD_STATE`.
- Soportar ON, OFF y posiblemente TOGGLE.
- Responder `RESPONSE` con código de resultado, estado actual e ID de comando.
- Responder `STATE_REQUEST`.
- La central debe generar `COMMAND` con un `CMD_ID` único.
- Agregar MQTT `casa/iot/device_60/set` solo después de validar autorización y autenticación.

### 7.5 DHCP y discovery real

Objetivo: reducir la dependencia de IPs estáticas.

Propuesta:

- Emisores con DHCP.
- Central con IP fija o mecanismo de descubrimiento controlado.
- HELLO con IP y puerto observados.
- Registry actualizado desde HELLO y heartbeat.
- No depender exclusivamente de mDNS: en Windows y algunas redes `.local` puede ser inestable.
- Mantener la IP de la central en configuración persistente si no existe un mecanismo fiable de descubrimiento.

### 7.6 OTA distribuido

No implementarlo hasta que haya suficientes nodos para justificar la complejidad.

Diseño posible:

1. La central almacena el firmware.
2. Envía una orden OTA autenticada.
3. El nodo descarga por HTTP desde una fuente conocida.
4. Verifica hash y tamaño.
5. Aplica el firmware de forma segura.
6. Reinicia y reporta resultado.
7. Si el boot falla, debe existir rollback real o una estrategia de recuperación física.

No considerar “descargar y reiniciar” como OTA seguro sin verificar integridad y recuperación.

### 7.7 Cifrado

Solo necesario si existe un requisito real de confidencialidad, por ejemplo datos sensibles o actuadores de alto riesgo.

La propuesta histórica era AES-128-GCM con:

- Nonce de 12 bytes.
- Tag de autenticación de 16 bytes.
- Overhead aproximado de 28 bytes.
- Flag `IOT_FLAG_ENCRYPTED`.

El diseño de nonce debe analizarse cuidadosamente: `BOOT_ID + SEQ + random` solo es seguro si no hay repetición y si el generador aleatorio tiene propiedades suficientes. No afirmar que un nonce es único sin demostrarlo.

Con un payload máximo de 64 bytes, el overhead puede dejar demasiado poco espacio para TLV. Antes de implementarlo se debe decidir si se amplía el paquete, se reduce el modelo o se cambia de protocolo.

### 7.8 Persistencia de estado del receptor

Persistir en LittleFS, con escritura limitada para no desgastar flash:

- ID.
- Tipo.
- Nombre.
- Último estado.
- Último uptime.
- Último BOOT_ID.

Escribir por lotes, por ejemplo cada cinco minutos o al cambiar información estable, no en cada paquete.

### 7.9 Alertas de dispositivo OFFLINE

Puede resolverse en Home Assistant sin tocar firmware:

```yaml
automation:
  - alias: "Alerta sensor offline"
    trigger:
      - platform: state
        entity_id: sensor.device_02_status
        to: "offline"
        for: "00:03:00"
    action:
      - service: notify.mobile_app
        data:
          message: "PIR Entrada está offline hace 3 minutos"
```

Los nombres de entidades y servicios deben adaptarse a la instalación real.

### 7.10 Discovery MQTT dinámico

Cuando la central reciba un HELLO nuevo:

- Generar configuración MQTT Discovery.
- Incluir nombre, tipo, state topic y availability.
- Publicar retained.
- Evitar colisiones de `unique_id`.
- No asumir que el discovery V3 se puede copiar literalmente a V4.

### 7.11 Modo de prueba para PIR

Opciones:

- Segundo pin de test, por ejemplo D7, con antirrebote de 100 ms.
- Comando CONFIG autenticado que active temporalmente modo de prueba.

El pin físico es más simple. El comando remoto es más flexible, pero requiere autorización, expiración y protección contra activaciones accidentales.

---

## 8. Evaluación de arquitectura V5

Solo iniciar esta sección cuando V4 tenga tests y seguridad básica.

### 8.1 Objetivo

Separar claramente:

```text
aplicación
  ↓
modelo de mensajes
  ↓
seguridad
  ↓
serialización
  ↓
transporte
  ↓
red física
```

El diseño actual tiene acoplamiento entre `IoTNode`, `WiFiUDP`, Arduino y tipos de aplicación. Una futura arquitectura puede introducir interfaces como:

```cpp
class ITransport {
public:
    virtual bool send(const uint8_t* data, size_t len, const Endpoint& dst) = 0;
    virtual int receive(uint8_t* data, size_t capacity, Endpoint& src) = 0;
};

class ISecurity {
public:
    virtual bool sign(Packet& packet) = 0;
    virtual bool verify(const Packet& packet) = 0;
};
```

Estas son ideas de diseño, no APIs aprobadas. Antes de crearlas hay que medir el impacto en flash, RAM y complejidad.

### 8.2 Requisitos de la decisión

La decisión V5 debe producir:

- Al menos tres arquitecturas comparadas.
- Matriz de ventajas, riesgos, coste y compatibilidad.
- Decisión explícita.
- Plan de migración V4 → V5.
- Estrategia de interoperabilidad.
- Plan para mantener V3 mientras se migra.
- Criterios para cancelar la reescritura si el beneficio no compensa.

### 8.3 Posibles componentes

Evaluar, no asumir:

- Modelo de eventos.
- HELLO y capabilities.
- HEARTBEAT.
- EVENT.
- ACK.
- COMMAND.
- CONFIG.
- RESPONSE.
- OTA.
- ERROR.
- Versionado.
- Transportes intercambiables.
- Seguridad modular.
- Storage opcional.
- Profiles de dispositivo.
- Logging estructurado.

---

## 9. Matriz de trazabilidad del conocimiento consolidado

| Conocimiento | Estado actual | Fase futura | Evidencia requerida |
|---|---|---|---|
| ACK asíncrono V3 | Integrado | Fase 0/7 | Prueba de pérdida y retransmisión |
| Drain loop V3 | Integrado | Fase 0/7 | Ráfaga de paquetes |
| PIR/TIMBRE independientes | Integrado en diseño | Fase 7 | Activación simultánea |
| Modo LOCAL/HA | Integrado parcialmente probado | Fase 7 | Broker caído y recuperado |
| Timeout TCP de ESP8266 | Regla preventiva | Fase 0 | Medición, sin reintroducirlo |
| Fix `doc.to<JsonObject>()` | Integrado | Fase 9 | Retained MQTT + Home Assistant |
| BOOT_ID persistente | Pendiente | Fase 2 | Reinicios y LittleFS |
| Auth antes de ACK/dedup | Pendiente | Fase 3 | Paquete HMAC inválido |
| HMAC de 8 bytes | Decisión pendiente | Fase 5 | Compatibilidad y tests |
| Anti-replay | Pendiente | Fase 4 | Replay y fuera de orden |
| Tests host | Pendiente | Fase 1 | `pio test` reproducible |
| Simulador UDP | Pendiente | Fase 6 | Casos reproducibles |
| Sirena con patrón | Pendiente | Fase 8 | MOTION/TIMBRE simultáneos |
| DHT22 | Futuro | Después de Fase 7 | DATA y heartbeat |
| Reed switch | Futuro | Después de Fase 7 | OPEN/CLOSE y state report |
| Relé | Futuro | Después de Fase 7 | COMMAND/RESPONSE |
| DHCP/discovery | Futuro | Después de V4 estable | Cambio de IP |
| OTA distribuido | Futuro lejano | Cuando haya necesidad | Hash, rollback |
| AES-GCM | Condicional | Solo requisito de confidencialidad | Modelo de nonce y payload |
| Persistencia de estado | Futuro | Después de estabilidad | Reinicio de central |
| Alertas HA offline | Futuro | Independiente del firmware | Automatización HA |
| Dashboard dinámico | Futuro | Después de HELLO estable | MQTT Discovery |
| Arquitectura V5 | Decisión futura | Después de tests | Matriz comparativa |

---

## 10. Checklist de finalización de una fase

Una fase no está terminada solo porque el código compile. Marcarla como terminada únicamente cuando se cumpla todo lo correspondiente:

- [ ] Se leyó el código actual antes de editar.
- [ ] Se documentó la decisión tomada.
- [ ] Se implementó el cambio mínimo necesario.
- [ ] Se preservó V3 si el cambio era de V4.
- [ ] Se añadieron o actualizaron tests.
- [ ] Se ejecutaron los tests.
- [ ] Se compiló el firmware afectado.
- [ ] Se revisaron warnings relevantes.
- [ ] Se comprobó `git diff --check`.
- [ ] Se midieron RAM, flash o tiempo de loop si aplica.
- [ ] Se actualizaron changelog y documentación.
- [ ] Se registraron limitaciones y casos no probados.
- [ ] Se verificó que no hay secretos en los cambios.

### Formato de registro recomendado

```markdown
## Fase X — estado: VERIFICADA

Fecha:
Rama:
Archivos modificados:
Comandos ejecutados:
Pruebas realizadas:
Resultado:
Limitaciones:
Siguiente fase:
```

---

## 11. Comandos de referencia

### Estado y revisión

```bash
git status --short
git diff --check
git diff --stat
git log -5 --oneline
```

### Compilación V3

```bash
pio run -d emisor_pir
pio run -d receptor_bocina
```

### Compilación V4

```bash
pio run -d emisor_pir_v4
pio run -d receptor_central_v4
```

### Tests futuros

```bash
pio test -e native_test
```

### Aplicación segura de un patch futuro

Si en el futuro se genera un patch externo, nunca aplicarlo directamente:

```bash
git apply --check archivo.patch
```

Solo si la comprobación pasa, revisar:

```bash
git diff --stat
git diff
```

y después aplicar:

```bash
git apply archivo.patch
```

El patch debe ser texto diff raw, no un bloque Markdown con fences. Debe generarse contra el commit o estado exacto que se está usando.

### Auditoría de secretos histórica

```bash
git log --all -- secrets.h
git log -S'IOT_AUTH_KEY' --all -- .
```

No pegar en una issue, log o respuesta pública ninguna clave encontrada. Si se detecta exposición, rotarla.

---

## 12. Definición de “terminado” para el proyecto futuro

El proyecto no debe declararse estabilizado hasta cumplir estos criterios:

1. V3.5.1 sigue compilando y funcionando de forma independiente.
2. V4 compila en emisor y central.
3. Existen tests host para serialización, CRC, TLV, deduplicación, cola y auth.
4. BOOT_ID persistente está conectado y probado.
5. La autenticación se evalúa antes de los efectos internos.
6. La política anti-replay está definida y probada con UDP fuera de orden.
7. El HMAC y su compatibilidad están documentados.
8. Existe un simulador o procedimiento reproducible para paquetes válidos, corruptos, duplicados y replay.
9. Las pruebas hardware no muestran bloqueos del loop ni watchdog resets no explicados.
10. La documentación coincide con el código.
11. No existen secretos versionados.
12. Las futuras funcionalidades —sensores, relé, OTA, cifrado— están separadas de la estabilización básica.
13. Toda decisión de V5 se basa en una comparación, no en una preferencia previa.

La siguiente LLM debe preferir una implementación pequeña, verificable y reversible antes que una reescritura amplia. Si el código actual contradice este plan, debe informar la discrepancia, leer el código, ajustar el plan con evidencia y no inventar que la fase ya fue completada.



---

## 13. Segunda auditoría histórica con `unificador-skill`

**Fecha de auditoría:** 2026-08-30
**Objetivo:** comprobar que este plan no perdió información relevante de los 11 drafts históricos eliminados del árbol de trabajo.

Los drafts fueron recuperados desde el commit padre mediante un worktree temporal de solo lectura. No se aplicó ningún patch, no se modificó el firmware y no se ejecutaron compilaciones ni pruebas hardware durante esta auditoría.

### 13.1 ANÁLISIS PREVIO DE FRAGMENTOS

```text
Material recibido: 11 archivos históricos en _drafts/
Tamaño: 6.179 líneas y 128.633 bytes; conjunto largo (>30K)
Fuentes: notas técnicas, diagnósticos, meta-prompts, instrucciones y patch de seguridad
Documento canónico comparado: este archivo, 1.425 líneas antes de esta auditoría
Tema central: preservar V3.5.1, estabilizar V4.3 y decidir posteriormente una arquitectura V5

Duplicaciones detectadas:
- BUGS_FIXED.md y bugs.md repiten BUG-001..BUG-010 y amplían BUG-011.
- ideas.md, prodoco.md, prompt.md y prompt2.md repiten la visión de V5,
  perfiles, transportes y seguridad.
- META_PROMPT.md, plantilla de prompt.md, prompt.md y prompt2.md son variantes
  de meta-proceso.
- instrucciones.md y v4.3.1-security.patch describen el mismo cambio de seguridad.
- 1mejoras.md duplica la propuesta de la sirena de la Fase 8.

Contradicciones detectadas:
- BUGS_FIXED.md declara BOOT_ID y auth resueltos; el código y este plan los marcan pendientes.
- El patch pasa HMAC de 4 a 8 bytes pero conserva nombres HMAC4 y no define compatibilidad.
- El patch rechaza secuencias no crecientes; este plan exige decidir si UDP debe aceptar
  paquetes fuera de orden mediante una ventana.
- bugs.md presenta B622/DMZ e IPs concretas como hechos; este plan las considera hipótesis
  externas no verificadas.
- instrucciones.md propone commit/push y aplicación del patch; este plan exige revisión,
  pruebas y aprobación antes de publicar.
- El patch contiene fences Markdown e índices 0000000, por lo que no es un diff aplicable.

Brechas encontradas:
- No hay resultados de compilación, tests host, pruebas hardware ni métricas de loop.
- No hay evidencia de retained MQTT/Home Assistant verificado.
- No hay auditoría histórica real de secretos.
- No hay base de commit válida para aplicar el patch.
- Falta cerrar la rotación/provisión de claves, el fallback de LittleFS, la política de ACK
  para paquetes inválidos y la semántica de interrupción/reinicio de la sirena.

Valores que deben parametrizarse o confirmarse:
- Duraciones 4.000/200/200/500 ms.
- HMAC de 4 u 8 bytes.
- UDP 4210 y MQTT 1883.
- IPs 192.168.0.200, 192.168.0.201, 192.168.0.15 y 192.168.1.200.
- Firewall 1024-65535.
- IDs, topics MQTT, nombres de entidades y opciones del simulador.

Orden confirmado:
V3/línea base → tests host → BOOT_ID → auth antes de efectos → anti-replay
→ HMAC/compatibilidad → simulador → hardware → sirena V3 → documentación
→ sensores/relé/OTA/cifrado → evaluación V5.

Acción: consolidación aceptada, sin aplicar el patch ni declarar resueltos los bugs
que no tengan evidencia en código, compilación o hardware.
```

### 13.2 Matriz de cobertura por draft

| Draft histórico | Cobertura en este plan | Resultado de la auditoría |
|---|---|---|
| `1mejoras.md` | Fase 8 y matriz de trazabilidad | **PARCIAL**: se conserva la sirena no bloqueante y sus decisiones; la auditoría exige mantener comparación segura de `millis()` y no aplicar bloques históricos sin revisar. |
| `BUGS_FIXED.md` | Sección 4, reglas técnicas y fases 2–5 | **CONTRADICTORIO como estado histórico**: se conservan causas y reglas, pero BUG-008 y BUG-010 siguen pendientes hasta probarse. |
| `META_PROMPT.md` | Reglas de uso, documentación y checklist | **FUERA DE ALCANCE técnico**: queda representado como disciplina documental, no como evidencia de firmware. |
| `bugs.md` | BUG-011, Fase 9 y reglas de red | **PARCIAL/CONTRADICTORIO**: el fix `to<JsonObject>()` está cubierto; la topología B622/DMZ queda explícitamente no verificada. |
| `ideas.md` | Sección 7 y evaluación V5 | **PARCIAL**: seguridad, tests, OTA, capabilities, telemetría y expansión están cubiertos; zonas, máquina de estados ampliada y registro de eventos quedan como backlog pendiente. |
| `instrucciones.md` | Comandos de validación y aplicación segura de patch | **CONTRADICTORIO como automatismo**: se conserva `git apply --check`, diff y compilación; commit/push requiere revisión humana. |
| `plantilla de prompt.md` | Principios de no inventar, requisitos y aceptación | **FUERA DE ALCANCE técnico**: no es una decisión específica del proyecto. |
| `prodoco.md` | Sección 8 sobre V5 | **PARCIAL**: sus interfaces son candidatos, no APIs aprobadas; primero debe compararse con estándares. |
| `prompt.md` | Fases de auditoría, seguridad, migración y pruebas | **PARCIAL**: se conserva el alcance técnico, pero no se adopta como mandato de crear protocolo propio. |
| `prompt2.md` | Sección 8 y matriz V5 | **CUBIERTO como alcance**: la investigación debe poder concluir que no conviene un protocolo propio. |
| `v4.3.1-security.patch` | Fases 2–5 y comandos de patch | **CONTRADICTORIO/NO APLICABLE**: es una propuesta de cambios, no una implementación válida. Debe regenerarse desde el código real. |

### 13.3 Información accionable conservada

El plan conserva la información operativa relevante de los drafts:

- La prohibición de usar `connect()` como prueba rápida en la ruta crítica.
- La independencia de PIR y TIMBRE, cola asíncrona y drain loop.
- Las reglas sobre `LOCAL`, PIR sostenido en `HIGH`, OTA y secretos.
- El fix de ArduinoJson y la necesidad de verificar el payload retained real.
- BOOT_ID persistente, fallback de LittleFS, auth antes de ACK/deduplicación y anti-replay.
- La diferencia entre HMAC de cuatro y ocho bytes.
- La propuesta no bloqueante de la sirena y las decisiones de concurrencia.
- Tests host, simulador UDP, pruebas hardware, sensores, relé, DHCP, OTA, cifrado y V5.
- La separación entre propuestas, código aplicado, código compilado y funcionalidad verificada.

### 13.4 Estados permitidos para futuras tareas

A partir de esta auditoría, no usar solamente “resuelto”. Cada tarea debe tener uno de estos estados:

1. **PROPUESTO:** aparece como idea o diseño, pero no está implementado.
2. **APLICADO:** existe un cambio en el árbol de trabajo.
3. **COMPILADO:** el cambio aplicado compila en el entorno correspondiente.
4. **VERIFICADO:** además de compilar, pasó la prueba funcional definida.
5. **VERIFICADO EN HARDWARE:** la prueba funcional se confirmó en el ESP8266 y entorno real.
6. **RECHAZADO:** no se adopta, con motivo documentado.
7. **FUERA DE ALCANCE:** es una plantilla o procedimiento que no forma parte del firmware.

Una entrada histórica, un changelog o un patch nunca demuestra por sí solo los estados `COMPILADO`, `VERIFICADO` o `VERIFICADO EN HARDWARE`.

### 13.5 Decisiones bloqueantes agregadas por la auditoría

Antes de aplicar una futura corrección de seguridad:

- Decidir HMAC de 4 frente a 8 bytes.
- Si cambia a 8 bytes, renombrar el tag para no conservar un nombre `HMAC4` engañoso.
- Definir interoperabilidad entre nodos antiguos y nuevos.
- Elegir orden estricto o ventana anti-replay para UDP; no aceptar silenciosamente una implementación `seq <= lastSeq` como equivalente a una ventana.
- Definir qué ocurre si LittleFS no monta o el contador está corrupto.
- Definir si un paquete inválido recibe ACK.
- Probar el orden de efectos: parseo/CRC → auth → anti-replay → registry → ACK → callback.
- Regenerar el patch contra un commit real, sin fences Markdown ni índices ficticios.

### 13.6 Verificaciones MQTT y firewall conservadas como procedimientos condicionados

Estos comandos son referencias históricas y deben ejecutarse únicamente con placeholders sustituidos localmente, sin revelar contraseñas:

```bash
docker run --rm --network host eclipse-mosquitto mosquitto_sub \
  -h <IP_BROKER> -p 1883 -u <user> -P <pass> \
  -t 'homeassistant/#' -v --retained-only
```

La aceptación de BUG-011 requiere comprobar que el payload retained es JSON válido y que Home Assistant registra las entidades; un log de `publish()` no basta.

Reglas históricas para OTA en Windows, solo si se confirma que son necesarias en la red real:

```powershell
New-NetFirewallRule -DisplayName "PlatformIO OTA" -Direction Inbound -Protocol UDP -LocalPort 1024-65535 -Action Allow
New-NetFirewallRule -DisplayName "PlatformIO OTA TCP" -Direction Inbound -Protocol TCP -LocalPort 1024-65535 -Action Allow
```

### 13.7 Backlog explícito pendiente de clasificación

Las siguientes ideas aparecieron en los drafts y se desarrollan con detalle en la sección 14. No deben considerarse implementadas solo porque estén descritas:

- Máquina de estados de alarma más completa.
- Zonas de sensores.
- Registro persistente de eventos.
- Telemetría y dashboard ampliado.
- Capabilities genéricas.
- Profiles de aplicación.
- Rollback OTA.

Antes de implementarlas, asignar a cada una prioridad, dependencia, archivos afectados, coste de RAM/flash, criterio de aceptación y clasificación: post-V4, V5 o descartada.

### 13.8 Resultado de la comparación

La auditoría no encontró una pérdida crítica de conocimiento técnico en el plan. Sí encontró una necesidad de hacer explícita la trazabilidad histórica y de separar mejor:

```text
propuesta → aplicada → compilada → verificada → verificada en hardware
```

Por eso esta sección se incorpora al documento canónico. El patch histórico no se recupera como implementación; queda registrado como propuesta rechazada hasta ser regenerado y validado. La topología B622/DMZ tampoco se convierte en hecho del proyecto.



---

## 14. Desarrollo detallado de las ideas de mejora

Esta sección conserva el contenido estratégico de `ideas.md` con suficiente detalle para que una LLM futura pueda desarrollar cada idea sin depender de la conversación original ni de los drafts eliminados.

### 14.1 Prioridad general del proyecto

Antes de añadir sensores, dashboards o funciones de conveniencia, el sistema debe seguir esta cadena de madurez:

```text
¿Puede detectar?
      ↓
¿Puede comunicar?
      ↓
¿Puede recuperarse de un fallo?
      ↓
¿Puede impedir comandos falsos?
      ↓
recién entonces → nuevas funciones
```

Orden estratégico original:

1. Seguridad y confiabilidad.
2. Estado completo de cada dispositivo.
3. Registro de eventos.
4. Máquina de estados de alarma.
5. Zonas.
6. Configuración centralizada.
7. OTA controlado y rollback.
8. Añadir sensores sin crear protocolos diferentes.
9. Capabilities.
10. Watchdog y recuperación.
11. Telemetría.
12. Panel de diagnóstico.

Este orden estratégico debe combinarse con el orden técnico de las fases 0–9. Si existe conflicto, no saltar la línea base, los tests o la seguridad para adelantar una función visual.

### 14.2 Regla de clasificación por capa

Antes de implementar cualquier funcionalidad nueva, clasificarla en una o más de estas capas:

```text
PROTOCOLO
  Formato de mensajes, TLV, tipos, flags, ACK, versionado y seguridad del wire format.

DISPOSITIVO
  Lectura de sensores, actuadores, watchdog local, WiFi, almacenamiento y firmware.

CENTRAL
  Registry, estados, deduplicación, máquina de alarma, zonas, eventos y comandos.

HOME ASSISTANT / MQTT
  Discovery, topics, entidades, automatizaciones, dashboard y notificaciones.
```

Una funcionalidad no debe implementarse en la capa equivocada:

- Un sensor nuevo no debe crear un protocolo paralelo si puede usar `IoTProtocol`.
- Una regla de alarma no debe quedar codificada únicamente en Home Assistant si la sirena local debe continuar funcionando sin MQTT.
- Un secreto no debe viajar como configuración sin autenticación.
- Una capacidad del dispositivo debe anunciarse desde el dispositivo/protocolo; la central puede publicarla después a MQTT.
- Un dashboard no debe convertirse en la fuente de verdad del estado de seguridad.

Cada propuesta futura debe registrar:

```text
Capa principal:
Capas secundarias:
Mensajes afectados:
Persistencia necesaria:
Dependencia de MQTT:
Dependencia de hardware:
```

---

### 14.3 Idea 1 — Seguridad y confiabilidad primero

**Objetivo:** asegurar que el sistema detecte eventos, los comunique, se recupere de fallos y rechace comandos falsos antes de añadir nuevas funciones.

#### Requisitos

- HMAC para todos los mensajes importantes.
- Protección contra replay.
- `BOOT_ID` persistente.
- Contador `SEQ` robusto.
- Autenticación obligatoria para comandos.
- Credenciales fuera del repositorio.
- OTA protegido.
- Watchdog para recuperar bloqueos.
- Recuperación automática de WiFi y MQTT.
- Registro de eventos importantes.

#### Flujo de confiabilidad

```text
sensor detecta evento
      ↓
emisor genera mensaje
      ↓
mensaje firmado y con identidad de sesión
      ↓
UDP transmite sin bloquear el loop
      ↓
central valida estructura, CRC, auth y anti-replay
      ↓
central procesa una sola vez y responde ACK válido
      ↓
central activa acción local
      ↓
MQTT se publica si está disponible
      ↓
si una parte falla, las demás continúan
```

#### Fallos que deben aislarse

```text
WiFi perdido
    ↓
reintentar sin bloquear la alarma

MQTT perdido
    ↓
continuar en modo local y reintentar con backoff

UDP con pérdida
    ↓
retransmitir eventos reliable sin bloquear sensores

loop bloqueado
    ↓
watchdog
    ↓
reinicio

reinicio
    ↓
BOOT_ID nuevo
    ↓
HELLO/heartbeat
    ↓
ONLINE
```

#### Archivos relacionados

```text
lib/IoTProtocol/IoTAuth.h
lib/IoTProtocol/IoTAuth.cpp
lib/IoTProtocol/IoTNode.h
lib/IoTProtocol/IoTNode.cpp
lib/IoTProtocol/IoTStorage.h
lib/IoTProtocol/IoTStorage.cpp
emisor_pir_v4/src/main.cpp
receptor_central_v4/src/main.cpp
receptor_bocina/src/mqtt_cliente.cpp
receptor_bocina/src/alarma.cpp
```

#### Decisiones pendientes

1. Qué mensajes requieren HMAC obligatorio.
2. Si ACK, HELLO y heartbeat deben firmarse siempre.
3. Si se usará HMAC truncado de 4 u 8 bytes.
4. Cómo se rotan y provisionan las claves.
5. Qué ocurre si LittleFS falla.
6. Qué nivel de pérdida de MQTT es aceptable.
7. Cuántos reintentos reliable se permiten.
8. Cómo se informa un fallo de autenticación sin filtrar datos sensibles.

#### Criterios de aceptación

- Un evento local continúa funcionando aunque MQTT esté apagado.
- Un paquete con HMAC inválido no produce efectos ni ACK no autorizado.
- Un replay no activa la alarma.
- Un reinicio genera una identidad de sesión distinguible.
- Un fallo de WiFi o MQTT no bloquea el loop principal.
- El watchdog y la recuperación se prueban con fallos controlados.
- Las credenciales no aparecen en archivos versionados, logs ni payloads de diagnóstico.

---

### 14.4 Idea 2 — Estado completo de cada dispositivo

**Objetivo:** convertir el registry de `ONLINE / STALE / OFFLINE` en una ficha diagnóstica útil para la central y Home Assistant.

#### Modelo propuesto

```text
DEVICE
 ├── ID
 ├── nombre
 ├── tipo
 ├── IP
 ├── puerto
 ├── RSSI
 ├── uptime
 ├── BOOT_ID
 ├── último SEQ
 ├── último heartbeat
 ├── firmware
 ├── estado
 ├── batería
 ├── errores
 └── capabilities
```

#### Significado de los campos

- `ID`: identificador estable del dispositivo dentro de la red IoT.
- `nombre`: nombre legible anunciado por HELLO o configuración central.
- `tipo`: PIR, puerta, temperatura, relé u otro tipo registrado.
- `IP` y `puerto`: endpoint observado por la central; pueden cambiar si se usa DHCP.
- `RSSI`: calidad de WiFi reportada por el dispositivo.
- `uptime`: segundos desde el último arranque del dispositivo.
- `BOOT_ID`: identidad de la sesión actual.
- `último SEQ`: última secuencia aceptada para ese BOOT_ID.
- `último heartbeat`: instante local en que se recibió la última señal de vida.
- `firmware`: versión reportada por HELLO o telemetría.
- `estado`: `UNKNOWN`, `ONLINE`, `STALE` u `OFFLINE`.
- `batería`: porcentaje, voltaje o estado de batería si el dispositivo lo soporta.
- `errores`: contadores estructurados, no texto arbitrario como única fuente.
- `capabilities`: funciones anunciadas por el dispositivo.

#### Ejemplo de estado legible

```text
PIR SALA
ONLINE
RSSI -58 dBm
uptime 17 días
firmware 4.3.1
último heartbeat: 8 s
```

#### Flujo de actualización

```text
HELLO
  → crear o actualizar ID, nombre, tipo, firmware y capabilities

HEARTBEAT
  → actualizar RSSI, uptime, heap, último heartbeat y estadísticas

EVENT / DATA / STATE_REPORT
  → actualizar lastSeen y estado ONLINE

temporizador de la central
  → ONLINE → STALE → OFFLINE según timeout

cambio de estado
  → registrar evento
  → publicar estado retained a MQTT
```

#### Decisiones pendientes

1. Qué campos pertenecen a `RemoteDevice` y cuáles a una estructura de telemetría separada.
2. Si el nombre puede ser cambiado remotamente.
3. Cómo se representa batería cuando un dispositivo no tiene batería.
4. Qué errores son contadores y cuáles son el último error.
5. Qué ocurre si cambia la IP pero no cambia el BOOT_ID.
6. Si el registry se persiste en LittleFS.
7. Cuánto tiempo se conserva un dispositivo OFFLINE.
8. Si `lastSeq` se reinicia al cambiar BOOT_ID.
9. Qué información se publica individualmente y cuál se publica como JSON agregado.

#### Archivos relacionados

```text
lib/IoTProtocol/IoTNode.h
lib/IoTProtocol/IoTNode.cpp
lib/IoTProtocol/IoTProtocol.h
receptor_central_v4/src/event_handler.cpp
receptor_central_v4/src/main.cpp
receptor_central_v4/src/mqtt_manager.cpp
receptor_central_v4/include/mqtt_manager.h
```

#### Criterios de aceptación

- La central muestra una ficha completa por dispositivo conocido.
- HELLO actualiza nombre, tipo, firmware y capabilities.
- Heartbeat actualiza RSSI, uptime y estadísticas.
- La transición `ONLINE / STALE / OFFLINE` es reproducible.
- Un cambio de IP se actualiza sin duplicar el dispositivo.
- Los datos publicados a MQTT tienen nombres y tipos estables.
- Un dispositivo sin batería no publica una batería ficticia.

---

### 14.5 Idea 3 — Registro de eventos

**Objetivo:** conservar los últimos eventos para que el sistema sea diagnosticable, no solo observable en tiempo real.

#### Capacidad inicial

Guardar en la central los últimos **100 eventos** mediante un buffer circular:

```text
16:04:21  PIR_SALA     MOTION
16:04:21  CENTRAL      ALARM_ON
16:04:22  MQTT         CONNECTED
16:05:02  PIR_SALA     HEARTBEAT
16:06:10  PIR_GARAJE   OFFLINE
16:06:15  PIR_GARAJE   ONLINE
```

#### Modelo de evento

```text
EventLogEntry
 ├── timestamp o uptime
 ├── source_id
 ├── source_name
 ├── category
 ├── event_code
 ├── value o payload pequeño
 ├── severity
 ├── boot_id opcional
 └── sequence opcional
```

#### Categorías iniciales

```text
SENSOR_EVENT
CENTRAL_STATE
ALARM_STATE
MQTT_STATE
WIFI_STATE
DEVICE_STATE
SECURITY
OTA
SYSTEM_ERROR
```

#### Flujo

```text
evento recibido
      ↓
validar y clasificar
      ↓
insertar en buffer circular de 100 entradas
      ↓
si es importante, publicar a MQTT
      ↓
si la persistencia está habilitada, guardar por lotes
      ↓
Home Assistant consulta o muestra los eventos
```

#### Persistencia

- La primera versión puede mantener el historial en RAM.
- La persistencia en LittleFS debe escribirse por lotes, no en cada evento.
- Debe existir una política de desgaste y recuperación ante archivo corrupto.
- El timestamp puede ser `millis()`, uptime o tiempo real si existe una fuente confiable.
- No guardar secretos dentro del registro.

#### Decisiones pendientes

1. Si los 100 eventos sobreviven a un reinicio.
2. Si el timestamp será RTC, NTP, uptime o una combinación.
3. Cuánto texto puede contener cada entrada.
4. Si el registro se publica como JSON, como topics individuales o mediante API.
5. Qué eventos son `INFO`, `WARNING`, `ERROR` o `CRITICAL`.
6. Qué eventos deben disparar notificación.
7. Cómo se consulta desde Home Assistant.
8. Cuánto flash se reserva.
9. Cómo se compacta y recupera el archivo.

#### Archivos probables

```text
receptor_central_v4/include/event_log.h
receptor_central_v4/src/event_log.cpp
receptor_bocina/include/event_log.h
receptor_bocina/src/event_log.cpp
lib/IoTProtocol/IoTProtocol.h
receptor_central_v4/src/mqtt_manager.cpp
```

#### Criterios de aceptación

- Se conservan como máximo 100 eventos sin corromper el buffer.
- Al insertar el evento 101 se elimina únicamente el más antiguo.
- Un evento de seguridad incluye origen y contexto suficiente.
- Los cambios `ONLINE`, `STALE`, `OFFLINE`, MQTT y alarma quedan registrados.
- El registro no bloquea el procesamiento UDP.
- El sistema puede mostrar o exportar el historial desde Home Assistant.
- Un fallo de LittleFS no impide que la alarma local siga funcionando.

---

### 14.6 Idea 4 — Máquina de estados de alarma

**Objetivo:** reemplazar la lógica simplificada de alarma ON/OFF por una máquina de estados explícita y verificable.

#### Estados del ciclo de alarma

```text
DISARMED
  ↓
ARMING
  ↓
ARMED
  ↓
TRIGGERED
  ↓
ALARMING
  ↓
ACKNOWLEDGED
  ↓
DISARMED
```

#### Significado de cada estado

- `DISARMED`: la alarma está desactivada. Los sensores pueden seguir reportando eventos, pero no activan la sirena según la política configurada.
- `ARMING`: se inició el armado y está transcurriendo un tiempo de salida. Durante este periodo puede permitirse que el usuario abandone la zona sin disparar la alarma.
- `ARMED`: la alarma está armada y los sensores configurados como activos pueden provocar una alarma.
- `TRIGGERED`: se recibió un evento que cumple las condiciones de disparo, por ejemplo `MOTION`, `DOOR_OPEN`, `SMOKE` o `TAMPER`.
- `ALARMING`: la central ya ejecutó la acción de alarma: sirena, publicaciones MQTT y notificaciones.
- `ACKNOWLEDGED`: el evento de alarma fue reconocido por el usuario o por una orden autorizada, pero todavía deben cumplirse las condiciones para regresar a `DISARMED`.
- `DISARMED`: estado final después del reconocimiento y desarmado.

#### Modos de operación

```text
DISARMED
ARM_HOME
ARM_AWAY
NIGHT
ALARM
MAINTENANCE
```

Los modos representan la política de funcionamiento, mientras que los estados representan el ciclo de transición de la alarma. No deben mezclarse automáticamente sin decidir primero el modelo.

#### Significado de cada modo

- `DISARMED`: todos los sensores de alarma están desactivados para efectos de disparo.
- `ARM_HOME`: la casa está ocupada; algunos sensores interiores pueden quedar excluidos y sensores perimetrales permanecen activos.
- `ARM_AWAY`: todos los sensores configurados quedan activos.
- `NIGHT`: se activa una combinación específica para horario nocturno, por ejemplo puertas, ventanas y zonas exteriores, dejando inactivos ciertos sensores interiores.
- `ALARM`: modo de emergencia o condición global de alarma. Debe definirse si es un modo persistente o simplemente un reflejo de `ALARMING`.
- `MAINTENANCE`: permite pruebas y configuración sin activar la alarma real. Debe requerir autorización y no debe habilitarse accidentalmente desde MQTT.

#### Transiciones iniciales propuestas

```text
DISARMED
  └─ comando ARM_HOME/ARM_AWAY/NIGHT
       → ARMING

ARMING
  ├─ finaliza el tiempo de salida
  │    → ARMED
  └─ comando DISARM
       → DISARMED

ARMED
  ├─ evento permitido de sensor
  │    → TRIGGERED
  └─ comando DISARM
       → DISARMED

TRIGGERED
  └─ procesamiento confirmado del evento
       → ALARMING

ALARMING
  ├─ comando ACK autorizado
  │    → ACKNOWLEDGED
  └─ nueva condición crítica
       → ALARMING

ACKNOWLEDGED
  ├─ comando DISARM autorizado
  │    → DISARMED
  └─ condición todavía activa
       → ALARMING

MAINTENANCE
  ├─ finaliza la sesión de mantenimiento
  │    → DISARMED
  └─ evento de prueba
       → registrar evento sin activar alarma real
```

#### Decisiones pendientes

1. Si `ALARM` será un modo separado o un alias de `ALARMING`.
2. Si `TRIGGERED` y `ALARMING` deben ser estados separados.
3. Cuánto dura el tiempo de `ARMING`.
4. Si el tiempo de entrada debe existir antes de pasar de `TRIGGERED` a `ALARMING`.
5. Qué sensores están activos en `ARM_HOME`.
6. Qué sensores están activos en `NIGHT`.
7. Si `ACKNOWLEDGED` apaga inmediatamente la sirena.
8. Qué sucede si el sensor continúa activo después del reconocimiento.
9. Qué comandos pueden cambiar el modo.
10. Cómo se autentican los comandos remotos.
11. Cómo se persiste el modo después de reiniciar la central.
12. Qué ocurre si se reinicia la central mientras está en `ALARMING`.
13. Cómo se publica cada estado a MQTT/Home Assistant.
14. Qué eventos quedan registrados en el historial.
15. Qué diferencia hay entre una alarma real y un evento de prueba en `MAINTENANCE`.

#### Archivos que probablemente habría que revisar

```text
receptor_bocina/include/state_machine.h
receptor_bocina/src/state_machine.cpp
receptor_bocina/include/config.h
receptor_bocina/src/config.cpp
receptor_bocina/src/alarma.cpp
receptor_bocina/src/main.cpp
receptor_bocina/src/mqtt_cliente.cpp
receptor_bocina/src/mqtt_discovery.cpp
receptor_central_v4/src/event_handler.cpp
receptor_central_v4/src/main.cpp
```

#### Criterios de aceptación

- Cada transición válida está implementada explícitamente.
- Las transiciones inválidas se rechazan y quedan registradas.
- `ARM_HOME`, `ARM_AWAY`, `NIGHT` y `MAINTENANCE` tienen políticas diferentes.
- Un evento de sensor solo dispara alarma si el modo y el estado lo permiten.
- Un comando MQTT no autenticado no puede armar, desarmar ni reconocer la alarma.
- La central conserva el estado después de un reinicio según la política decidida.
- MQTT/Home Assistant refleja el modo y el estado actual.
- Los cambios de estado quedan en el registro de eventos.
- Existen pruebas para cada transición y para comandos inválidos.

---

### 14.7 Idea 5 — Zonas de sensores

**Objetivo:** organizar los dispositivos por ubicación y aplicar políticas de armado por zona, en lugar de tratar cada sensor como una entidad aislada.

#### Modelo conceptual

```text
CASA
├── Sala
├── Cocina
├── Dormitorio
├── Garaje
└── Patio
```

Cada dispositivo debe poder pertenecer a una zona:

```text
PIR_SALA      → Sala
PIR_COCINA    → Cocina
PIR_GARAJE    → Garaje
PUERTA_PATIO  → Patio
```

#### Ejemplo de política

```text
ARM_HOME

Sala       → activa
Cocina     → activa
Dormitorio → inactiva
Garaje     → activa
```

#### Modelo propuesto

```text
Zone
 ├── zone_id
 ├── name
 ├── enabled
 ├── members[]
 ├── trigger_policy
 └── notification_policy
```

```text
DeviceConfig
 ├── device_id
 ├── zone_id
 ├── enabled
 ├── priority
 └── sensor_policy
```

#### Flujo

```text
HELLO o configuración remota
      ↓
central conoce device_id y zone_id
      ↓
se selecciona ARM_HOME/ARM_AWAY/NIGHT
      ↓
la central evalúa si la zona está activa
      ↓
evento del sensor
      ├─ zona activa     → puede disparar alarma
      ├─ zona inactiva   → registrar/publicar sin sirena
      └─ mantenimiento   → registrar como prueba
```

#### Decisiones pendientes

1. Si las zonas se configuran únicamente en la central o también en el dispositivo.
2. Si un dispositivo puede pertenecer a varias zonas.
3. Qué zona predeterminada recibe un dispositivo nuevo.
4. Qué ocurre con un sensor sin zona.
5. Si `ARM_HOME` y `NIGHT` comparten zonas o tienen políticas independientes.
6. Si una zona deshabilitada sigue reportando eventos.
7. Cómo se autentica la modificación de zonas.
8. Cómo se persisten las zonas.
9. Cómo se publican las zonas a Home Assistant.
10. Si una zona completa OFFLINE debe producir una alerta especial.

#### Archivos probables

```text
receptor_central_v4/include/zone_manager.h
receptor_central_v4/src/zone_manager.cpp
receptor_central_v4/include/config.h
receptor_central_v4/src/event_handler.cpp
receptor_central_v4/src/main.cpp
receptor_central_v4/src/mqtt_manager.cpp
receptor_bocina/src/alarma.cpp
lib/IoTProtocol/IoTConfigHandler.h
lib/IoTProtocol/IoTConfigHandler.cpp
```

#### Criterios de aceptación

- Un dispositivo nuevo puede asignarse a una zona sin cambiar el protocolo.
- `ARM_HOME`, `ARM_AWAY` y `NIGHT` producen políticas diferentes.
- Un evento de zona inactiva no activa la sirena, pero queda registrado.
- Un sensor sin zona tiene un comportamiento explícito y documentado.
- Cambiar una zona requiere autenticación.
- Las zonas sobreviven al reinicio según la política definida.
- Home Assistant puede mostrar zonas y su estado.

---

### 14.8 Idea 6 — Configuración centralizada

**Objetivo:** configurar un dispositivo desde la central sin recompilarlo, conservando la configuración después de un reinicio y sin permitir cambios no autenticados.

#### Ejemplo de configuración

```text
device: PIR_SALA

heartbeat: 30 s
debounce: 500 ms
nombre: Sala
zona: Interior
prioridad: HIGH
modo: activo
```

#### Campos iniciales

```text
DeviceConfig
 ├── device_id
 ├── device_name
 ├── zone_id
 ├── heartbeat_interval_ms
 ├── debounce_ms
 ├── priority
 ├── enabled
 ├── auth_enabled
 └── config_version
```

#### Flujo seguro

```text
usuario autorizado cambia configuración en Home Assistant
      ↓
central valida formato y permisos
      ↓
central crea CONFIG autenticado con config_version
      ↓
dispositivo verifica auth
      ↓
dispositivo valida rango y versión
      ↓
dispositivo aplica configuración en RAM
      ↓
dispositivo guarda en LittleFS
      ↓
dispositivo responde ACK/RESPONSE
      ↓
central publica resultado
```

#### Reglas

- La configuración remota nunca puede saltarse autenticación.
- Validar rangos antes de guardar.
- Rechazar versiones antiguas o comandos duplicados.
- Mantener configuración válida anterior si falla la escritura.
- No permitir que una configuración remota desactive permanentemente el mecanismo de autenticación sin una operación local de recuperación.
- Registrar quién, cuándo y qué configuración se modificó, sin guardar secretos.

#### Decisiones pendientes

1. Qué campos pueden cambiarse remotamente.
2. Qué campos requieren reinicio.
3. Si el nombre y zona son autoridad de la central o del dispositivo.
4. Si la central espera confirmación de persistencia.
5. Qué ocurre cuando la versión de configuración no es compatible.
6. Cómo se revierte una configuración inválida.
7. Si `auth_enabled` puede cambiarse remotamente.
8. Cómo se autorizan usuarios de Home Assistant.
9. Qué tamaño máximo tendrá el mensaje CONFIG.

#### Archivos probables

```text
lib/IoTProtocol/IoTConfigHandler.h
lib/IoTProtocol/IoTConfigHandler.cpp
lib/IoTProtocol/IoTStorage.h
lib/IoTProtocol/IoTStorage.cpp
lib/IoTProtocol/IoTProtocol.h
receptor_central_v4/src/mqtt_manager.cpp
receptor_central_v4/src/event_handler.cpp
emisor_pir_v4/src/main.cpp
```

#### Criterios de aceptación

- Un cambio válido se aplica, persiste y se confirma.
- Un cambio no autenticado se rechaza.
- Un valor fuera de rango no se guarda.
- Una configuración corrupta no deja al dispositivo inutilizable.
- Un reinicio conserva la configuración válida anterior.
- La central distingue `sent`, `accepted`, `persisted` y `rejected`.
- Los cambios aparecen en el registro de eventos.

---

### 14.9 Idea 7 — OTA controlado y rollback

**Objetivo:** actualizar firmware remoto de forma autenticada, verificable y recuperable.

#### Flujo propuesto

```text
Central detecta firmware nuevo
      ↓
verifica versión, hash y compatibilidad
      ↓
envía orden OTA autenticada
      ↓
dispositivo acepta y prepara actualización
      ↓
descarga firmware desde fuente autorizada
      ↓
verifica hash y tamaño
      ↓
aplica firmware
      ↓
reinicia
      ↓
BOOT_ID nuevo
      ↓
HELLO y heartbeat
      ↓
central verifica versión y estado ONLINE
      ↓
OK o rollback
```

#### Estados OTA

```text
OTA_IDLE
OTA_REQUESTED
OTA_AUTHORIZED
OTA_DOWNLOADING
OTA_VERIFYING
OTA_APPLYING
OTA_REBOOTING
OTA_CONFIRMED
OTA_FAILED
OTA_ROLLBACK
```

#### Requisitos

- Orden OTA autenticada.
- Firmware con versión y hash.
- Compatibilidad de hardware comprobada.
- Tiempo límite para descarga y confirmación.
- No activar OTA si la batería o alimentación no son suficientes, si aplica.
- Confirmación posterior al reinicio.
- Rollback real o mecanismo físico de recuperación.
- Registro de versión anterior, nueva versión y resultado.

#### Decisiones pendientes

1. Si la central sirve el firmware por HTTP o usa otro servidor.
2. Cómo se autentica la descarga, además de verificar hash.
3. Si se firma el firmware o solo se usa hash.
4. Qué bootloader/particiones permiten rollback en ESP8266.
5. Cuánto tiempo tiene el dispositivo para confirmar el arranque.
6. Qué ocurre si se corta la energía durante OTA.
7. Si se actualizan todos los nodos o uno por uno.
8. Cómo se detiene una actualización defectuosa.
9. Si V3 participa o solo V4.
10. Qué versiones son compatibles con el wire format.

#### Archivos probables

```text
receptor_central_v4/src/ota_manager.cpp
receptor_central_v4/include/ota_manager.h
emisor_pir_v4/src/ota_client.cpp
emisor_pir_v4/include/ota_client.h
lib/IoTProtocol/IoTProtocol.h
lib/IoTProtocol/IoTAuth.*
lib/IoTProtocol/IoTStorage.*
```

#### Criterios de aceptación

- Una orden OTA no autenticada no se ejecuta.
- Un firmware con hash incorrecto no se instala.
- Un nodo que reinicia correctamente confirma la actualización.
- Un nodo que no confirma queda marcado como fallo.
- El rollback se prueba provocando un firmware inválido o fallo controlado.
- Una actualización no bloquea permanentemente la alarma local.
- El historial registra resultado y versiones sin secretos.

---

### 14.10 Idea 8 — Añadir sensores sin modificar el protocolo

**Objetivo:** permitir añadir sensores y actuadores sin crear protocolos diferentes para cada dispositivo.

#### Sensores y eventos previstos

```text
PIR
MAGNETICO
TEMPERATURA
HUMEDAD
HUMO
GAS
LUZ
BOTON
VIBRACION
```

#### Modelo de mensajes

```text
EVENT
 ├── MOTION
 ├── DOOR_OPEN
 ├── DOOR_CLOSE
 ├── SMOKE
 ├── BUTTON
 └── TEMPERATURE
```

Los datos continuos, como temperatura y humedad, pueden usar `DATA` con TLV específicos. Los cambios críticos, como puerta abierta, humo o movimiento, pueden usar `EVENT` reliable según la política.

#### Flujo de incorporación de un sensor

```text
definir tipo de dispositivo y capabilities
      ↓
reutilizar IoTNode y wire format
      ↓
definir TLV/EventCode solo si no existe uno compatible
      ↓
crear carpeta emisor_XXX_v4/
      ↓
implementar lectura no bloqueante
      ↓
enviar HELLO
      ↓
enviar DATA/EVENT/HEARTBEAT
      ↓
verificar que la central procesa genéricamente
      ↓
crear discovery o entidad MQTT
```

#### Regla de compatibilidad

- No modificar la cabecera existente para añadir un sensor.
- Reutilizar `MsgType`, `EventCode` y `TlvTag` existentes cuando semánticamente correspondan.
- Si hace falta un tipo nuevo compatible, añadirlo documentado.
- Si el cambio rompe consumidores existentes, incrementar la versión mayor.
- El receptor debe procesar por tipo/capability, no por una lista rígida de sensores conocidos.

#### Dispositivos candidatos

```text
emisor_pir_v4
emisor_temp_v4
emisor_puerta_v4
actuador_relay_v4
ESP8266 PIR
ESP32 PIR
ESP32 temperatura
ESP8266 puerta
ESP32 relé
```

#### Decisiones pendientes

1. Qué sensor usa `EVENT` y cuál usa `DATA`.
2. Qué eventos requieren ACK.
3. Cómo se representan unidades y escalas.
4. Cómo se manejan sensores desconocidos.
5. Si el receptor publica automáticamente una entidad para cada capability.
6. Cómo se evita crear enums incompatibles.
7. Qué parte es genérica y qué parte permanece específica del dispositivo.

#### Archivos probables

```text
lib/IoTProtocol/IoTProtocol.h
lib/IoTProtocol/IoTPacket.*
lib/IoTProtocol/IoTNode.*
receptor_central_v4/src/event_handler.cpp
receptor_central_v4/src/mqtt_manager.cpp
emisor_temp_v4/
emisor_puerta_v4/
actuador_relay_v4/
```

#### Criterios de aceptación

- Un sensor nuevo puede añadirse sin duplicar el protocolo.
- V3 sigue funcionando sin conocer el sensor nuevo.
- La central no requiere una reescritura para recibir un evento compatible.
- Cada nuevo sensor tiene HELLO, heartbeat y política de errores.
- Los tipos y TLV están documentados y probados.
- Un sensor desconocido no bloquea ni corrompe la central.

---

### 14.11 Idea 9 — Sistema de capabilities

**Objetivo:** que cada dispositivo anuncie automáticamente qué puede detectar, medir o controlar y que la central no dependa de una lista fija.

#### Ejemplo

```text
DEVICE_ID: 0x12

CAPABILITIES:
  MOTION
  TEMPERATURE
  BATTERY
```

```text
DEVICE_ID: 0x13

CAPABILITIES:
  DOOR
  BATTERY
```

#### Flujo

```text
dispositivo arranca
      ↓
HELLO incluye capabilities
      ↓
central valida y guarda capabilities
      ↓
central crea handlers/entidades compatibles
      ↓
Home Assistant recibe discovery correspondiente
      ↓
si cambian capabilities, se actualiza el registry
```

#### Modelo propuesto

```text
Capability
 ├── capability_id
 ├── kind: SENSOR / ACTUATOR / DIAGNOSTIC
 ├── data_type
 ├── unit
 ├── readable
 ├── writable
 ├── event_codes[]
 └── config_schema opcional
```

#### Ejemplo de capability de dispositivo

```text
MOTION
  kind: SENSOR
  readable: true
  writable: false

TEMPERATURE
  kind: SENSOR
  unit: °C
  readable: true
  writable: false

RELAY
  kind: ACTUATOR
  readable: true
  writable: true
```

#### Decisiones pendientes

1. Si capabilities se codifican como bitmask, lista TLV o mensaje separado.
2. Cómo se versiona una capability.
3. Qué hace la central con una capability desconocida.
4. Cómo se publican unidades y escalas.
5. Si un actuador debe anunciar comandos permitidos.
6. Si capabilities pueden cambiar en runtime.
7. Cómo se actualiza MQTT Discovery cuando cambian.
8. Qué límite de capabilities soporta un paquete HELLO.

#### Archivos probables

```text
lib/IoTProtocol/IoTProtocol.h
lib/IoTProtocol/IoTPacket.*
lib/IoTProtocol/IoTNode.*
receptor_central_v4/src/event_handler.cpp
receptor_central_v4/src/mqtt_manager.cpp
receptor_central_v4/include/device_registry.h
```

#### Criterios de aceptación

- La central conoce capabilities sin configuración manual para cada sensor.
- Un dispositivo con capabilities desconocidas no rompe el parser.
- Home Assistant crea únicamente entidades compatibles.
- Las capabilities de actuadores no habilitan comandos sin autenticación.
- HELLO y registry conservan la versión de capability.
- Añadir una capability compatible no exige cambiar la cabecera V4.

---

### 14.12 Idea 10 — Watchdog y sistema de recuperación

**Objetivo:** garantizar que un fallo de una parte no derribe todo el sistema.

#### Comportamiento requerido

```text
WiFi perdido
    ↓
reintentar

MQTT perdido
    ↓
reintentar con backoff

UDP funcionando
    ↓
seguir alarma

loop bloqueado
    ↓
watchdog
    ↓
reinicio

reinicio
    ↓
BOOT_ID nuevo
    ↓
heartbeat
```

#### Subsistemas independientes

- La detección local no depende de MQTT.
- La bocina local no depende de Home Assistant.
- El watchdog no debe ser ocultado alimentándolo dentro de un loop bloqueado.
- La reconexión WiFi no debe impedir la recepción UDP cuando el hardware aún puede recibir.
- El reinicio debe dejar una razón de boot registrable.

#### Estados de recuperación posibles

```text
HEALTHY
WIFI_RECONNECTING
MQTT_BACKOFF
DEGRADED_LOCAL
WATCHDOG_RESET
RECOVERY_PENDING
RECOVERED
```

#### Decisiones pendientes

1. Qué timeout del watchdog es seguro para cada firmware.
2. Qué razones de reinicio soporta la plataforma.
3. Qué acciones se intentan antes de reiniciar.
4. Cuántos fallos consecutivos provocan una alerta.
5. Si la central debe avisar de un watchdog reset.
6. Cómo se distingue fallo de WiFi, MQTT, UDP o aplicación.
7. Si se guarda un contador de reinicios en LittleFS.
8. Cómo evitar desgaste por reinicios repetidos.

#### Archivos probables

```text
emisor_pir_v4/src/main.cpp
receptor_central_v4/src/main.cpp
receptor_bocina/src/main.cpp
receptor_bocina/src/mqtt_cliente.cpp
receptor_bocina/src/state_machine.cpp
lib/IoTProtocol/IoTStorage.*
```

#### Criterios de aceptación

- MQTT caído no impide detectar ni activar la alarma local.
- WiFi perdido produce reintentos no bloqueantes.
- El watchdog reinicia un firmware realmente bloqueado.
- El reinicio anuncia BOOT_ID y razón de boot.
- La central puede distinguir un nodo recuperado de un nodo que simplemente estuvo offline.
- Los reintentos tienen backoff y no saturan la red.

---

### 14.13 Idea 11 — Telemetría

**Objetivo:** publicar periódicamente una visión cuantitativa del estado de cada dispositivo para diagnóstico y automatización.

#### Topic propuesto

```text
casa/iot/device/pir_sala/status
```

El nombre exacto debe mantener una convención estable y no depender de un nombre editable sin un `unique_id` estable.

#### Payload propuesto

```json
{
  "online": true,
  "rssi": -61,
  "uptime": 182736,
  "free_heap": 42136,
  "firmware": "4.3.1",
  "boot_id": 27
}
```

#### Campos posibles

```text
online
rssi
uptime
free_heap
firmware
boot_id
last_seq
queue_depth
tx_count
rx_count
ack_timeouts
retries
duplicates
battery
reset_reason
```

#### Flujo

```text
heartbeat o temporizador local
      ↓
recoger métricas sin bloquear
      ↓
construir payload limitado y válido
      ↓
firmar si la política lo exige
      ↓
publicar a la central
      ↓
central publica estado retained a MQTT
      ↓
Home Assistant actualiza sensores de diagnóstico
```

#### Decisiones pendientes

1. Si la telemetría viaja en `HEARTBEAT`, `DATA` o ambos.
2. Frecuencia de publicación.
3. Si el payload será JSON o TLV convertido a JSON por la central.
4. Qué campos son obligatorios.
5. Si se publican topics individuales además del JSON agregado.
6. Qué datos se publican retained.
7. Cómo se limita el tamaño del payload.
8. Cómo se evita que la telemetría compita con eventos críticos.
9. Si batería y reset reason son opcionales.

#### Archivos probables

```text
lib/IoTProtocol/IoTProtocol.h
lib/IoTProtocol/IoTNode.cpp
emisor_pir_v4/src/main.cpp
receptor_central_v4/src/event_handler.cpp
receptor_central_v4/src/mqtt_manager.cpp
receptor_bocina/src/mqtt_cliente.cpp
```

#### Criterios de aceptación

- La telemetría no bloquea la detección de eventos.
- El payload siempre es JSON válido si se eligió JSON.
- Los valores de RSSI, heap, uptime y BOOT_ID coinciden con el dispositivo.
- Home Assistant puede mostrar el estado sin depender de logs seriales.
- Un dispositivo sin una métrica opcional no publica un valor engañoso.
- La frecuencia y el tamaño están documentados.

---

### 14.14 Idea 12 — Panel de diagnóstico

**Objetivo:** mostrar rápidamente la salud de la central, MQTT, WiFi, dispositivos, alarma y cantidad de eventos.

#### Diseño conceptual

```text
╔══════════════════════════════════╗
║        IoT ALARM SYSTEM          ║
╠══════════════════════════════════╣
║ CENTRAL       🟢 ONLINE           ║
║ MQTT          🟢 CONNECTED        ║
║ WiFi          🟢 -57 dBm          ║
╠══════════════════════════════════╣
║ SALA          🟢 ONLINE           ║
║ COCINA        🟢 ONLINE           ║
║ GARAJE        🟠 STALE            ║
║ PATIO         🔴 OFFLINE          ║
╠══════════════════════════════════╣
║ ALARMA        DESARMADA           ║
║ EVENTOS       127                 ║
╚══════════════════════════════════╝
```

#### Secciones del panel

```text
Sistema
 ├── central online/offline
 ├── MQTT connected/disconnected
 ├── WiFi y RSSI
 └── firmware/uptime

Dispositivos
 ├── nombre
 ├── zona
 ├── ONLINE/STALE/OFFLINE
 ├── RSSI
 ├── batería
 └── último evento

Alarma
 ├── modo actual
 ├── estado actual
 ├── última activación
 └── reconocimiento pendiente

Diagnóstico
 ├── número de eventos
 ├── ACK timeouts
 ├── retransmisiones
 ├── errores de auth
 └── reinicios
```

#### Flujo de datos

```text
IoTNode/central
      ↓
registry + event log + alarm state
      ↓
MQTT topics retained / discovery
      ↓
Home Assistant dashboard
```

#### Decisiones pendientes

1. Si el dashboard se implementa solo en Home Assistant.
2. Qué entidades se crean mediante MQTT Discovery.
3. Si se usa un sensor JSON agregado o múltiples sensores.
4. Qué datos deben ser retained.
5. Cómo se representan estados desconocidos.
6. Si existe una vista histórica para eventos.
7. Qué alertas aparecen como notificación y cuáles solo como diagnóstico.
8. Cómo se organizan las entidades por zona.

#### Archivos probables

```text
receptor_central_v4/src/mqtt_manager.cpp
receptor_central_v4/src/event_handler.cpp
receptor_central_v4/src/event_log.cpp
receptor_central_v4/src/zone_manager.cpp
receptor_bocina/src/mqtt_discovery.cpp
README.md
```

#### Criterios de aceptación

- El dashboard muestra central, MQTT y WiFi.
- Cada dispositivo conocido aparece con estado actualizado.
- `STALE` y `OFFLINE` son visualmente distinguibles.
- El modo/estado de alarma se refleja correctamente.
- La cantidad de eventos coincide con el registro definido.
- La pérdida de MQTT no impide la alarma local; el dashboard solo queda temporalmente desactualizado.
- No se crean entidades duplicadas al reiniciar o recibir HELLO repetidos.

---

### 14.15 IoTProtocol como plataforma común

**Objetivo:** convertir `IoTProtocol` en la columna vertebral de una plataforma de dispositivos intercambiables, en vez de continuar agregando lógica específica a `emisor_pir_v4`.

#### Mensajes base de la plataforma

```text
┌─────────────────────────────┐
│          IoTProtocol        │
├─────────────────────────────┤
│ HELLO                       │
│ HEARTBEAT                   │
│ STATE_REPORT                │
│ EVENT                       │
│ ACK                         │
│ COMMAND                     │
│ CONFIG                      │
│ OTA                         │
│ ERROR                       │
└─────────────────────────────┘
```

#### Hardware objetivo

```text
ESP8266 PIR
ESP32 PIR
ESP32 temperatura
ESP8266 puerta
ESP32 relé
```

Todos deben poder hablar el mismo idioma, aunque sus drivers de hardware sean distintos.

#### Separación recomendada

```text
Aplicación del dispositivo
      ↓
modelo de eventos/datos
      ↓
IoTProtocol
      ↓
seguridad
      ↓
transporte
      ↓
WiFi/UDP u otro transporte evaluado
```

#### Reglas de diseño

- La biblioteca no debe conocer detalles del sensor físico.
- El dispositivo debe adaptar su hardware al modelo de mensajes.
- La central debe procesar mensajes genéricos siempre que sea posible.
- Las funciones específicas deben vivir en handlers o perfiles, no en el parser base.
- Añadir un sensor no debe exigir cambiar todos los firmwares.
- Un cambio incompatible requiere versión mayor y migración documentada.

#### Decisiones pendientes

1. Si `IoTProtocol` seguirá siendo propio o adoptará una tecnología estándar.
2. Qué parte de `IoTNode` debe separarse de `WiFiUDP`.
3. Si los perfiles de dispositivo son necesarios.
4. Qué interfaces de transporte y seguridad son realmente útiles en ESP8266.
5. Qué API pública se compromete a mantener.
6. Cómo se mantiene compatibilidad con V3.
7. Cuándo la abstracción añade más complejidad que valor.

#### Criterios de aceptación

- Un sensor nuevo reutiliza la biblioteca sin copiar un protocolo entero.
- La central procesa mensajes por tipo/capability.
- Los handlers de aplicación no duplican serialización y seguridad.
- Existe una estrategia de versionado.
- Hay tests de compatibilidad entre dispositivos.
- La arquitectura elegida se justifica con una matriz, no solo con preferencias.

---

### 14.16 Roadmap detallado por versiones

Este roadmap conserva la secuencia estratégica original de `ideas.md`. Las versiones son objetivos de organización; no deben declararse alcanzadas hasta cumplir sus criterios.

#### V4.3.1 — Seguridad y pruebas

```text
V4.3.1
├── Seguridad
├── HMAC
├── Replay protection
├── BOOT_ID
└── Tests
```

Alcance:

- BOOT_ID persistente conectado.
- HMAC aplicado de manera consistente.
- Verificación antes de ACK/deduplicación/dispatch.
- Política anti-replay definida.
- Tests host de protocolo, CRC, TLV, cola y auth.
- Separación de secretos.
- Línea base de compilación V3/V4.

No avanzar por nombre de versión si estos elementos solo están documentados pero no probados.

#### V4.4 — Estado, eventos y diagnóstico

```text
V4.4
├── Estado de dispositivos
├── Eventos
├── Diagnóstico
└── Watchdog
```

Alcance:

- Registry ampliado.
- Heartbeat y telemetría coherentes.
- Registro circular de 100 eventos.
- Estados `ONLINE / STALE / OFFLINE` verificables.
- Razón de boot y recuperación.
- Primer dashboard o conjunto de entidades de diagnóstico.

#### V4.5 — Zonas y máquina de alarma

```text
V4.5
├── Zonas
├── Modos de alarma
└── Máquina de estados
```

Alcance:

- Estados `DISARMED`, `ARMING`, `ARMED`, `TRIGGERED`, `ALARMING`, `ACKNOWLEDGED`.
- Modos `DISARMED`, `ARM_HOME`, `ARM_AWAY`, `NIGHT`, `ALARM`, `MAINTENANCE`.
- Políticas por zona.
- Comandos autenticados.
- Registro de transiciones.
- Pruebas de transiciones válidas e inválidas.

#### V5.0 — Capabilities, configuración y OTA

```text
V5.0
├── Capacidades
├── Configuración remota
├── OTA
└── Rollback
```

Alcance:

- HELLO con capabilities.
- Registry dinámico.
- Configuración remota autenticada y persistente.
- OTA autenticado, hash/verificación y confirmación.
- Rollback probado o decisión documentada de usar recuperación física.
- Revisión de si la arquitectura requiere romper V4.

#### V5.x — Expansión

```text
V5.x
├── Nuevos sensores
├── Relés
├── Batería
├── Temperatura
└── Expansión
```

Alcance posible:

- PIR adicionales.
- Sensores magnéticos.
- Temperatura y humedad.
- Humo, gas, luz, botón y vibración.
- Relés y otros actuadores.
- Métricas de batería.
- Nuevos perfiles de dispositivo.
- Transportes alternativos solo si la evaluación los justifica.

#### Criterios de promoción de versión

- Todos los elementos de la versión están implementados.
- Los tests automatizados pasan.
- La compilación del hardware afectado pasa.
- Las pruebas funcionales están registradas.
- La documentación coincide con el código.
- No hay secretos ni cambios de wire format no documentados.
- Se conserva la compatibilidad prometida o existe plan de migración.

---

### 14.17 Cinco primeras prioridades estratégicas

Además del orden técnico de las fases, la priorización estratégica original recomienda:

1. **Seguridad.** Evitar paquetes falsos, replay y comandos no autorizados.
2. **Pruebas automáticas.** Convertir las reglas del protocolo en regresiones reproducibles.
3. **Máquina de estados.** Hacer explícitas las reglas de la alarma.
4. **Registro de eventos.** Poder explicar qué ocurrió después de un fallo.
5. **Sistema de capabilities.** Permitir que la central conozca dinámicamente qué puede hacer cada dispositivo.

Si hay que elegir entre una función nueva y una de estas cinco, priorizar la que reduzca riesgo o aumente verificabilidad, salvo que exista una razón operativa documentada.

### 14.18 Estado de las ideas después de esta ampliación

| Idea | Estado documental | Próximo paso |
|---|---|---|
| Seguridad y confiabilidad | **DETALLADA** | Ejecutar fases 0–7 y registrar evidencia |
| Estado completo de dispositivos | **DETALLADA** | Definir campos finales y persistencia |
| Registro de 100 eventos | **DETALLADA** | Elegir RAM/LittleFS y API de consulta |
| Máquina de estados de alarma | **DETALLADA** | Resolver decisiones de estados/modos/transiciones |
| Zonas | **DETALLADA** | Definir modelo y políticas por modo |
| Configuración centralizada | **DETALLADA** | Definir autorización, versión y rollback de config |
| OTA y rollback | **DETALLADA** | Verificar capacidades reales del bootloader |
| Sensores sin protocolo paralelo | **DETALLADA** | Implementar un sensor piloto y probar compatibilidad |
| Capabilities | **DETALLADA** | Elegir representación en HELLO/TLV |
| Watchdog y recuperación | **DETALLADA** | Medir timeouts y probar fallos controlados |
| Telemetría | **DETALLADA** | Definir payload/topic/frecuencia |
| Dashboard | **DETALLADA** | Definir entidades MQTT Discovery y vista HA |
| IoTProtocol como plataforma | **DETALLADA** | Ejecutar evaluación V5 antes de crear abstracciones |
| Roadmap V4.3.1→V5.x | **DETALLADA** | Promover versiones solo con criterios cumplidos |
| Clasificación por capa | **DETALLADA** | Aplicarla a cada nueva issue o feature |
