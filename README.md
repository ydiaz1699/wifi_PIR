# wifi_PIR - Alarma PIR + Timbre por WiFi/UDP

Sistema de alarma con sensor PIR y timbre inalámbrico usando ESP8266 (D1 Mini / NodeMCU).

## Arquitectura

```
[Emisor PIR/Timbre]  --UDP-->  [Receptor Bocina]  --MQTT-->  [Home Assistant]
     (D1 Mini)                    (NodeMCU v2)
   192.168.0.200                192.168.0.201
```

## Versión 3.2

### Cambios vs V3.1:
- **Receptor**: Protección anti-bloqueo MQTT — verificación TCP rápida antes de `connect()`
- **Receptor**: No intenta reconectar MQTT mientras la bocina está activa
- **Emisor**: Más reintentos (5) y mayor timeout de ACK (500ms) para sobrevivir bloqueos del receptor
- **Receptor**: Duración del timbre subida a 500ms para mayor audibilidad

## Estructura

```
wifi_PIR/
├── secrets.h.template     ← copiar a secrets.h con tus credenciales
├── network_config.h       ← configuración de red compartida
├── emisor_pir/            ← PlatformIO project (D1 Mini)
│   ├── platformio.ini
│   ├── include/
│   │   ├── device_config.h
│   │   └── logger.h
│   └── src/
│       ├── device_config.cpp
│       └── main.cpp
└── receptor_bocina/       ← PlatformIO project (NodeMCU v2)
    ├── platformio.ini
    ├── include/
    │   ├── alarma.h
    │   ├── config.h
    │   ├── hal.h
    │   ├── logger.h
    │   ├── mqtt_cliente.h
    │   ├── mqtt_discovery.h
    │   ├── ota.h
    │   ├── red_wifi.h
    │   └── state_machine.h
    └── src/
        ├── alarma.cpp
        ├── config.cpp
        ├── hal.cpp
        ├── main.cpp
        ├── mqtt_cliente.cpp
        ├── mqtt_discovery.cpp
        ├── ota.cpp
        ├── red_wifi.cpp
        └── state_machine.cpp
```

## Setup

1. Copiar `secrets.h.template` → `secrets.h` y llenar con tus datos WiFi/MQTT
2. Abrir cada subcarpeta en PlatformIO
3. Flashear emisor_pir al D1 Mini
4. Flashear receptor_bocina al NodeMCU v2

## OTA

El receptor soporta OTA en `192.168.0.201`:
```bash
cd receptor_bocina
pio run -e receptor_bocina_ota -t upload
```
