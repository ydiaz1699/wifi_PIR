## Lo que ya está integrado

La candidata unificada ya incluye:

- Arquitectura V4 con compatibilidad funcional de V3.
- PIR y TIMBRE independientes.
- Protocolo binario con CRC/TLV.
- ACK automático.
- Retransmisiones reliable.
- Deduplicación por `BOOT_ID + SEQ`.
- Protección anti-replay.
- Validación de ACK por sesión.
- HMAC con verificación antes de registry, ACK, deduplicación y callback.
- `BOOT_ID` persistente en LittleFS.
- Cola de eventos.
- Drain UDP limitado a 8 paquetes por iteración.
- MQTT diferido después de atender IoT.
- Modo LOCAL/HA.
- Bocina y compatibilidad MQTT/Home Assistant de V3.
- MQTT Discovery.
- Simulador y pruebas host.
- Candidatos unificados de emisor y central.
- Compilación normal y OTA de ambos dispositivos.

Validaciones actuales:

```text
Tests C++:       10/10
Tests Python:    16/16
Simulador:       10/10
Builds V4:       4/4
```

## Qué recomiendo integrar antes de probar intensivamente

### 1. Corregir la política de cola URGENT

La documentación de `IoTNode.h` indica:

```text
URGENT nunca se descarta
BACKGROUND sí puede descartarse
```

Pero actualmente, si la cola está llena y no hay ningún elemento `BACKGROUND`, un evento nuevo puede ser rechazado aunque sea `URGENT`.

Esto debe corregirse antes de probar ráfagas de PIR/TIMBRE, porque podríamos interpretar una pérdida de evento como un problema de radio cuando en realidad sería un problema de backpressure.

También conviene que el emisor compruebe el valor retornado por:

```cpp
node.sendEvent(...)
```

Ahora registra:

```text
MOTION encolado
```

aunque el evento podría haber sido rechazado.

**Prioridad: alta.**

### 2. Hacer segura la ruta de fallo de LittleFS/BOOT_ID

Actualmente, si LittleFS falla:

- Se intenta formatear automáticamente.
- Se puede perder el contador de arranque.
- El fallback usa `micros() & 0xFFFF`.
- No se comprueba correctamente si la escritura del contador tuvo éxito.

Eso significa que en una condición de almacenamiento degradado no se puede garantizar un `BOOT_ID` único y persistente.

Antes de probar reinicios, hay que decidir y aplicar una política clara:

- No formatear automáticamente un filesystem existente sin diagnóstico.
- Informar explícitamente `STORAGE_DEGRADED`.
- No declarar el `BOOT_ID` como persistente si la escritura falló.
- Garantizar IDs distintos en dos arranques normales consecutivos.
- Documentar el comportamiento cuando el contador llega a `0xFFFF`.

**Prioridad: alta para las pruebas de reinicio y replay.**

### 3. Validar explícitamente el modo de autenticación

La HMAC está implementada, pero ahora:

- La clave viene compilada desde `secrets.h`.
- El `authEnabled` por defecto es `false`.
- El emisor y la central pueden arrancar con políticas diferentes.
- `IoTStorage::loadAuthKey()` existe, pero los firmwares no la usan actualmente.

Para comenzar, recomiendo hacer primero una prueba controlada con:

```text
Emisor:  AUTH DISABLED
Central: AUTH DISABLED
```

Después, en otro ciclo, configurar ambos como:

```text
Emisor:  AUTH REQUIRED
Central: AUTH REQUIRED
```

usando exactamente la misma clave de laboratorio.

No mezclar:

```text
V3 ↔ V4
V4 DISABLED ↔ V4 REQUIRED
```

porque no son escenarios interoperables.

La prueba autenticada debe verificar:

- HMAC válida: aceptada.
- HMAC incorrecta: rechazada.
- HMAC ausente en modo `REQUIRED`: rechazada.
- Paquete inválido: no actualiza registry.
- Paquete inválido: no genera ACK.
- Paquete inválido: no llega al callback.
- Replay: rechazado.

**Prioridad: alta, pero se puede comenzar el smoke test con auth desactivada en una LAN aislada.**

### 4. Comprobar la política real de PIR

Hay una discrepancia entre la documentación histórica de V3 y el código real.

La documentación menciona “re-trigger” mientras el PIR permanece en `HIGH`, pero el código V3 actual solo genera evento en el flanco:

```cpp
LOW -> HIGH
```

La V4 mantiene ese mismo comportamiento.

Por tanto, no recomiendo implementar re-trigger automáticamente sin decidir antes la política final. Para esta primera prueba se debe medir:

1. PIR en reposo.
2. Primera activación.
3. PIR sostenido en `HIGH`.
4. Liberación a `LOW`.
5. Nueva activación.
6. TIMBRE durante PIR sostenido.
7. PIR y TIMBRE simultáneos.

La prueba debe confirmar que:

- Una activación PIR produce un evento.
- Una segunda activación después de volver a `LOW` produce otro.
- TIMBRE no se pierde durante actividad PIR.
- No hay duplicados por rebote.

**Prioridad: alta para paridad funcional.**

### 5. Validar MQTT con broker real

MQTT ya está diferido correctamente después de `node.loop()`, pero `mqtt.connect()` sigue siendo síncrono.

Hay que medir con el broker apagado:

- Tiempo máximo de una iteración.
- Si se pierden ACK.
- Si la bocina continúa funcionando.
- Si los eventos UDP siguen procesándose.
- Qué ocurre durante tres reconexiones fallidas.
- Si vuelve a modo LOCAL correctamente.

Después, con broker activo, validar:

- MQTT Discovery de las 7 entidades.
- Topics V3 conservados.
- LWT `online/offline`.
- Retained messages.
- Cambio armado/desarmado.
- Comando de bocina.
- Recuperación del broker.

**Prioridad: media-alta.** No impide el primer test UDP/PIR/TIMBRE si se prueba inicialmente con el broker apagado o fuera de la ruta.

### 6. Mejorar OTA de la central

La OTA del emisor tiene contraseña:

```cpp
ArduinoOTA.setPassword(OTA_PASSWORD);
```

Pero la central todavía no tiene la misma protección:

- No configura `setPassword()`.
- Su entorno OTA no usa `--auth`.
- Si arranca sin WiFi, no se reinicializa OTA cuando WiFi vuelve.
- La OTA todavía requiere validación de recuperación serial.

Por eso recomiendo:

1. Probar primero ambos equipos por USB.
2. Dejar OTA para el final.
3. Añadir password y configuración equivalente a la central.
4. Validar password incorrecta.
5. Mantener siempre firmware recuperable por USB.

**Prioridad: media para comenzar hardware; alta antes de usar OTA como método principal.**

### 7. Hacer atómica la configuración remota

`IoTConfigHandler` todavía tiene riesgos:

- Modifica la configuración directamente mientras analiza TLVs.
- Si `saveConfig()` falla, la RAM puede quedar modificada.
- Puede aplicar parcialmente una configuración inválida.
- Ejecuta el callback aunque la persistencia falle.
- No existe todavía un flujo central MQTT → CONFIG → RESPONSE completo.
- Cambiar `authEnabled` remotamente no tiene una transición segura entre ambos equipos.

Esto no bloquea el primer test físico, porque la configuración remota todavía no es necesaria, pero sí debe corregirse antes de afirmar que la configuración remota es confiable.

**Prioridad: media, posterior al primer ciclo físico.**

## Lo que no hace falta integrar ahora

No recomiendo añadir todavía estas extensiones:

- Capability Discovery.
- Event Log.
- `COMMAND/RESPONSE` para relés.
- Máquina avanzada de alarma y zonas.
- Sirena intermitente.
- DHCP o discovery de IP.
- AES-GCM.
- Persistencia avanzada del registry.
- CONFIG completo desde MQTT.
- Adaptación automática V3/V4.

Todas están correctamente clasificadas como extensiones futuras o decisiones pendientes. Integrarlas ahora aumentaría el riesgo y retrasaría la validación de la base.

## Orden recomendado para probar los dispositivos

### Fase 1 — Preparación

- Mantener V3 como firmware de recuperación.
- Preparar USB/serial para ambas placas.
- Usar una LAN aislada.
- No tener V3 y V4 activas simultáneamente.
- Confirmar:
  ```text
  Emisor:  192.168.0.200
  Central:  192.168.0.201
  UDP:      4210
  ```
- Usar inicialmente:
  ```text
  AUTH DISABLED
  MQTT fuera de la ruta crítica
  ```

### Fase 2 — Flash por USB

1. Cargar central unificada.
2. Abrir monitor serial.
3. Confirmar WiFi, IP, `BOOT_ID`, UDP y modo auth.
4. Cargar emisor unificado.
5. Confirmar HELLO y ACK.

### Fase 3 — Protocolo mínimo

Probar:

- HELLO.
- HELLO_ACK.
- Un evento MOTION.
- Un evento TIMBRE.
- ACK.
- Bocina.
- Publicación local.
- Estados del dispositivo.

### Fase 4 — Sensores

Probar:

- PIR individual.
- TIMBRE individual.
- PIR sostenido.
- PIR y TIMBRE simultáneos.
- Pulsaciones repetidas.
- Rebote.
- Evento mientras el reliable anterior está esperando ACK.

Registrar en serial:

```text
bootId
seq
ACK recibido
reintentos
ackTimeouts
queueDrops
queueOverflows
duplicates
replays
RTT
```

### Fase 5 — Reinicios y red

Probar:

- Reinicio del emisor con LittleFS intacto.
- `BOOT_ID` diferente.
- `SEQ` reiniciado correctamente.
- WiFi desconectado y reconectado.
- Central desconectada y reconectada.
- Duplicado.
- ACK antiguo.
- ACK con `SRC/SEQ` incorrectos.

### Fase 6 — MQTT y HA

Con broker apagado:

- Confirmar que la bocina y UDP funcionan.
- Medir que MQTT no bloquea de forma problemática.

Con broker activo:

- Discovery.
- LWT.
- Topics V3.
- Estado armado/desarmado.
- Bocina desde MQTT.
- Recuperación tras caída.

### Fase 7 — Auth

Reflashear/configurar ambos con la misma clave y probar:

```text
HMAC correcta
HMAC incorrecta
HMAC ausente
Replay
Duplicado autenticado
```

### Fase 8 — OTA

Solo después de que USB, UDP, sensores, reinicio, MQTT y auth funcionen.

## Conclusión

Para comenzar las pruebas en los dispositivos no hace falta implementar Capability Discovery, AES-GCM, relés ni event log.

Yo integraría primero estas cuatro correcciones:

1. Cola `URGENT` y medición real de eventos rechazados.
2. Manejo seguro de fallo de LittleFS/`BOOT_ID`.
3. Política reproducible de autenticación.
4. Decisión y validación explícita del comportamiento PIR.

Después ya se puede iniciar la prueba hardware básica con `AUTH DISABLED` y MQTT fuera de la ruta crítica. MQTT, configuración remota y OTA se validan en bloques posteriores, sin tener que volver a cargar firmware por cada pequeña modificación.
