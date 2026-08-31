# Plan de ejecución futura — wifi_PIR

> **Documento canónico para continuar el proyecto sin consultar archivos históricos o borradores.**
>
> Este documento reúne el estado técnico conocido, las decisiones que deben conservarse, los problemas detectados, el orden de ejecución recomendado, los criterios de aceptación y el trabajo futuro. Una LLM que inicie una sesión nueva debe leer primero este archivo y después inspeccionar el código actual antes de modificarlo.

**Repositorio:** `ydiaz1699/wifi_PIR`
**Proyecto:** red doméstica de sensores y alarma sobre ESP8266
**Fecha de consolidación:** 2026-08-31
**Estado de este plan:** documento canónico actualizado con el plan maestro de unificación V3→V4; las fases siguen requiriendo evidencia de implementación, compilación y verificación.

---

## 1. Cómo debe usar este documento una LLM futura

Antes de cambiar código:

1. Leer este documento completo o, como mínimo, las secciones 2, 3, 4, 6 y 7.
2. Ejecutar `git status --short` y conservar cualquier cambio previo del usuario.
3. Leer los archivos reales relacionados con la fase que se va a ejecutar. Este plan describe la intención, pero el código es la fuente de verdad de las firmas, nombres y estructuras actuales.
4. No asumir que una tarea está implementada porque aparezca en la documentación. Verificarla en el código y mediante compilación o prueba.
5. No mezclar V3 de producción con V4 de desarrollo durante la transición. La unificación final sí migrará el comportamiento probado de V3 a la arquitectura V4, pero solo después de validarlo.
6. No eliminar ni sobrescribir funcionalidad de producción para acelerar el desarrollo de V4; V3 se conserva hasta que la versión unificada esté validada en hardware.
7. Ejecutar primero las comprobaciones pequeñas y después las pruebas completas.
8. Actualizar este documento, `CHANGELOG.md`, `ROADMAP.md` o la documentación correspondiente cuando una fase se complete. La documentación no debe afirmar que una función está terminada si no existe una prueba que lo confirme.

### Regla de trabajo principal

La versión V3.5.1 es la alarma operativa y debe permanecer estable. La versión V4.3 es el laboratorio de la biblioteca `IoTProtocol`. La ruta segura es:

```text
preservar V3 → crear pruebas host para V4 → corregir seguridad V4
→ migrar el comportamiento probado de V3 → validar con simulador/hardware
→ promover una única versión → añadir sensores → evaluar una futura V5
```

No se debe reescribir todo el repositorio para resolver una sola incidencia de V4.

## 1.1 Objetivo arquitectónico: una única versión final

El repositorio tiene dos generaciones funcionales, pero el objetivo del proyecto es terminar con una sola versión de producción. No se deben mantener V3 y V4 como dos productos permanentes ni copiar ambas implementaciones dentro de los mismos archivos.

```text
V3.5.1 actual
  └── comportamiento probado: PIR, TIMBRE, ACK, bocina, modo LOCAL/HA, OTA
        ↓ migración y validación por fases
V4 unificada
  ├── núcleo común IoTProtocol
  ├── emisor PIR + TIMBRE
  └── central + alarma + MQTT/HA + OTA
        ↓ validación en hardware
una única versión de producción
```

### Qué es `lib/IoTProtocol/`

`wifi_PIR/lib/IoTProtocol/` es la biblioteca común que debe convertirse en el núcleo técnico de la versión final. No es una tercera versión del producto ni debe contener lógica específica de la bocina, MQTT, Home Assistant o los pines del PIR.

| Módulo | Responsabilidad |
|---|---|
| `IoTProtocol.h/.cpp` | Wire format binario, CRC16, TLV, serialización y deserialización |
| `IoTNode.h/.cpp` | UDP, cola, prioridades, ACK, retransmisiones, deduplicación y heartbeat |
| `IoTAuth.h/.cpp` | Autenticación HMAC |
| `IoTStorage.h/.cpp` | LittleFS, contador de arranque y configuración persistente |
| `IoTConfigHandler.h/.cpp` | Procesamiento de configuración remota |

La separación final será:

```text
lib/IoTProtocol/
    protocolo, transporte común, reliability, seguridad y persistencia

emisor_pir/
    lectura PIR/TIMBRE, configuración del dispositivo y uso de IoTNode

receptor_central/
    registry, máquina de alarma, bocina, MQTT/Home Assistant, OTA y diagnóstico
```

La biblioteca recibe y valida mensajes; la central decide si un evento activa la bocina o se publica en MQTT. El núcleo no debe conocer pines, nombres de topics ni reglas concretas de la alarma.

