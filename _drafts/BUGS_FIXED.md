# Bugs Resueltos

Documentación de todos los bugs encontrados y cómo se resolvieron.
**Propósito**: evitar re-introducir estos errores en futuras versiones.

---

## BUG-001: TIMBRE se pierde cuando MQTT bloquea el loop

**Síntoma**: El emisor envía TIMBRE pero el receptor no responde (sin ACK). El receptor está bloqueado dentro de `mqtt.connect()`.

**Causa raíz**: `PubSubClient::connect()` en ESP8266 bloquea **5 segundos** cuando el broker no responde. Durante ese tiempo, los paquetes UDP del emisor se pierden porque `manejarAlarma()` no corre.

**Por qué `setSocketTimeout(1)` no ayuda**: Ese timeout afecta operaciones de lectura TCP, NO la conexión inicial. `espClient.connect()` tiene un timeout interno del stack TCP de ~5s que no se puede reducir.

**Solución (V3.3)**: Modo dual LOCAL/HA. En modo LOCAL, `manejarMQTT()` no intenta `connect()` jamás. Solo sondea cada 5 minutos (y solo si la bocina no está sonando).

**Regla**: NUNCA llamar a una función bloqueante (connect, http request, delay largo) en el loop principal del receptor. Si algo puede bloquear >100ms, debe ir en una condición que garantice que no hay alarma activa.

---

## BUG-002: PIR y TIMBRE se bloquean mutuamente

**Síntoma**: Si se presiona el timbre mientras el emisor está esperando ACK del PIR (o viceversa), el segundo evento se descarta.

**Causa raíz**: La máquina de estados TX tenía un solo canal (`txState`). Solo un evento podía estar "en vuelo" a la vez. Si `txState != IDLE`, el nuevo evento se descartaba.

**Solución (V3.5)**: ACK asíncrono no-bloqueante con cola de 4 slots paralelos. Cada evento se envía INMEDIATAMENTE sin esperar. Los ACKs se verifican en background. Múltiples eventos pueden estar en vuelo simultáneamente.

**Regla**: Nunca diseñar un emisor con un solo canal de envío si hay múltiples sensores. Los sensores deben ser 100% independientes.

---

## BUG-003: PIR solo detecta una vez, después no responde

**Síntoma**: El PIR envía MOTION una vez, luego no envía más hasta presionar el timbre varias veces.

**Causa raíz DUAL**:
1. **Antirebote muy alto (2000ms)**: si el usuario activa el PIR cada ~1s, la segunda activación cae dentro del antirebote.
2. **Módulo PIR (HC-SR501) se queda en HIGH**: después de detectar, la salida permanece HIGH durante 3-60 segundos (configurable con potenciómetro). El código detecta "flanco de subida" (`pirActual && !pirAnterior`), pero si el PIR ya está en HIGH, no hay nuevo flanco.

**Solución**:
- Antirebote reducido a 200ms
- Para módulo PIR real: ajustar potenciómetro de tiempo al mínimo (~3s)
- Para testing con cable/jumper: soltar (LOW) antes de reconectar (HIGH)

**Regla**: Un sensor PIR NO es un botón. No genera flancos limpios. La detección debe considerar que la salida permanece HIGH por un tiempo configurable del módulo.

---

## BUG-004: `LOCAL` es una macro del SDK ESP8266

**Síntoma**: Error de compilación al usar `enum class ModoConexion { LOCAL, INTELIGENTE }`.

**Causa raíz**: El SDK ESP8266 define `#define LOCAL static` en `c_types.h`. El preprocesador expande `LOCAL` → `static` antes de que el compilador vea el enum.

**Solución**: Renombrar a `MODO_LOCAL` y `MODO_HA`.

**Regla**: NUNCA usar `LOCAL`, `REMOTE`, `ICACHE_FLASH_ATTR`, o cualquier palabra que pueda ser macro del SDK ESP8266 como identificador en código de usuario. En caso de duda, prefixar con algo específico del proyecto.

---

## BUG-005: OTA "No response from device"

**Síntoma**: `pio run -e receptor_bocina_ota -t upload` falla con "No response from device".

**Causa raíz**: El firewall de Windows bloquea la conexión entrante del ESP al PC. El OTA funciona así: PC envía invitación (UDP 8266) → ESP responde en un puerto alto → firewall bloquea esa respuesta.

