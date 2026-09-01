# Seguridad de la unificación V4

## Frontera de autenticación

`IoTNode` es la única frontera de recepción de V4. Su `IoTAuthProvider` permite conectar HMAC u otro mecanismo sin acoplar el transporte a BearSSL:

- `DISABLED`: bypass completo; no invoca callbacks de verificación ni firma. Es el modo usado cuando `authEnabled` está desactivado.
- `OPTIONAL`: paquetes sin marca de autenticación pasan; los marcados deben validarse.
- `REQUIRED`: todo paquete entrante debe validarse y toda salida se firma.

La verificación ocurre después de deserializar/validar el wire format y del filtro de destino, pero antes de incrementar `rxPackets`, actualizar el registry, procesar ACK, enviar ACK automático, modificar la ventana de deduplicación o invocar el callback de aplicación. Un paquete rechazado no produce ninguno de esos efectos. El callback `onRejected` es únicamente diagnóstico y debe mantenerse sin lógica de estado.

## Sesión, deduplicación y ACK

Cada remoto mantiene su `BOOT_ID` actual y una ventana deslizante de ocho posiciones sobre `SEQ`. Se aceptan paquetes nuevos y fuera de orden mientras estén dentro de la ventana; un `SEQ` repetido se reconoce pero no vuelve a invocar el callback. Un `SEQ` demasiado antiguo se descarta y, si el paquete solicita ACK, se reconoce sin ejecutar efectos. Los replays no actualizan `lastSeen` ni el endpoint registrado, por lo que tampoco pueden mantener artificialmente un remoto en estado ONLINE. Un `BOOT_ID` diferente solo inicia una nueva sesión cuando avanza según aritmética serial de 16 bits; un BOOT_ID anterior o ambiguo no puede hacer retroceder el registry ni resetear la deduplicación.

Los ACK normalmente requieren el `BOOT_ID` conocido tanto en `RemoteDevice` como en `expectedBootId`. La única excepción es el bootstrap del primer reliable: si el canal está activo y esperando ACK, `SRC` y `SEQ` coinciden, no existe una sesión no-cero conocida para el destino y el ACK autenticado/estructural trae `BOOT_ID` no cero, ese ACK crea o actualiza el `RemoteDevice` y establece `expectedBootId`. Después del bootstrap, un ACK de una sesión anterior, un ACK con sesión desconocida o uno que solo coincida por SRC+SEQ no confirma el evento. Un ACK fuera de un reliable activo tampoco crea ni refresca un remoto. `BOOT_ID=0` es solo el sentinel interno de `registerRemote()` y se rechaza en el wire; un reliable enviado antes de conocer el BOOT_ID del destino usa su primer ACK válido como bootstrap bajo esas condiciones. El wire format binario no cambia.

El callback de firma se ejecuta al encolar y al enviar directamente. Los reintentos reutilizan la copia ya firmada. `IoTAuth::signPacket()` es idempotente para conservar compatibilidad con callers antiguos, y exige que el TLV HMAC sea único y último. `verifyPacket()` rechaza una firma ausente, duplicada, mal formada o seguida por TLV no autenticados.

## Salidas cubiertas

`IoTNode` centraliza la firma de `EVENT`, `HELLO`, `HEARTBEAT`, `ACK`, `STATE_REQUEST`, `RESPONSE` y cualquier paquete enviado por `sendDirect()` o `enqueue()`. `STATE_REPORT` del emisor V4 ya no llama manualmente a `IoTAuth::signPacket()`, evitando doble TLV HMAC. El formato wire no cambia: la autenticación continúa usando el flag `IOT_FLAG_AUTHENTICATED` y el TLV `0xF0` de cuatro bytes.

La respuesta a una CONFIG se construye en `IoTConfigHandler` y se firma mediante `IoTNode::sendDirect()`. Si una CONFIG cambia la política de autenticación, la respuesta de esa operación usa la política anterior; el cambio se aplica después de persistirla y queda activo para los paquetes siguientes.

## Frontera MQTT, Discovery y alarma

MQTT no forma parte de `IoTProtocol`. `receptor_central_v4` configura el cliente durante `setup()`, pero difiere el primer `connect()` hasta `loop()`: cada iteración atiende primero `buzzer.loop()` y `node.loop()`, y solo después puede entrar en el manager MQTT. El primer intento espera 1 segundo, no se realiza con la bocina activa y, si falla, conserva LOCAL con sondeo cada 5 minutos; en HA se mantiene el backoff de 15 segundos y el retorno a LOCAL tras tres fallos. `PubSubClient::connect()` continúa siendo una operación síncrona durante el intento, por lo que la ausencia de bloqueo en una ráfaga que coincida con una reconexión sigue requiriendo prueba de integración en hardware.

El adapter MQTT publica los topics V4 y los topics V3 de alarma en paralelo. El LWT y la disponibilidad de Discovery usan `casa/alarma/estado`; MOTION publica `detectado`, TIMBRE publica `presionado`, y los comandos de bocina/modo y sus estados aceptan/publican ambos namespaces. `mqtt_discovery.cpp` publica siete configuraciones Home Assistant retained después de conectar, con `unique_id` bajo `central_alarma_*` para que la migración no reutilice los identificadores de la V3 congelada. Los topics de runtime y la IP estática `192.168.0.201` son compartidos deliberadamente para la migración, por lo que V3 y V4 no deben ejecutarse simultáneamente. Ninguno de estos payloads se incorpora al wire format ni a `IoTNode`.

MOTION conserva 1000 ms y solo activa la bocina en `armado`; TIMBRE conserva 500 ms y activa la bocina también en `desarmado`. El temporizador único se compara con una diferencia signed de `millis()` y sigue siendo no bloqueante. No se define prioridad ni política de cola para simultaneidad porque no existe una política equivalente documentada en V3.

## Estado de validación de este cambio

La suite host continúa siendo la del codec/TLV/CRC y no demuestra `IoTNode`, MQTT, Discovery ni hardware. La validación requerida para cerrar U-16/U-18 en el gate sigue incluyendo broker caído/recuperado, LWT retained, recepción de dos eventos y observación de ACK/deduplicación. V3 permanece congelada.

## Capacidades explícitamente fuera de alcance

Este cambio no implementa Capability Discovery, Event Log, AES, DHCP ni un flujo CONFIG/COMMAND nuevo.

## Validación y limitaciones

Los tests host existentes cubren el codec binario, TLV, CRC, longitudes, versión y prioridades. No se añadieron simulaciones de hardware ni un falso UDP para aparentar cobertura: `IoTNode` depende de `WiFiUDP`/ESP8266 y `IoTAuth` depende de BearSSL, por lo que el entorno `native` actual no puede probar de forma honesta el orden de efectos, el canal reliable, registry, deduplicación o HMAC.

La validación de firmware requiere compilación PlatformIO para ESP8266 y, para cerrar el gate de la matriz, pruebas con dos nodos reales o un simulador UDP validado. Sigue pendiente medir en hardware pérdida/reintentos, reinicio con `BOOT_ID`, replay fuera de ventana, WiFi/broker caídos, consumo de RAM/flash y el comportamiento de la configuración remota. V3 permanece congelada y no forma parte de este cambio.
