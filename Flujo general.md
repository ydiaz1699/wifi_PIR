Claro. Viendo los dos códigos juntos —emisor_pir_unificado y receptor_central_unificado—, el flujo completo queda así.

Flujo general
                 ┌──────────────────────────┐
                 │     EMISOR PIR/TIMBRE    │
                 │       ID = 0x02          │
                 └────────────┬─────────────┘
                              │
                              │ IoTProtocol / UDP 4210
                              ▼
                 ┌──────────────────────────┐
                 │      CENTRAL IoT         │
                 │       ID = 0x01          │
                 └────────────┬─────────────┘
                              │
                ┌─────────────┴─────────────┐
                │                           │
                ▼                           ▼
          Alarma local                  MQTT / HA
          Bocina + LED                 Home Assistant

1. Arranque del emisor
emisor_pir_unificado
        │
        ▼
Serial.begin()
        │
        ▼
Configura watchdog
        │
        ▼
Configura pines
 ┌──────┴────────┐
 │               │
 PIR             TIMBRE
 D2              D3
 INPUT           INPUT_PULLUP
 │               │
 └──────┬────────┘
        ▼
IoTStorage.begin()
        │
        ▼
loadConfig()
        │
        ├── heartbeat
        ├── antirebote
        ├── deviceName
        └── authEnabled
        │
        ▼
Obtiene BOOT_ID
        │
        ▼
Conecta WiFi
        │
        ▼
Inicializa OTA
        │
        ▼
node.begin(BOOT_ID)
        │
        ▼
Configura callback
        │
        ▼
Configura heartbeat
        │
        ▼
ConfigHandler
        │
        ▼
Configura HMAC
        │
        ▼
HELLO ──────────────────────────────► CENTRAL


El emisor queda entonces en:

┌─────────────────────────────┐
│     LOOP DEL EMISOR         │
├─────────────────────────────┤
│ Watchdog                    │
│ WiFi                        │
│ OTA                         │
│ IoTNode.loop()              │
│ Leer PIR                    │
│ Leer TIMBRE                 │
└─────────────────────────────┘

2. Arranque de la central

La central hace:

receptor_central_unificado
        │
        ▼
Serial
        │
        ▼
Watchdog
        │
        ▼
LED + Buzzer
        │
        ▼
IoTStorage
        │
        ▼
loadConfig()
        │
        ▼
BOOT_ID
        │
        ▼
Configura HMAC
        │
        ▼
WiFi
        │
        ▼
node.begin(BOOT_ID)
        │
        ▼
Configura Auth
        │
        ▼
Registra handleIoTPacket()
        │
        ▼
Inicializa MQTT
        │
        ▼
Inicializa OTA
        │
        ▼
STATE_REQUEST broadcast


El último paso es importante:

CENTRAL
   │
   │ STATE_REQUEST
   │ dst = BROADCAST
   │ UDP 4210
   ▼
TODOS LOS NODOS


Por tanto, aunque el emisor ya haya enviado su HELLO, la central además le solicita explícitamente su estado.

3. Descubrimiento inicial

El emisor manda:

HELLO
src = 0x02
dst = CENTRAL
device_type = PIR_SENSOR
device_name = "PIR Entrada"
bootId = XXXX
seq = XXXX


La central lo recibe:

UDP
 │
 ▼
IoTNode
 │
 ├── versión
 ├── dirección
 ├── autenticación
 ├── registry
 ├── ACK
 ├── deduplicación
 │
 ▼
handleIoTPacket()
 │
 ▼
MsgType::HELLO


Entonces:

Central
   │
   ├── registra dispositivo 0x02
   ├── obtiene nombre
   └── MQTT:
          casa/iot/device_02/name
          casa/iot/device_02/status

4. Detección PIR

Cuando el PIR pasa:

LOW → HIGH


el emisor detecta:

if (pirActual && !pirAnterior)


Después:

PIR
 │
 ▼
antirebote
 │
 ▼
EventCode::MOTION
 │
 ▼
node.sendEvent()


El paquete sale aproximadamente como:

EVENT
├── src = 0x02
├── dst = central
├── bootId
├── seq
├── EVENT_TYPE = MOTION
└── EVENT_VALUE = 1


Flujo:

PIR
 │
 ▼
Emisor
 │
 │ EVENT / MOTION
 │
 ▼
UDP
 │
 ▼
Central IoTNode
 │
 ├── Auth
 ├── dedup
 ├── ACK
 └── callback
        │
        ▼
handleIoTPacket()
        │
        ▼
EVENT
        │
        ├───────────────► MQTT
        │
        ▼
¿alarma armada?
        │
     ┌──┴───┐
    SI      NO
     │       │
     ▼       ▼
  BOCINA    nada


Si está armada:

MOTION
 ↓
DURACION_BOCINA_MOTION_MS
 ↓
1000 ms
 ↓
Buzzer ON
 ↓
LED ON
 ↓
Buzzer OFF
 ↓
