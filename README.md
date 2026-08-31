# wifi_PIR — Red IoT de Sensores por WiFi/UDP

Sistema modular de alarma y sensores IoT usando ESP8266 con protocolo propio sobre UDP en LAN WiFi.

## Arquitectura

```
     ┌── PIR01 (D1 Mini, 192.168.0.200)
     ├── PIR02 (futuro)
     ├── TEMP01 (futuro)
     └── PUERTA01 (futuro)
           │
           │  WiFi LAN / UDP puerto 4210
           ▼
  ┌────────────────────────┐
  │   RECEPTOR CENTRAL     │──── MQTT (opcional) ──► Home Assistant
  │   NodeMCU v2           │
  │   192.168.0.201        │
  └────────────────────────┘
           │
        Bocina + LED
```

## Versiones

| Versión | Estado | Descripción |
|---------|--------|-------------|
| **V3.5.1** | ✅ Producción | Protocolo texto, ACK async, modo LOCAL/HA |
| **V4.3** | 🔧 Desarrollo | Protocolo binario (IoTProtocol), TLV, CRC16, HMAC, LittleFS |

## Documentación

| Documento | Contenido |
|-----------|-----------|
| [**ARCHITECTURE.md**](docs/ARCHITECTURE.md) | Cómo funciona todo el sistema (emisor, receptor, protocolo, modos) |
| [**ANALISIS_INICIAL_HALLAZGOS.md**](docs/ANALISIS_INICIAL_HALLAZGOS.md) | Snapshot auditable del análisis, hallazgos y método para comparar ideas futuras |
| [**universal-protocol/**](docs/universal-protocol/) | Meta-prompt, unificación y auditoría de los drafts para diseñar el protocolo universal |
| [**INFORME_DRAFTS_RESTANTES.md**](docs/universal-protocol/INFORME_DRAFTS_RESTANTES.md) | Trazabilidad de los cinco drafts restantes, bugs históricos y backlog futuro |
| [**CHANGELOG.md**](docs/CHANGELOG.md) | Historial completo de versiones (V3.1 → V4.3) |
| [**BUGS_FIXED.md**](docs/BUGS_FIXED.md) | Bugs históricos con estados de evidencia, causas y reglas preventivas |
| [**PLAN_EJECUCION_FUTURA.md**](docs/PLAN_EJECUCION_FUTURA.md) | Estado técnico y orden seguro para continuar el desarrollo |
| [**ROADMAP.md**](docs/ROADMAP.md) | Mejoras futuras con instrucciones concretas paso a paso |

## Quick Start

### 1. Clonar y configurar

```bash
git clone https://github.com/ydiaz1699/wifi_PIR.git
cd wifi_PIR
cp secrets.h.template secrets.h
# Editar secrets.h con tus credenciales WiFi/MQTT/AUTH_KEY
```

### 2. Flashear emisor (V3.5.1 producción)

```bash
cd emisor_pir
pio run -t upload
```

### 3. Flashear receptor (V3.5.1 producción)

```bash
cd receptor_bocina
pio run -t upload
```

### 4. Verificar

Abrir monitor serial del receptor:
```bash
pio device monitor -b 115200
```

Presionar timbre/activar PIR → debe aparecer:
```
[I] Evento nuevo: PIR01 #1 tipo=TIMBRE (192.168.0.200)
[I] TIMBRE: sonido corto de aviso
```

## Estructura del proyecto

```
wifi_PIR/
├── docs/                     ← DOCUMENTACIÓN
│   ├── ARCHITECTURE.md
│   ├── ANALISIS_INICIAL_HALLAZGOS.md
│   ├── universal-protocol/
│   │   ├── META_PROMPT.md
│   │   └── INFORME_UNIFICACION.md
│   ├── CHANGELOG.md
│   ├── BUGS_FIXED.md
│   ├── PLAN_EJECUCION_FUTURA.md
│   └── ROADMAP.md
│
├── secrets.h.template        ← Copiar a secrets.h (WiFi + MQTT + AUTH_KEY)
├── network_config.h          ← Gateway, subnet, puerto UDP
│
├── emisor_pir/               ← EMISOR V3.5.1 (PRODUCCIÓN)
├── receptor_bocina/          ← RECEPTOR V3.5.1 (PRODUCCIÓN)
│
├── lib/IoTProtocol/          ← BIBLIOTECA V4.3 (DESARROLLO)
├── emisor_pir_v4/            ← EMISOR V4.3
└── receptor_central_v4/      ← RECEPTOR V4.3
```

## Hardware

| Dispositivo | Board | Pines |
|-------------|-------|-------|
| Emisor PIR+Timbre | D1 Mini | PIR=D2, Timbre=D3 (pullup) |
| Receptor Central | NodeMCU v2 | Buzzer=D5, LED=D6 |

## Principios de diseño

1. **UDP primero** — la alarma local funciona sin internet/MQTT
2. **Nunca bloquear** — ninguna función puede freezar la recepción UDP
3. **Sensores independientes** — PIR y timbre no se interfieren
4. **Receptor genérico** — procesa cualquier sensor por tipo de evento
5. **Modo LOCAL resiliente** — si MQTT muere, todo sigue funcionando

## OTA (Over-The-Air)

Requiere regla de firewall en Windows:
```powershell
# PowerShell como admin:
New-NetFirewallRule -DisplayName "PlatformIO OTA" -Direction Inbound -Protocol UDP -LocalPort 1024-65535 -Action Allow
New-NetFirewallRule -DisplayName "PlatformIO OTA TCP" -Direction Inbound -Protocol TCP -LocalPort 1024-65535 -Action Allow
```

Luego:
```bash
pio run -d receptor_bocina -e receptor_bocina_ota -t upload
```