### Versiones durante la transición

```text
V3.5.1
├── emisor_pir/
└── receptor_bocina/

V4.3 en desarrollo
├── lib/IoTProtocol/
├── emisor_pir_v4/
└── receptor_central_v4/
```

No se deben borrar ahora `emisor_pir/` ni `receptor_bocina/`: contienen comportamiento de producción que debe inventariarse, migrarse y verificarse. Los proyectos `emisor_pir_v4/` y `receptor_central_v4/` son transitorios mientras se construye la versión unificada.

### Plan maestro de unificación V3 → V4

Las fases `U` son el plan de producto. Las fases técnicas numeradas de la sección 6 contienen las tareas detalladas de seguridad, pruebas y validación que se ejecutan dentro de estas etapas.

#### U0 — Línea base y protección de V3 — `VERIFICADA`

- Compilar emisor y receptor V3.
- Compilar emisor y central V4.
- Ejecutar la suite host existente de `IoTProtocol`.
- Confirmar que `secrets.h` permanece fuera del control de versiones.
- No introducir cambios funcionales en V3 durante el hardening inicial.

Evidencia actual:

```text
emisor_pir              PASS
receptor_bocina         PASS
emisor_pir_v4           PASS
receptor_central_v4     PASS
host IoTProtocol        10/10 PASS
```

#### U1 — Inventario de comportamiento V3 — `PENDIENTE`

Registrar antes de migrar:

- comportamiento del PIR y del TIMBRE;
- ACK asíncrono y eventos simultáneos;
- deduplicación y retransmisiones;
- prioridad local de la bocina;
- modo LOCAL/HA y reconexión MQTT;
- MQTT Discovery;
- OTA;
- máquina de estados;
- límites de bloqueo del loop.

V3 es la fuente del comportamiento probado, no la arquitectura final. Cada elemento debe tener destino en la central o en el emisor unificado y un criterio de aceptación.

#### U2 — Completar y endurecer el núcleo V4 — `EN PROGRESO`

Ejecutar, en este orden:

1. Conectar BOOT_ID persistente de `IoTStorage` con `IoTNode`.
2. Completar tests host de deduplicación, cola y autenticación.
3. Definir el contrato de autenticación y su compatibilidad.
4. Autenticar antes de ACK, registry, deduplicación y callbacks.
5. Aplicar anti-replay compatible con UDP fuera de orden.
6. Validar HMAC, tamaño, campos firmados y política de paquetes inválidos.
7. Crear el simulador UDP reproducible.

No cambiar el wire format V4.x durante esta etapa salvo que una incompatibilidad obligue a una nueva versión mayor documentada.

#### U3 — Migrar el emisor V3 a la arquitectura V4 — `PENDIENTE`

El emisor final debe usar `IoTNode` y publicar:

```text
EVENT + EventCode::MOTION
EVENT + EventCode::TIMBRE
```

Debe conservar:

- lectura independiente de PIR y TIMBRE;
- ausencia de `delay()` bloqueante;
- varios eventos en vuelo;
- retransmisión asíncrona;
- manejo correcto de PIR sostenido en `HIGH`.

El destino final puede ser `emisor_pir/`, reutilizando ese nombre como proyecto único después de migrar y validar. `emisor_pir_v4/` no debe conservarse como segundo emisor permanente.

#### U4 — Migrar el receptor V3 a la central V4 — `PENDIENTE`

La central final debe integrar el comportamiento probado de `receptor_bocina/` con `receptor_central_v4/`:

```text
receptor_central/
├── núcleo IoTProtocol
├── recepción UDP y registry
├── máquina de alarma
├── bocina
├── MQTT/Home Assistant
├── OTA
└── diagnóstico
```

Debe conservar:

- bocina local inmediata;
- UDP antes que MQTT;
- funcionamiento sin broker;
- modo LOCAL/HA;
- reconexión MQTT controlada;
- procesamiento independiente de PIR y TIMBRE;
- OTA y máquina de estados, una vez verificadas.

`IoTProtocol` no activará directamente la bocina ni publicará directamente en MQTT.

#### U4.1 — OTA de mantenimiento para los dos equipos — `PENDIENTE`

El objetivo es que ambos equipos físicos puedan actualizarse por WiFi:

```text
emisor  — Wemos D1 Mini / ESP8266
central — NodeMCU v2 / ESP8266
```