LED OFF

5. Detección del timbre

El timbre usa INPUT_PULLUP.

Por tanto:

HIGH = libre
LOW  = presionado


Cuando detecta:

HIGH → LOW


el flujo es:

Timbre
  │
  ▼
antirebote
  │
  ▼
EventCode::TIMBRE
  │
  ▼
node.sendEvent()
  │
  ▼
UDP
  │
  ▼
Central
  │
  ▼
handleIoTPacket()
  │
  ├── MQTT: casa/alarma/timbre
  │
  ▼
debeActivarBocina()
  │
  ▼
SIEMPRE
  │
  ▼
Buzzer 500 ms


Aquí hay una diferencia importante respecto al PIR:

PIR / DOOR
     │
     └── depende de "armado"

TIMBRE
     │
     └── suena incluso "desarmado"

6. Heartbeat

Cada 60 segundos el IoTNode del emisor genera:

HEARTBEAT
├── uptime
├── RSSI
├── free heap
├── queue depth
├── TX count
└── ACK timeouts


Flujo:

EMISOR
   │
   │ HEARTBEAT
   ▼
CENTRAL
   │
   ▼
IoTNode
   │
   ▼
handleIoTPacket()
   │
   ▼
publishHeartbeat()
   │
   ├── uptime
   ├── rssi
   ├── heap
   ├── tx_count
   └── ack_timeouts
   │
   ▼
MQTT


Por ejemplo:

casa/iot/device_02/uptime
casa/iot/device_02/rssi
casa/iot/device_02/heap
casa/iot/device_02/tx_count
casa/iot/device_02/ack_timeouts

7. STATE_REQUEST / STATE_REPORT

Este es el flujo de sincronización:

                 CENTRAL
                    │
                    │ STATE_REQUEST
                    ▼
              ┌───────────┐
              │  EMISOR   │
              └─────┬─────┘
                    │
                    │ STATE_REPORT
                    ▼
                 CENTRAL


El emisor construye:

STATE_REPORT
├── STATE_MOTION
├── STATE_BUTTON
├── UPTIME_SEC
├── RSSI_VAL
├── FREE_HEAP
├── TX_COUNT
└── ACK_TIMEOUTS


La central lo interpreta:

STATE_REPORT
      │
      ├── motion
      ├── button
      └── diagnóstico
             │
             ▼
            MQTT


Por ejemplo:

casa/iot/device_02/state/motion = active
casa/iot/device_02/state/button = released

8. CONFIG remota

La central puede mandar:

CONFIG


al emisor.

Flujo:

CENTRAL
   │
   │ CONFIG
   ▼
EMISOR
   │
   ▼
IoTConfigHandler
   │
   ├── valida
   ├── aplica
   └── persiste LittleFS
   │
   ▼
onConfigApplied()
   │
   ├── cambia heartbeat
   ├── cambia antirebote
   └── cambia HMAC


Por eso el emisor puede cambiar configuración sin recompilar.

9. Cambio de modo de alarma mediante MQTT

Home Assistant puede mandar:

armado


o:

desarmado


a:

casa/alarma/modo/set


La central recibe:

MQTT
 │
 ▼
mqttCallback()
 │
 ▼
modoAlarma = "armado"


y publica:

casa/alarma/modo/state


Después:

PIR EVENT
    │
    ▼
Central
    │
    ▼
modoAlarma
    │
 ┌──┴──────┐
 │         │
armado   desarmado
 │         │
 ▼         ▼
Bocina     no bocina


El sensor no necesita saber si la alarma está armada.

10. MQTT y modo LOCAL/HA

La central tiene dos estados:

MODO_LOCAL
MODO_HA


Arranca:

LOCAL


Después de aproximadamente 1 segundo:

¿Broker MQTT disponible?
       │
    ┌──┴───┐
   SI      NO
    │       │
    ▼       ▼
   HA     LOCAL


Si está en HA:

MQTT conectado
      │
      ▼
mqtt.loop()
      │
      ├── comandos
      ├── estados
      ├── eventos
      └── heartbeat


Si el broker desaparece:

HA
 │
 ├── intento cada 15 s
 │
 ├── fallo
 │
 ├── fallo
 │
 ├── fallo
 │
 ▼
LOCAL


En LOCAL sigue funcionando:

PIR
TIMBRE
UDP
ACK
Bocina
Estado IoT


MQTT no es necesario para que la alarma básica funcione.

11. Home Assistant Discovery

Cuando la central consigue MQTT:

MQTT conectado
      │
      ▼
publicarDiscovery()
      │
      ├── PIR
      ├── Timbre
      ├── Online
      ├── Bocina
      ├── Modo
      ├── Uptime
      └── IP


Home Assistant recibe configuraciones en:

homeassistant/binary_sensor/...
homeassistant/switch/...
homeassistant/select/...
homeassistant/sensor/...


Y esas entidades consumen principalmente los topics V3:

