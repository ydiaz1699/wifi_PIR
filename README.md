# wifi_PIR — Red IoT de Sensores por WiFi/UDP

Sistema modular de alarma y sensores IoT usando ESP8266/ESP32 con protocolo binario propio.

## Arquitectura V4

```
     ┌── PIR01 (0x02)
     ├── PIR02 (0x03)         Cualquier sensor/botón/actuador
     ├── TIMBRE01 (0x20)      usa la misma biblioteca IoTProtocol
     ├── PUERTA01 (0x40)
     └── TEMP01 (0x60)
           │
           │  WiFi / UDP (binario + CRC16)
           ▼
  ┌──────────────────┐
  │ CENTRAL (0x01)   │──── MQTT ────► Home Assistant
  │   NodeMCU v2     │
  └──────────────────┘
           │
        Bocina / LED
```

## Versiones

| Versión | Descripción |
|---------|-------------|
| V3.3 | Protocolo texto (`PIR01\|1\|TIMBRE`), modo dual LOCAL/HA |
| **V4.0** | Protocolo binario universal (IoTProtocol), TLV, CRC16, cola de eventos, heartbeat, discovery |

## Estructura del proyecto

```
wifi_PIR/
├── secrets.h.template        ← copiar a secrets.h
├── network_config.h          ← red compartida (gateway, subnet, puerto)
│
├── lib/IoTProtocol/          ← BIBLIOTECA REUTILIZABLE
│   ├── IoTProtocol.h         ← Formato de paquete, enums, TLV tags
│   ├── IoTProtocol.cpp       ← CRC16, serialización, TLV read/write
│   ├── IoTNode.h             ← Nodo de red (cola, ACK, heartbeat)
│   ├── IoTNode.cpp           ← Implementación completa
│   └── library.json
│
├── emisor_pir_v4/            ← EMISOR V4 (D1 Mini)
│   ├── platformio.ini
│   ├── include/device_config.h   ← ID, tipo, nombre, pines
│   ├── include/logger.h
│   └── src/
│       ├── device_config.cpp
│       └── main.cpp
│
├── receptor_central_v4/      ← RECEPTOR V4 (NodeMCU v2)
│   ├── platformio.ini
│   ├── include/
│   │   ├── config.h
│   │   ├── hal.h
│   │   ├── logger.h
│   │   ├── event_handler.h
│   │   └── mqtt_manager.h
│   └── src/
│       ├── config.cpp
│       ├── hal.cpp
│       ├── event_handler.cpp
│       ├── mqtt_manager.cpp
│       └── main.cpp
│
├── emisor_pir/               ← EMISOR V3.3 (legacy)
└── receptor_bocina/          ← RECEPTOR V3.3 (legacy)
```

## Protocolo IoTProtocol V4

### Formato del paquete (binario)

```
┌───────┬─────┬──────┬─────┬─────┬──────┬───────┬─────────┬─────────┬───────┐
│ MAGIC │ VER │ TYPE │ SRC │ DST │ SEQ  │ FLAGS │ PAY_LEN │ PAYLOAD │ CRC16 │
│ 2B    │ 1B  │ 1B   │ 1B  │ 1B  │ 2B   │ 1B    │ 2B      │ N B     │ 2B    │
└───────┴─────┴──────┴─────┴─────┴──────┴───────┴─────────┴─────────┴───────┘
```

### Tipos de mensaje

| Tipo | Dirección | Uso |
|------|-----------|-----|
| EVENT | Sensor → Central | PIR, timbre, puerta, humo |
| DATA | Sensor → Central | Temperatura, humedad |
| COMMAND | Central → Actuador | Relé ON/OFF |
| ACK | Cualquiera | Confirmación |
| HEARTBEAT | Sensor → Central | Estoy vivo + RSSI |
| HELLO | Sensor → Central | Discovery automático |

### Payload TLV (Type-Length-Value)

Extensible sin romper compatibilidad:
```
EVENT_TYPE + EVENT_VALUE + RSSI_VAL       ← evento PIR
TEMPERATURE + HUMIDITY + BATTERY_PCT      ← sensor DHT
CMD_STATE + CMD_DURATION + CMD_CHANNEL    ← comando relé
```

## Agregar un nuevo sensor

Solo necesitás cambiar `device_config.h`:

```cpp
#define MY_DEVICE_ID    0x03              // Nuevo ID
#define MY_DEVICE_TYPE  DeviceType::DOOR_SENSOR
#define MY_DEVICE_NAME  "Puerta Garage"
```

Y adaptar el `loop()` para leer tu sensor específico. La comunicación es idéntica.

## Setup

1. Copiar `secrets.h.template` → `secrets.h`
2. Flashear `emisor_pir_v4` al D1 Mini
3. Flashear `receptor_central_v4` al NodeMCU v2
4. El receptor auto-detecta sensores y publica en MQTT

## MQTT Topics (automáticos)

```
casa/iot/device_02/evento      ← PIR01 events
casa/iot/device_02/uptime      ← PIR01 uptime
casa/iot/device_02/rssi        ← PIR01 signal
casa/iot/device_03/evento      ← PIR02 events
casa/iot/device_40/temperatura ← TEMP sensor
casa/iot/central/estado        ← online/offline
casa/iot/alarma/modo/set       ← armado/desarmado
casa/iot/alarma/bocina/set     ← ON/OFF manual
```

## OTA

```bash
pio run -d receptor_central_v4 -e receptor_central_ota -t upload
```