**Solución**: Abrir puertos en PowerShell (como admin):
```powershell
New-NetFirewallRule -DisplayName "PlatformIO OTA" -Direction Inbound -Protocol UDP -LocalPort 1024-65535 -Action Allow
New-NetFirewallRule -DisplayName "PlatformIO OTA TCP" -Direction Inbound -Protocol TCP -LocalPort 1024-65535 -Action Allow
```

**Regla**: OTA en ESP8266 requiere que el firewall del PC permita conexiones entrantes en puertos altos. Documentar esto para cada nuevo usuario.

---

## BUG-006: `espClient.connect()` bloquea 5 segundos (verificarTCPBroker)

**Síntoma**: `verificarTCPBroker()` se diseñó para hacer un "check rápido" de 1 segundo, pero en la práctica bloquea 5 segundos.

**Causa raíz**: En ESP8266, `WiFiClient::connect(host, port)` tiene un timeout interno de ~5 segundos que NO se puede configurar externamente. `setSocketTimeout()` solo afecta operaciones post-conexión (read/write), no el handshake TCP inicial.

**Solución**: No usar `connect()` como test. En su lugar, no intentar MQTT cuando no es necesario (modo LOCAL con sondeo espaciado).

**Regla**: En ESP8266, CUALQUIER `client.connect()` puede bloquear hasta 5s si el servidor no responde. No hay forma de hacerlo non-blocking con WiFiClient síncrono. La única solución es no llamarlo cuando hay trabajo crítico pendiente.

---

## BUG-007: UDP no funciona entre repetidor y router principal

**Síntoma**: El emisor (en repetidor WiFi canal 6) envía pero el receptor (en router principal canal 1) no recibe. Ping funciona.

**Causa raíz**: Algunos repetidores WiFi filtran paquetes UDP unicast o tienen problemas con ARP resolution cross-AP. El ping funciona porque usa ICMP que tiene tratamiento especial en algunos firmwares de repetidores.

**Solución parcial**: 
- Verificar que "AP Isolation" / "Client Isolation" esté desactivado en el repetidor
- Idealmente, ambos dispositivos en el mismo AP
- Configurar repetidor en modo "AP Bridge" (no "Range Extender")

**Regla**: Para sistemas IoT críticos (alarmas), todos los dispositivos deberían estar en el mismo AP o en un mesh con bridge L2 real.

---

## BUG-008: Deduplicación rota con reinicio del emisor

**Síntoma**: Después de reiniciar el emisor, el receptor puede considerar el primer paquete como "duplicado" si el SEQ coincide con uno anterior.

**Causa raíz (V4.0)**: Solo se guardaba `lastSeq` por remoto. Si el emisor reiniciaba y el nuevo SEQ=1 coincidía con un SEQ viejo=1, se descartaba.

**Solución (V4.1)**: BOOT_ID de 16 bits. Cada boot genera un ID nuevo. Si BOOT_ID cambia → el remoto reinició → resetear ventana de dedup → aceptar todos los paquetes del nuevo boot.

**Regla**: La deduplicación SIEMPRE debe considerar la "sesión" (boot) del emisor, no solo el número de secuencia.

---

## BUG-009: AUTH_KEY expuesta en código versionado

**Síntoma**: La clave HMAC estaba hardcodeada en `main.cpp` y se subió a GitHub.

**Causa raíz**: Copy-paste durante desarrollo. No se pensó en separar credenciales.

**Solución**: Mover a `secrets.h` (que está en `.gitignore`). Definir como `IOT_AUTH_KEY` en `secrets.h.template` para documentar el formato.

**Regla**: Toda credencial (WiFi, MQTT, auth keys) va en `secrets.h`. NUNCA en archivos `.cpp` o `.h` que se versionan.

---

## BUG-010: Lógica de auth inconsistente (doble check)

**Síntoma**: El auth podía aceptar paquetes no autenticados incluso con auth "habilitado", o rechazar paquetes válidos.

**Causa raíz**: Dos puntos de decisión desincronizados:
1. `if (storage.config().authEnabled && ...)` en event_handler.cpp
2. `auth._required` interno de IoTAuth

Si uno decía "auth habilitado" y otro no, el comportamiento era impredecible.

**Solución**: Un solo punto de decisión: siempre llamar `auth.verifyPacket()`. Internamente usa `_required` que se sincroniza al boot con `auth.setRequired(config.authEnabled)`.

**Regla**: Para lógica de seguridad, tener UN SOLO punto de decisión. Nunca duplicar checks de auth en múltiples archivos.