casa/alarma/evento
casa/alarma/timbre
casa/alarma/estado
casa/alarma/bocina/set
casa/alarma/bocina/state
casa/alarma/modo/set
casa/alarma/modo/state


Esto es lo que mantiene la compatibilidad con V3.

12. Flujo de autenticación

Cuando HMAC está habilitado:

EMISOR
   │
   │ paquete
   ▼
IoTNode
   │
   ▼
verificar HMAC
   │
 ┌─┴─────────┐
 │           │
válido     inválido
 │           │
 ▼           X
continúa    descarta
 │
 ▼
registry / ACK / dedup / callback


En la central ocurre lo mismo.

Por tanto:

          ┌─────────────┐
          │ IoTProtocol │
          └──────┬──────┘
                 │
              IoTAuth
                 │
        ┌────────┴────────┐
        │                 │
     Emisor            Central


La autenticación está en la capa de protocolo, no en la lógica PIR.

13. OTA

En el emisor:

WiFi
 │
 ▼
ArduinoOTA
 │
 ▼
otaEnProgreso() = true
 │
 ▼
NO sensores
NO eventos
NO tráfico aplicación
 │
 └── ArduinoOTA.handle()


Después de terminar:

OTA
 │
 ▼
reinicio
 │
 ▼
BOOT_ID nuevo
 │
 ▼
HELLO
 │
 ▼
normal

14. Flujo completo de un PIR, de extremo a extremo

Este es probablemente el flujo más importante:

┌─────────────┐
│ SENSOR PIR  │
└──────┬──────┘
       │ HIGH
       ▼
┌─────────────┐
│ main.cpp    │
│ digitalRead │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ Antirebote   │
└──────┬──────┘
       │
       ▼
┌─────────────────┐
│ AlarmProfile    │
│ MOTION          │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ IoTNode         │
│ sendEvent()     │
└────────┬────────┘
         │
         ▼
      UDP 4210
         │
         ▼
┌─────────────────┐
│ Central IoTNode │
└────────┬────────┘
         │
         ├── Auth
         ├── ACK
         ├── Dedup
         │
         ▼
┌──────────────────────┐
│ handleIoTPacket()    │
└──────────┬───────────┘
           │
           ├───────────────► MQTT EVENT
           │
           ▼
   debeActivarBocina()
           │
       ┌───┴───┐
       │       │
    armado  desarmado
       │       │
       ▼       X
   Buzzer
       │
       ▼
      LED

15. Flujo completo del sistema

Resumiendo ambos programas en una sola cadena:

                         ┌──────────────────┐
                         │    HOME          │
                         │   ASSISTANT      │
                         └────────┬─────────┘
                                  │
                              MQTT │
                                  ▼
                         ┌──────────────────┐
                         │      CENTRAL     │
                         │                  │
                         │ IoTNode          │
                         │ IoTAuth          │
                         │ MQTT             │
                         │ AlarmProfile     │
                         │ Buzzer           │
                         └────────┬─────────┘
                                  ▲
                                  │
                            UDP 4210
                                  │
                         ┌────────┴─────────┐
                         │                  │
                    EVENT/HB/STATE      ACK/CONFIG
                         │                  │
                         ▼                  ▲
                  ┌────────────────────────────┐
                  │       EMISOR PIR           │
                  │                            │
                  │ IoTNode                    │
                  │ IoTAuth                    │
                  │ IoTStorage                 │
                  │ IoTConfigHandler           │
                  │ OTA                         │
                  │                            │
                  │ PIR ──────┐               │
                  │ Timbre ───┤               │
                  └───────────┴────────────────┘


Y conceptualmente las responsabilidades quedan así:

┌─────────────────────────────────────────────┐
│ EMISOR                                      │
│                                             │
│ "¿Qué detecté?"                             │
│                                             │
│ PIR      → MOTION                           │
│ Timbre   → TIMBRE                           │
│ Estado   → STATE_REPORT                     │
│ Vida     → HEARTBEAT                        │
└──────────────────────┬──────────────────────┘
                       │
                       │ IoTProtocol
                       ▼
┌─────────────────────────────────────────────┐
│ IoTNode                                     │
│                                             │
│ transporte                                  │
│ ACK                                         │
│ retransmisión                               │
│ deduplicación                               │
│ registry                                    │
│ heartbeat                                   │
│ autenticación                               │
└──────────────────────┬──────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────┐
│ CENTRAL                                     │
│                                             │
│ "¿Qué hago con lo que detectó?"             │
│                                             │
│ armado/desarmado                            │
│ bocina                                      │
│ LED                                         │
│ MQTT                                        │
│ Home Assistant                              │
│ estado de nodos                              │
└─────────────────────────────────────────────┘


Esa es la idea fundamental de la unificación: el emisor no decide la política de alarma y la central no necesita saber qué modelo concreto de sensor tiene delante. AlarmProfile define el significado del evento e IoTProtocol/IoTNode se encargan del transporte y la fiabilidad.
