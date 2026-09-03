# Operaciones y validación reproducible

Este documento conserva procedimientos operativos que no pertenecen al wire format ni al núcleo `IoTProtocol`. Los comandos deben ejecutarse desde la raíz del repositorio y después de revisar `git status --short`.

## 1. Preflight

```bash
git status --short
git branch --show-current
```

No sobrescribir cambios del usuario. Antes de flashear o aplicar un patch, revisar el diff y confirmar la placa, el puerto, la red y el entorno PlatformIO.

## 2. Compile check host actual

El check aislado está en `tests/host_compile/`. Usa las fuentes reales de `lib/IoTProtocol` y `lib/AlarmProfile`, pero stubs host para Arduino/ESP8266/WiFiUDP.

Valida:

- conversiones wire de `AlarmProfile`;
- TLV, round-trip, CRC y truncamiento;
- compilación/enlace de `IoTNode`;
- `begin()`, `sendHello()`, `sendEvent()`, cola y reliable simulado.

```bash
g++ -std=c++14 -Wall -Wextra \
  -Itests/host_compile/compat \
  -Ilib/IoTProtocol \
  -Ilib/AlarmProfile \
  tests/host_compile/check.cpp \
  tests/host_compile/compat/arduino_stub.cpp \
  lib/IoTProtocol/IoTProtocol.cpp \
  lib/IoTProtocol/IoTNode.cpp \
  -o /tmp/wifi_pir_host_check

/tmp/wifi_pir_host_check
```

Resultado esperado:

```text
OK: host compile check wifi_PIR
```

Este check no demuestra WiFi, UDP físico, MQTT, OTA ni hardware. No confundir `tests/host_compile/compat/Arduino.h` con el stub mínimo histórico `tests/host_compat/Arduino.h`: el primero es el adaptador ampliado para `IoTNode`.

## 3. Tests PlatformIO existentes

El entorno host original de `tests/` continúa siendo la referencia para los tests Unity del codec. Si PlatformIO está instalado:

```bash
pio test -d tests -e native_test
```

La suite debe ejecutarse como una sola corrida, no en modo watch. Si el entorno o las dependencias no están disponibles, registrar el bloqueo; no afirmar que pasó.

## 4. Patch histórico de seguridad

El archivo `v4.3.1-security.patch` no existe en el árbol actual. No crear un patch ficticio copiando índices o fences de una conversación anterior.

Si en el futuro existe un patch raw contra esta revisión exacta:

```bash
git status --short
git apply --check /ruta/al/patch
# Solo después de revisar la salida:
git apply /ruta/al/patch
git diff --check
git diff --stat
git diff
```

Si `git apply --check` falla, detenerse y regenerar el patch contra el código actual. No aplicar a ciegas ni resolver conflictos borrando cambios.

La aplicación, commit y push requieren revisión humana. No agregar temporalmente `*.patch` a `.gitignore` sin una decisión explícita.

## 5. Compilación de proyectos actuales

Las rutas actuales de desarrollo unificado son:

```text
emisor_pir_unificado/
receptor_central_unificado/
```

La línea V3 conservada está bajo:

```text
legacy/emisor_pir/
legacy/receptor_bocina/
```

Antes de compilar, enumerar los entornos reales:

```bash
pio project config --json-output -d emisor_pir_unificado
pio project config --json-output -d receptor_central_unificado
```

Después usar el entorno que realmente aparezca en cada `platformio.ini`. No reutilizar comandos históricos para `emisor_pir_v4` o `receptor_central_v4`, porque esas rutas no forman parte del árbol actual.

## 6. MQTT/Home Assistant

Un log de `publish()` no demuestra que Home Assistant recibió discovery. La validación debe registrar, sin secretos:

1. broker y topic usados;
2. payload retained observado;
3. JSON válido;
4. availability/LWT;
5. entidad creada o actualizada en Home Assistant;
6. resultado después de reiniciar broker o HA.

Ejemplo de inspección local, ajustando host y topic a la configuración real:

```bash
mosquitto_sub -h <BROKER_HOST> -t 'homeassistant/#' -v -C 20
```

No copiar IPs históricas del draft sin verificar gateway, subnet, ruta y broker reales. No afirmar “V3.5.2 verificada” solo por observar una llamada a `publish()`.

La regla operativa sigue siendo: UDP y la alarma local tienen prioridad; la reconexión MQTT debe tener backoff y no puede bloquear indefinidamente el loop.

## 7. OTA

OTA es mantenimiento externo al wire protocol. Antes de una prueba:

- confirmar hostname, placa y entorno correctos;
- mantener credencial OTA separada de WiFi, MQTT y HMAC;
- probar primero recuperación serial/USB;
- confirmar que no hay una alarma crítica activa;
- verificar firewall y red desde el PC real;
- registrar resultado, versión y recuperación.

