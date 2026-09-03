# Brief de tarea: Hardening de IoTProtocol V4.1.1

## Contexto

Revisión de código del proyecto de alarma IoT (ESP8266, `lib/IoTProtocol` +
`emisor_pir_unificado` + `receptor_central_unificado`) detectó 6 puntos a
atender, de severidad variable. Ninguno es bloqueante para producción actual,
pero varios son riesgos latentes que conviene cerrar antes de escalar la red
a más nodos o depender más del canal de auth/config remota.

No se incluye el punto sobre `_computeHmac` omitiendo `dst`: al revisar el
código se confirmó que sí está incluido (`headerData[3] = pkt.dst`). Se
mantiene como sugerencia de test (ítem 4 más abajo), no como bug.

---

## Ítem 1 — Escritura no atómica de `config.json` (LittleFS)

**Riesgo:** Un corte de energía durante `IoTStorage::saveConfig()` puede
dejar el archivo con una mezcla de líneas viejas/nuevas (formato
`key=value`), sin checksum ni validación cruzada. `loadConfig()` no detecta
esta corrupción parcial.

**Archivos afectados:**
- `lib/IoTProtocol/IoTStorage.cpp` (`saveConfig()`, `loadConfig()`)
- `lib/IoTProtocol/IoTStorage.h`

**Propuesta:**
- Escribir a un archivo temporal (`/iot/config.json.tmp`) y hacer
  rename atómico al path final (LittleFS soporta `rename()`).
- Opcional: agregar una línea de checksum simple (CRC16 ya disponible en
  `IoTProtocol.cpp`) al final del archivo, y que `loadConfig()` la valide
  antes de aceptar los valores; si falla, usar defaults y loguear.

**Criterio de aceptación:**
- Simular corte de energía (truncar el archivo a la mitad durante un test)
  y verificar que `loadConfig()` no aplica una config parcial/mezclada.
- `saveConfig()` nunca deja el archivo final en estado intermedio.

**Prioridad:** Media-alta (afecta confiabilidad de config remota vía
`IoTConfigHandler`).

---

## Ítem 2 — Fallback de `BOOT_ID` a `micros()` sin FS

**Riesgo:** Si `IoTStorage::begin()` falla, `getBootId()` cae a
`micros() & 0xFFFF`, reintroduciendo el problema de BOOT_ID no determinístico
que V4.1 resolvió. Ocurre justo cuando el storage falla — el peor momento
para perder confiabilidad de dedup tras un crash/reboot loop.

**Archivos afectados:**
- `lib/IoTProtocol/IoTStorage.cpp` (`getBootId()`)

**Propuesta (a decidir con el usuario, no autoimponer una sola opción):**
- Opción A: mantener el fallback pero subir el log a `LOG_ERROR` y publicar
  un evento/telemetría explícito (ej. TLV de diagnóstico) para que la
  central lo vea en MQTT.
- Opción B: si no hay storage, negarse a operar en modo `REQUIRED` de auth
  (forzar un estado degradado explícito) ya que la sesión no es confiable.

**Criterio de aceptación:**
- Con storage forzado a fallar (mock/test), el comportamiento resultante
  (log, telemetría o modo degradado) es visible y documentado.

**Prioridad:** Baja-media (caso borde, pero con impacto en integridad de
dedup).

---

## Ítem 3 — TLVs que fallan silenciosamente por espacio insuficiente

**Riesgo:** `IOT_MAX_PAYLOAD = 64` deja poco margen cuando se suma HMAC (6
bytes) + varios TLV de config/discovery. `addTLV_*` devuelve `bool` pero
varios call sites (p. ej. `sendHello()`, `_sendHeartbeat()`) no comprueban el
retorno. Un TLV que no cupo se pierde sin ningún log ni error.

**Archivos afectados:**
- `lib/IoTProtocol/IoTNode.cpp` (`sendHello()`, `_sendHeartbeat()`, `sendEvent()`)
- `emisor_pir_unificado/src/main.cpp` (`sendStateReport()`)
- Cualquier otro sitio que construya payloads con múltiples TLV opcionales

**Propuesta:**
- Comprobar el retorno de cada `addTLV_*` en los paquetes críticos (mínimo
  `HELLO`, que es reliable y determina el registro del dispositivo) y
  loguear con `LOG_WARN` si algún TLV no entró.
- Evaluar si vale la pena una función helper que acumule "cuántos TLV se
  perdieron" para no repetir el `if` en cada línea.

**Criterio de aceptación:**
- Con un `DEVICE_NAME`/`FW_VERSION` deliberadamente largo, un test confirma
  que el sistema loguea el TLV perdido en vez de fallar en silencio.

**Prioridad:** Media (impacta observabilidad, no la ejecución en sí, pero
esconde bugs de configuración).

---

## Ítem 4 — Test de vector conocido para HMAC

**Riesgo:** No es un bug encontrado, pero `_computeHmac()` es código
criptográfico crítico sin cobertura de test visible en los fragmentos
revisados. Un futuro refactor (ej. cambio de orden de campos en
`headerData`) podría romper la compatibilidad wire sin que nada lo detecte.

