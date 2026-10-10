# SOFTWARE

Toolchain, entornos de compilación y cómo construir/subir firmware.

## Toolchain

- **PlatformIO Core** (verificado con 6.1.18). Framework `arduino`, plataforma
  `espressif8266`.
- Cada subproyecto tiene su propio `platformio.ini`; se compilan por separado.
- Librerías propias en `lib/` se comparten vía `lib_extra_dirs = ../lib` y
  `build_flags = -I../` (para incluir `secrets.h` / `network_config.h` de la
  raíz).

## Entornos (envs)

### Emisor — `emisor_pir_unificado/platformio.ini`

- `env:emisor_unificado` — board `d1_mini`, monitor 115200.
- `env:emisor_unificado_ota` — extiende el anterior; `upload_protocol = espota`,
  `upload_port = 192.168.0.200`, `--auth=${sysenv.WIFI_PIR_OTA_PASSWORD}`.

### Receptor — `receptor_central_unificado/platformio.ini`

- `env:receptor_central_unificado` — board `nodemcuv2`, monitor 115200.
  `lib_deps`:
  - `knolleary/PubSubClient@^2.8` (MQTT)
  - `bblanchon/ArduinoJson@^6.21.5`
- `env:receptor_central_unificado_ota` — `espota` a `192.168.0.201`.

### Pruebas — `tests/platformio.ini`

Entorno `native` (host) para tests unitarios; ver `TESTING.md`.

## Librerías propias

- `lib/IoTProtocol/` (V4.3.0, `library.json`): `IoTNode`, `IoTProtocol`,
  `IoTAuth`, `IoTStorage`, `IoTConfigHandler`.
- `lib/AlarmProfile/`: `AlarmProfile.h` (enums de dominio).

## Dependencias de código (no versionadas)

Antes de compilar, copiar las plantillas de la **raíz** y ajustarlas:

```
copy secrets.h.template         secrets.h           (Windows)
copy network_config.h.template  network_config.h    (Windows)
```

`secrets.h` provee WiFi/MQTT, `IOT_AUTH_KEY`/`IOT_AUTH_KEY_LEN` y
`OTA_PASSWORD`; `network_config.h` provee gateway/subnet y `IOT_UDP_PORT_DEFAULT`.
Ambos están en `.gitignore` (ver `PROTOCOL.md`).

## Compilar

```
cd emisor_pir_unificado        && pio run -e emisor_unificado
cd receptor_central_unificado  && pio run -e receptor_central_unificado
```

> Nota: `pio run` puede terminar con exit code 1 por el banner "Obsolete PIO
> Core"; la señal real de éxito es `[SUCCESS]` / `1 succeeded`.

## Subir (OTA)

Exportar la contraseña OTA en el entorno antes de subir (nunca commitearla):

```powershell
$env:WIFI_PIR_OTA_PASSWORD = "<tu_clave_ota>"
pio run -e emisor_unificado_ota -t upload
pio run -e receptor_central_unificado_ota -t upload
```

El emisor usa `OTA_PASSWORD` de `secrets.h` en `ArduinoOTA.setPassword(...)`;
el flag `--auth` del env OTA debe coincidir con esa clave.