La configuración histórica de firewall de Windows no garantiza por sí sola que OTA funcione. ArduinoOTA no debe describirse como rollback garantizado en ESP8266: una actualización interrumpida puede requerir recuperación física.

## 8. Pruebas de red

Para una incidencia UDP, medir antes de cambiar firmware:

```bash
ip route
ip addr
```

En cada equipo registrar gateway, subnet, IP del broker y ruta entre emisor y central. Verificar aislamiento de clientes, bridge/NAT, firewall y acceso al puerto UDP. B622, DMZ y reservas DHCP de los drafts son hipótesis hasta observarlas en la red real.

## 9. Sirena futura

La sirena intermitente permanece pendiente. Antes de implementarla hay que definir `sirenOn()`, `isBusy()`, tiempos, prioridad frente a TIMBRE/MOTION, apagado manual y pruebas de rollover de `millis()`. No copiar el bloque histórico directamente al firmware.

## 10. Evidencia y cierre

Después de cada operación:

```bash
git diff --check
git status --short
```

Registrar separadamente:

```text
código existente
compilación
prueba host
simulador
broker/HA
OTA
hardware
```

Una operación que no se ejecutó debe quedar como `PENDIENTE`, no como `VERIFICADA`.


## 11. Gate de la central unificada

Correcciones aplicadas en la revisión 2026-09-02:

- orden del loop: WiFi → `IoTNode.loop()` → `buzzer.loop()` → STATE_REQUEST pendiente → MQTT;
- `mqttDisponible` se limpia al perder WiFi y refleja `mqtt.loop()` junto con `mqtt.connected()`;
- la recuperación HA→LOCAL hace un primer sondeo 60 s después de tres fallos y luego conserva el intervalo LOCAL de 5 min;
- los comandos MQTT V3/V4 pasan por handlers centralizados, pero todavía no tienen `CMD_ID` ni deduplicación semántica;
- STATE_REQUEST se conserva pendiente si no hay WiFi y se emite hasta tres veces después de una conexión, en t=0 s, t=3 s y t=10 s; una reconexión MQTT reinicia también esa ventana para recuperar estados perdidos durante la caída del broker; los plazos son rollover-safe frente a `millis()`;
- `STATE_ALARM` y `STATE_FLOOD` tienen topics de estado explícitos;
- EVENT sin `EVENT_TYPE`, códigos desconocidos y eventos sin política acústica no activan la bocina;
- TAMPER queda explícitamente en modo publicación/log sin bocina hasta aprobar su política de seguridad.

Esto no prueba todavía PubSubClient, MQTT/HA, STATE_SYNC en nodos reales ni hardware. La deduplicación de comandos MQTT y la política final de TAMPER siguen siendo decisiones de producto pendientes.



## 12. Gate de integración del emisor PIR V4

Correcciones aplicadas en la revisión de integración:

- `pirAnterior` y `timbreAnterior` usan semántica lógica (`true` = activo) y se inicializan desde los pines después de `pinMode()`. El nivel presente durante el arranque no se interpreta como un flanco nuevo.
- El antirrebote del PIR continúa viniendo de `IoTStorage::config().antireboteMs`; el del timbre usa `ANTIREBOTE_TIMBRE_MS` del perfil de hardware. `CFG_ANTIREBOTE_MS` no reconfigura el timbre porque no existe un campo/contrato separado para ello.
- El emisor instala la política de autenticación, el handler de configuración, el callback de paquetes y el heartbeat antes de enviar HELLO. HELLO usa la cola reliable de `IoTNode`; no se añade un HELLO periódico sin una política de intervalo.
- `STATE_BUTTON=1` significa timbre presionado y `STATE_BUTTON=0` liberado.
- `IoTConfigHandler` envía `RESPONSE` antes de ejecutar el callback que cambia la política HMAC. Esa transición es deliberada: la respuesta usa la política anterior. La central todavía no implementa el flujo CONFIG/RESPONSE completo ni matching por `CMD_ID`.
- `IoTNode` mantiene la liveness por `lastSeen` de cualquier paquete válido, usando `IOT_STALE_TIMEOUT_MS=90000` e `IOT_OFFLINE_TIMEOUT_MS=180000`. La constante duplicada `HEARTBEAT_TIMEOUT_MS` de la central fue retirada; convertir la liveness en “último heartbeat” requiere un contrato separado.
- OTA permanece pausada para sensores y tráfico de aplicación mientras `otaEnProgreso()` es verdadero. No se fuerza `ESP.restart()` desde `onEnd()` porque el framework ESP8266 controla el reinicio normal; esa ruta debe validarse con hardware y firmware/filesystem OTA.

Pendiente para el gate físico: probar STATE_REQUEST inmediatamente tras arranque, pérdida y recuperación de central, retransmisiones/ACK con pérdida UDP, transición CONFIG/HMAC, OTA durante actividad de sensores y reinicios independientes/simultáneos. Estas pruebas no se consideran verificadas por una compilación.