**Archivos afectados:**
- `lib/IoTProtocol/IoTAuth.cpp`

**Propuesta:**
- Agregar un test unitario con un vector de entrada fijo (key, packet
  fields, payload) y el HMAC truncado esperado, calculado independientemente
  (ej. con Python/`hmac` + `hashlib.sha256`, truncado a 4 bytes) para
  comparar contra la salida de `_computeHmac`.

**Criterio de aceptación:**
- Test pasa con el vector fijo; cualquier cambio accidental en el orden o
  contenido de `headerData` lo rompe.

**Prioridad:** Media (protección contra regresiones futuras, no un fix).

---

## Ítem 5 — Test dedicado para `sessionChanged` / canal reliable

**Riesgo:** La lógica de manejo de reinicio de sesión remota mientras hay un
paquete reliable en vuelo (`_reliable.expectedBootId`,
`_reliable.expectedBootKnown`, bootstrap de ACK) es la parte con más ramas
condicionales del código y no tiene tests visibles. Es exactamente el tipo
de lógica donde una regresión silenciosa es fácil.

**Archivos afectados:**
- `lib/IoTProtocol/IoTNode.cpp` (`_handleAck()`, `_updateRemote()`,
  `_processIncoming()`)

**Propuesta:** Escribir tests unitarios (con mocks de `WiFiUDP`/tiempo) que
cubran al menos:
1. ACK normal, sesión ya conocida → confirma reliable.
2. Bootstrap: primer ACK de un remoto nuevo, sin `bootId` previo → establece
   sesión.
3. Remoto reinicia mientras hay reliable en vuelo → `sessionChanged=true`,
   `_reliable.expectedBootId` se actualiza, próximo ACK con el nuevo bootId
   confirma correctamente.
4. ACK de sesión vieja (bootId desactualizado) tras el reinicio → se
   descarta como replay, no confirma el reliable.

**Criterio de aceptación:**
- Los 4 casos anteriores tienen test y pasan.

**Prioridad:** Alta (es la lógica más compleja y crítica para la
confiabilidad del protocolo; conviene proteger antes de tocarla de nuevo).

---

## Ítem 6 — Inconsistencia de límite de nombre de dispositivo (20 vs 24)

**Riesgo:** `RemoteDevice::name` es `char[20]` en `IoTNode.h`, pero
`IOT_STORAGE_NAME_MAX = 24` en `IoTStorage.h`. Un nombre de 20-23 caracteres
se guarda bien en storage pero se trunca silenciosamente al llegar por
`HELLO` (`getTLV_string` con `maxLen=20`).

**Archivos afectados:**
- `lib/IoTProtocol/IoTNode.h` (`RemoteDevice::name`)
- `lib/IoTProtocol/IoTStorage.h` (`IOT_STORAGE_NAME_MAX`)

**Propuesta:** Unificar a una sola constante compartida (ej. mover
`IOT_STORAGE_NAME_MAX` a `IoTProtocol.h` y que ambas structs la usen), o
documentar explícitamente por qué difieren si es intencional.

**Criterio de aceptación:**
- Un nombre de 23 caracteres se preserva completo de punta a punta (config
  → HELLO → registry remoto), o el límite queda documentado y validado en
  un solo lugar.

**Prioridad:** Baja (cosmético/UX, pero fácil de arreglar).

---

## Ítem 7 — Confirmar `.gitignore` cubre secretos

**Riesgo:** `secrets.h` e `IOT_AUTH_KEY` se mencionan como "no versionados"
en comentarios, pero no se vio un `.gitignore` en los fragmentos
compartidos. Riesgo de exponer credenciales si alguien los agrega sin
querer.

**Archivos afectados:**
- Raíz del repo (`.gitignore`)

**Propuesta:** Confirmar que existe y cubre `secrets.h`,
`network_config.h` (mencionado en `main.cpp` de central) y cualquier
artefacto de build. Si no existe, crearlo.

**Criterio de aceptación:**
- `git status` tras crear un `secrets.h` de prueba no lo muestra como
  archivo para commitear.

**Prioridad:** Alta (seguridad, es rápido de verificar/arreglar).

---

## Resumen de prioridades sugerido

| # | Ítem | Prioridad |
|---|------|-----------|
| 7 | Confirmar `.gitignore` | Alta |
| 5 | Tests de `sessionChanged`/reliable | Alta |
| 1 | Escritura atómica de config | Media-alta |
| 3 | Validar retorno de `addTLV_*` | Media |
| 4 | Test de vector HMAC | Media |
| 2 | Fallback de BOOT_ID sin FS | Baja-media |
| 6 | Unificar límite de nombre | Baja |

## Fuera de alcance de este brief

- No se propone cambiar el tamaño de `IOT_MAX_PAYLOAD` (64 bytes): es una
  decisión de diseño consciente documentada en el código, no un bug.
- No se audita aquí el firmware completo de OTA ni el broker MQTT/Home
  Assistant; el foco es el core `IoTProtocol` y su consumo en los dos nodos
  revisados.
