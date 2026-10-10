# HARDWARE

Hardware real del sistema. Datos extraídos de `emisor_pir_unificado/` y
`receptor_central_unificado/`.

## Placas

| Nodo | Placa PlatformIO | MCU | Notas |
|---|---|---|---|
| Emisor PIR + timbre | `d1_mini` | ESP8266 (4 MB flash, ~80 KB RAM) | WiFi, LittleFS, OTA |
| Receptor central | `nodemcuv2` | ESP8266 | WiFi, LittleFS, OTA, bocina/LED |

Plataforma: `espressif8266`, framework `arduino` para ambos.

## Identidad del emisor

Fuente: `emisor_pir_unificado/include/device_config.h`.

- `MY_DEVICE_ID = 0x02`
- `MY_DEVICE_TYPE = AlarmProfile::DeviceType::PIR_SENSOR` (`0x02`)
- `MY_DEVICE_NAME = "PIR Entrada"`
- OTA: `EMISOR_OTA_HOSTNAME = "wifi-pir-emisor-v4"`, `EMISOR_OTA_PORT = 8266`.

El receptor es `MY_DEVICE_ID = 0x01` (`IOT_DEVICE_CENTRAL`).

## Pines del emisor

Fuente: `emisor_pir_unificado/src/device_config.cpp`.

| Señal | Pin | Modo | Lógica |
|---|---|---|---|
| PIR | `D2` (`PIN_PIR`) | `INPUT` | Activo en **HIGH**; evento por flanco de subida. |
| Timbre | `D3` (`PIN_TIMBRE`) | `INPUT_PULLUP` | Activo en **LOW**; evento por flanco de bajada. |

El estado lógico se sincroniza con el nivel físico en `setup()` para no emitir
un flanco falso al arrancar.

## Red (IPs estáticas)

Fuente: `device_config.cpp` y `network_config.h` (no versionado; plantilla en
`network_config.h.template`).

| Nodo | IP | Rol de red |
|---|---|---|
| Emisor | `192.168.0.200` | `dispositivo_IP`; también `upload_port` OTA. |
| Central | `192.168.0.201` | `central_IP`; destino de eventos y OTA. |
| Gateway | `192.168.0.1` | `redGateway()` en la plantilla. |
| Subnet | `255.255.255.0` | `redSubnet()`. |

- Puerto UDP: **4210** (`IOT_UDP_PORT_DEFAULT` en `network_config.h`, fuente
  única de verdad; `UDP_PORT` del emisor y `PUERTO_UDP` legacy lo referencian).

## Requisitos eléctricos del PIR

- Sensor PIR tipo HC-SR501 (o equivalente): salida digital que se mantiene en
  **HIGH durante cientos de ms a segundos** mientras hay detección (no genera
  flancos limpios como un botón; ver regla en `docs/ROADMAP.md`).
- Alimentación según el módulo (típicamente 5 V con salida compatible 3V3 del
  ESP8266). Lectura por polling de nivel + detección de flanco por software (no
  interrupción); mantener el `loop()` corto para no perder pulsos cortos del
  timbre (hallazgo B14, no bloqueante).
- Timbre cableado a `D3` con `INPUT_PULLUP`, activo en LOW.