La primera implementación será **ArduinoOTA**, que ya forma parte del ecosistema ESP8266 y requiere menos memoria y dependencias que añadir un servidor web completo. El receptor V3 ya tiene `ArduinoOTA` en `receptor_bocina/src/ota.cpp`; el emisor V4 y la central V4 todavía no tienen el flujo completo conectado aunque la central ya declara un entorno `*_ota` en PlatformIO.

La OTA será un **plano de mantenimiento separado** de `lib/IoTProtocol/`: no se añadirá al wire format ni se mezclará con EVENT, ACK, HMAC o capabilities. Si ambos firmwares comparten suficiente código, se podrá crear un helper separado, por ejemplo `lib/IoTOTA/`, pero no se debe introducir lógica OTA dentro de `lib/IoTProtocol/`.

#### Orden de implementación OTA

1. Añadir `ArduinoOTA` al emisor y a la central unificada.
2. Asignar hostname, puerto y credencial OTA distintos o identificables por equipo.
3. Mantener la credencial OTA separada de WiFi, MQTT y HMAC; debe vivir en `secrets.h` y no aparecer en logs.
4. Crear entornos PlatformIO `emisor_pir_ota` y `receptor_central_ota` con `upload_protocol = espota`.
5. Ejecutar el servicio OTA dentro del loop sin desplazar la recepción UDP: en la central, UDP va primero; en el emisor, el procesamiento de sensores y `IoTNode` conserva prioridad.
6. Usar callbacks de inicio, progreso, finalización y error.
7. Definir un estado seguro antes de aceptar la actualización: no comenzar OTA mientras la central esté ejecutando una alarma crítica o mientras el actuador deje una salida peligrosa activa.
8. Verificar tamaño de firmware y espacio libre con `ESP.getFreeSketchSpace()` antes de depender de OTA.
9. Mantener siempre un procedimiento de recuperación por USB/serial.

#### Seguridad OTA

ArduinoOTA permite configurar password, hostname y puerto, pero eso no convierte automáticamente la OTA en segura para Internet. La OTA se limitará a la LAN de mantenimiento, con firewall o VLAN, sin exponer el puerto al exterior. La credencial OTA no será reutilizada como clave HMAC.

La OTA del ESP8266 no debe documentarse como rollback garantizado. El diseño debe asumir que una pérdida de energía o una imagen incompatible puede requerir recuperación física; por eso cada equipo debe probarse primero por USB y conservarse un firmware recuperable.

#### Interfaz web opcional

`ElegantOTA` se evaluará como una segunda etapa si se necesita actualizar desde un navegador (`/update`) o cargar también LittleFS. No será un requisito para desbloquear la OTA básica porque añade un servidor HTTP y, en modo asíncrono, dependencias como `ESPAsyncWebServer`. Además, su edición open source declara licencia AGPL-3.0, por lo que antes de incorporarla al firmware final debe revisarse si esa licencia es aceptable para el proyecto.

La alternativa `jandrassy/ArduinoOTA` no será la base inicial: su propia documentación advierte que no está probada para PlatformIO. El ejemplo de Hackster sobre un SSID de depuración sirve como referencia educativa, pero no se copiarán credenciales ni un modo de acceso abierto a producción. El artículo de SunFounder está centrado en ESP32 y su modelo de particiones duales no se puede asumir para este ESP8266.

#### Criterios de aceptación OTA

- El emisor se actualiza por `espota` y vuelve a publicar HELLO/heartbeat después de reiniciar.
- La central se actualiza por `espota` y vuelve a recibir UDP.
- Un hostname no apunta al equipo equivocado.
- Una contraseña incorrecta es rechazada.
- Una imagen demasiado grande se detecta antes de iniciar la actualización.
- UDP, PIR, TIMBRE y bocina tienen un comportamiento seguro durante la ventana OTA.
- Una actualización fallida deja un procedimiento de recuperación documentado y probado por serial.
- Se mide RAM, flash y tiempo de loop antes y después de añadir OTA.
- No se prueba OTA durante una alarma crítica ni se expone a Internet.

#### Fuentes consultadas

Contenido reexpresado a partir de la documentación enlazada:

- [ESP8266 Arduino Core — OTA Updates](https://arduino.esp8266.com/Arduino/versions/2.1.0/doc/ota_updates/ota_updates.html): requisitos de flash, password/hostname/puerto, callbacks, seguridad y recuperación.
- [ESP8266 Arduino Core — documentación en español](https://esp8266-arduino-spanish.readthedocs.io/es/latest/ota_updates/readme.html): resumen de ArduinoOTA, actualización web y advertencias de seguridad.
- [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA) y [guía de integración](https://docs.elegantota.pro/getting-started/integration): interfaz web para firmware/LittleFS e integración con `begin()`/`loop()`.
- [ElegantOTA — modo asíncrono](https://docs.elegantota.pro/getting-started/async-mode): dependencias y trade-off del servidor web asíncrono.
- [ArduinoOTA de JAndrassy](https://github.com/jandrassy/ArduinoOTA): alcance general y advertencia específica para PlatformIO.
- [Hackster — OTA en Wemos D1 Mini](https://www.hackster.io/sulis-priyanto/ota-arduino-programming-wemos-d1-mini-4af961): referencia educativa de modo de depuración, no diseño de producción.
- [SunFounder — ArduinoOTA y ElegantOTA](https://www.sunfounder.com/blogs/news/esp32-ota-updates-a-complete-guide-to-arduinoota-and-elegantota-firmware-upgrades): comparación general, con advertencia de que su modelo principal es ESP32.



Si no es posible actualizar todos los dispositivos al mismo tiempo, la central unificada podrá aceptar temporalmente:

```text
central unificada
├── protocolo V4 binario
└── adaptador V3 textual temporal
```

Orden operativo:

1. actualizar la central;
2. mantener emisores V3 funcionando;
3. migrar emisores uno por uno;
4. confirmar que no quedan nodos V3;
5. retirar el adaptador textual.

Este adaptador no es la arquitectura final y tendrá una condición explícita de eliminación.

#### U6 — Validación integrada — `PENDIENTE`

Probar, como mínimo:

- PIR;
- TIMBRE;
- PIR y TIMBRE simultáneos;
- pérdida y recuperación de WiFi;
- MQTT apagado y recuperado;
- duplicados, CRC inválido y HMAC inválido;
- retransmisiones;
- reinicio de emisor y central;
- bocina durante ráfagas;
- Home Assistant;
- OTA;
- seguridad y anti-replay.

La validación debe incluir tests host, simulador y hardware, con métricas de loop, RTT, retransmisiones, RAM, flash y eventos perdidos.

#### U7 — Promoción a una única versión de producción — `PENDIENTE`

Solo cuando U2, U3, U4 y U6 estén verificadas:

```text
se conserva:
├── lib/IoTProtocol/
├── emisor_pir/
├── receptor_central/
├── tests/
└── tools/

se retira o archiva:
├── emisor_pir_v4/
├── receptor_central_v4/
└── código V3 duplicado que ya no tenga consumidores
```

La eliminación de proyectos antiguos requiere comprobar referencias, compilar desde la estructura final y validar hardware. No se debe borrar código solo porque exista una implementación aparentemente equivalente.

#### U8 — Expansión después de la unificación — `PENDIENTE`

Después de tener una única versión estable se podrán añadir:

- capability discovery;
- event log y telemetría;
- configuración remota autenticada;
- perfil de alarma, modos y zonas;
- nuevos sensores y relé;
- OTA de mantenimiento para emisor y central;
- interfaz web ElegantOTA, solo si se aprueba su licencia y necesidad;
- cifrado, solo si existe un requisito real de confidencialidad;
- evaluación comparativa de V5.

### Regla de promoción

Una fase de unificación no está terminada porque compile. Debe cumplir:

```text
implementación → tests → compilación → simulador/hardware → documentación → revisión
```

Mientras U7 no esté verificada, el estado oficial sigue siendo:

```text
V3.5.1 = producción
V4.3   = desarrollo
```

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

### Fase 1 — Infraestructura de pruebas host — `PARCIALMENTE VERIFICADA`

**Objetivo:** poder validar serialización, CRC, TLV, deduplicación, cola y HMAC sin depender inmediatamente del hardware.

#### Estado actual

Ya existe una infraestructura mínima y reproducible para la unidad independiente `IoTProtocol`:

```text
tests/
├── platformio.ini
├── host_compat/Arduino.h
├── src/IoTProtocol.cpp
└── test/test_protocol/test_protocol.cpp
```

Comando validado:

```bash
python3 -m platformio test -d tests -e native_test
```

Resultado actual: **10/10 pruebas pasadas** para CRC16, TLV, endianess, validación, serialización/deserialización, CRC, longitudes, payload máximo, prioridades y versión.

Todavía faltan tests host específicos para deduplicación, cola, HMAC y el orden de efectos de la recepción. Esas pruebas no deben marcarse como cubiertas por el resultado 10/10 actual.

#### Extensión pendiente

```text
tests/
├── platformio.ini
├── host_compat/Arduino.h
├── src/IoTProtocol.cpp
└── test/
    ├── test_protocol/test_protocol.cpp
    ├── test_dedup/test_dedup.cpp
    ├── test_auth/test_auth.cpp
    └── test_queue/test_queue.cpp
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

### 7.6 OTA para los dos equipos

El objetivo inmediato es habilitar actualización OTA de firmware para el emisor Wemos D1 Mini y la central NodeMCU v2. No se debe confundir esta OTA de mantenimiento con un futuro sistema de distribución OTA administrado por la central.

#### Etapa OTA-1 — ArduinoOTA

Usar la biblioteca `ArduinoOTA` del core ESP8266 en ambos firmwares:

- el receptor V3 ya tiene una implementación parcial en `receptor_bocina/src/ota.cpp`;
- el emisor V4 debe añadir inicialización y atención OTA;
- la central V4 debe añadir inicialización y atención OTA, no solo el entorno `upload_protocol = espota`;
- cada equipo debe tener hostname y credencial identificables;
- la contraseña OTA debe estar en `secrets.h`, separada de WiFi, MQTT y HMAC;
- la central debe procesar UDP antes de `ArduinoOTA.handle()`;
- el emisor debe conservar la prioridad de sensores y `IoTNode`;
- la OTA solo se permitirá durante una ventana de mantenimiento y dentro de la LAN.

Los entornos de PlatformIO deberán generar el firmware normal y el entorno de carga OTA, por ejemplo:

```ini
[env:emisor_pir_ota]
extends = env:emisor_pir
upload_protocol = espota
upload_port = <host-o-ip-del-emisor>

[env:receptor_central_ota]
extends = env:receptor_central
upload_protocol = espota
upload_port = <host-o-ip-de-la-central>
```

Los valores reales no se fijarán hasta confirmar la topología y el hostname de cada placa.

#### Etapa OTA-2 — Interfaz web opcional

Solo si se necesita actualizar desde un navegador se evaluará `ElegantOTA`. Esta opción requiere un servidor HTTP, sus rutas de actualización y atención desde el loop; el modo asíncrono añade dependencias de `ESPAsyncWebServer`. También debe revisarse la licencia AGPL-3.0 de la edición open source antes de usarla en el firmware final.

No se añadirá ElegantOTA a los dos equipos simplemente por tener una interfaz atractiva: ArduinoOTA cubre primero la necesidad de actualización por PlatformIO y mantiene menor complejidad en la ruta crítica UDP.

#### Seguridad, seguridad operacional y recuperación

- No exponer el puerto OTA a Internet.
- Aplicar firewall/VLAN o una red de mantenimiento.
- No aceptar OTA mientras la central esté en una alarma crítica o un actuador esté en un estado peligroso.
- Usar callbacks de inicio, progreso, finalización y error.
- Medir `ESP.getFreeSketchSpace()` y el tamaño del binario antes de actualizar.
- Probar pérdida de WiFi, imagen incompatible, contraseña incorrecta y reinicio posterior.
- Mantener siempre recuperación por USB/serial.
- No prometer rollback automático: en ESP8266 debe demostrarse específicamente o documentarse la recuperación física.

#### Criterio de terminado

La OTA de esta fase está terminada solo cuando ambos equipos se actualizan por `espota`, reinician con la aplicación operativa, mantienen la prioridad de alarma/UDP y tienen un procedimiento de recuperación probado. La carga web y la OTA distribuida desde la central son fases posteriores, no requisitos para declarar terminada OTA-1.

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
| Tests host | Parcialmente verificado: `IoTProtocol` 10/10 | Fase 1 / U2 | Deduplicación, cola, HMAC y orden de efectos |
| Simulador UDP | Pendiente | Fase 6 / U2 | Casos reproducibles |
| Sirena con patrón | Pendiente | Fase 8 | MOTION/TIMBRE simultáneos |
| DHT22 | Futuro | Después de Fase 7 | DATA y heartbeat |
| Reed switch | Futuro | Después de Fase 7 | OPEN/CLOSE y state report |
| Relé | Futuro | Después de Fase 7 | COMMAND/RESPONSE |
| DHCP/discovery | Futuro | Después de V4 estable | Cambio de IP |
| OTA para ambos equipos | Pendiente; ArduinoOTA V3 receptor parcial, V4 no conectado | U4.1 / Fase 7.6 | espota en emisor y central, recuperación serial |
| OTA web ElegantOTA | Condicional | Después de OTA-1 | Necesidad, dependencias y licencia AGPL-3.0 |
| OTA distribuido desde central | Futuro lejano | Después de OTA-1 y varios nodos | Autorización, hash, rollback |
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

Las siguientes ideas aparecieron en los drafts y no deben considerarse implementadas solo porque estén descritas:

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
