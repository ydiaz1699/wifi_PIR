# ARCHITECTURE.md — proyecto-reloj-ntp-nodemcu

## Patrón general

Máquina de estados no bloqueante con 3 componentes independientes que se
actualizan en cada iteración del `loop()` sin usar `delay()` (excepto las
excepciones ya marcadas en `WiFiManager.cpp`).

```
setup():
  └── Wire.begin(SDA=GPIO4, SCL=GPIO5)
  └── display.init() → LCD 16x2 I2C @0x27
  └── display.initCustomChars() → carga 8 segmentos en CGRAM (slots 1-8)
  └── WiFiManager.begin(SSID, PASSWORD)
  └── NtpClient listo (state=IDLE)

loop() infinito (~50-100 ms por ciclo):
  1. WiFiManager.update()      → FSM conexión WiFi
  2. if (wifi.isConnected())
       NtpClient.begin()       → dispara sincronización si estaba IDLE
  3. NtpClient.update()        → FSM sincronización NTP
  4. TimePacked::fromSystem()  → cada 200ms, lee getLocalTime()
  5. ui.showTime()             → renderiza 4 dígitos grandes + AM/PM + colon
  6. ui.showStatus()           → indicadores W/T/* en esquina inferior derecha
```

## FSM: WiFiManager

```
DISCONNECTED ──begin()──→ CONNECTING ──onStationModeGotIP──→ CONNECTED
     ↑                        │                                  │
     │                   timeout 15s                    onStationModeDisconnected
     │                   (30 intentos)                            │
     │                        ↓                                   ↓
     └──── tras 30s ──────  FAILED                          DISCONNECTED
```

- **Timeout de conexión:** 15 segundos
- **Máx intentos:** 30
- **Reintento tras FAILED:** cada 30 segundos (sin backoff — ver `.ai/SKILL.md`)
- **Duración típica de conexión:** 5–15 segundos (depende de la red)

## FSM: NtpClient

```
IDLE ──begin()──→ SYNCING ──getLocalTime() válido──→ SYNCED
  ↑                  │                                  │
  │            20 reintentos x 500ms                1 hora transcurrida
  │            (10 seg total)                    (resync automático)
  │                  ↓                                  ↓
  └── tras 5 min ── FAILED                             IDLE
```

- **Intervalo de reintento durante SYNCING:** 500ms
- **Máx reintentos:** 20 (10 segundos total antes de FAILED)
- **Validación de éxito:** `getLocalTime()` retorna válido Y `tm_year > 120` (≥2020)
- **Resync automático:** cada 1 hora (3,600,000 ms) mientras esté SYNCED
- **Reintento tras FAILED:** cada 5 minutos (300,000 ms)
- **Duración típica:** 2–5 segundos (una vez WiFi conectado)
- **También vuelve a SYNCING si WiFi se desconecta** mientras estaba SYNCED

## Display (sin FSM propia, actualización por tiempo)

- **Actualización de hora:** cada 200ms desde `getLocalTime()`
- **Parpadeo de dos puntos:** cada 500ms (alterna visible/invisible)
- **Patrón de dígitos:** grilla 3x2 caracteres por dígito, usando slots CGRAM 1-8
  (nunca 0 — ver `.ai/SKILL.md`)
- **Indicadores de estado** en columnas 13-15, fila 1: `W` (wifi conectado),
  `T` (hora sincronizada), `*` (ntp — actualmente redundante con T, ambos
  reflejan `isSynced()`)

## Transiciones de estado críticas (resumen)

| Desde | Evento | A | Dispara |
| --- | --- | --- | --- |
| BOOT | `setup()` completa | WiFi CONNECTING + Display INIT | — |
| WiFi DISCONNECTED | `WiFi.begin()` + `onStationModeGotIP` | WiFi CONNECTED | `NtpClient.begin()` |
| NTP SYNCING | `getLocalTime()` válido | NTP SYNCED | — |
| NTP SYNCED | 1 hora transcurrida | NTP SYNCING (resync) | — |
| NTP SYNCED | WiFi pierde conexión | NTP SYNCING (reintento c/5min) | — |

## Métricas de referencia

| Métrica | Valor estimado |
| --- | --- |
| Boot hasta LCD mostrando hora | 2–3 segundos |
| Conexión WiFi | 5–15 segundos |
| Sincronización NTP (post-WiFi) | 2–5 segundos |
| Jitter visual del display | ±200 ms |
| Heap usado en runtime | ~15–20 KB |
