# ⏰ Reloj Digital NTP NodeMCU ESP8266

Reloj digital con **LCD 16x2 I2C**, sincronización **NTP vía WiFi** y dígitos grandes renderizados con caracteres personalizados. Diseñado para NodeMCU v2 (ESP8266-12E) con arquitectura no bloqueante y máquinas de estado.

## 🎯 Características

- ✅ **Hora sincronizada por NTP** (sincronización automática cada 1 hora)
- ✅ **Dígitos grandes** en LCD 16x2 (6 segmentos personalizados por dígito)
- ✅ **WiFi con auto-reconnect** (reintentos automáticos cada 30 segundos)
- ✅ **Arquitectura no bloqueante** (máquinas de estado FSM, sin `delay()` en loop)
- ✅ **Indicadores de estado** (W=WiFi, T=Time sync, *=NTP)
- ✅ **Código modular** (WiFiManager, NtpClient, Display separados)
- ✅ **Optimizado para ESP8266** (C++17, -Os, PROGMEM para datos)

## 📋 Requisitos

### Hardware
| Componente | Especificación |
|-----------|---|
| **Microcontrolador** | NodeMCU v2 (ESP8266-12E) |
| **Pantalla** | LCD 16x2 I2C (dirección 0x27) |
| **Conexión I2C** | SDA=GPIO4 (D2), SCL=GPIO5 (D1) |
| **Alimentación** | 5V USB o batería (NodeMCU regula a 3.3V) |
| **Consumo** | ~150 mA WiFi activo, ~50 mA idle |

### Software
- **PlatformIO** (o Arduino IDE)
- **Framework Arduino ESP8266**
- **Librería**: `marcoschwartz/LiquidCrystal_I2C@^1.1.4`
- **C++17** mínimo

## 🚀 Instalación Rápida

### 1. Clonar repositorio
```bash
git clone <repo-url>
cd reloj-ntp-nodemcu
```

### 2. Configurar credenciales
```bash
cp include/secrets.h.template include/secrets.h
```

Editar `include/secrets.h` y completar:
```cpp
#pragma once

namespace secrets {
    inline constexpr std::string_view WIFI_SSID = "tu_ssid_aqui";
    inline constexpr std::string_view WIFI_PASSWORD = "tu_password_aqui";
    
    // Zona horaria (ej: 3600 = UTC+1, -18000 = UTC-5)
    inline constexpr int32_t GMT_OFFSET_SEC = 3600;
    inline constexpr int32_t DAYLIGHT_OFFSET_SEC = 0;
    
    inline constexpr std::string_view NTP_SERVER = "pool.ntp.org";
}
```

### 3. Compilar y subir
```bash
# Build
platformio run

# Upload
platformio run --target upload

# Monitor (ver logs)
platformio device monitor --baud 115200
```

## 📊 Arquitectura

### Máquinas de Estado

```
WiFiManager (no bloqueante):
  DISCONNECTED ──→ CONNECTING ──→ CONNECTED ──→ DISCONNECTED (auto-reconnect cada 30s)
     ↓
  FAILED (timeout 15s)

NtpClient (no bloqueante):
  IDLE ──→ SYNCING ──→ SYNCED (resync cada 1 hora)
     ↓      ↓
  FAILED ──→ Reintentos cada 5 min (max 20)

Display:
  Actualiza cada loop sin delay()
  - Dígitos: 200 ms
  - Parpadeo dos puntos: 500 ms
  - Indicadores: cada ciclo
```

### Flujo Principal
```
setup():
  1. Wire.begin() → I2C en GPIO4/GPIO5
  2. LCD init + caracteres personalizados
  3. WiFiManager.begin(SSID, PASSWORD)
  4. NtpClient ready (IDLE)

loop() (infinito, ~50-100 ms por ciclo):
  1. WiFiManager.update() → FSM conexión
  2. if (WiFi OK) → NtpClient.begin()
  3. NtpClient.update() → FSM sincronización
  4. getLocalTime() cada 200ms
  5. Renderizar display (dígitos + indicadores)
```

## 📁 Estructura del Proyecto

```
reloj-ntp-nodemcu/
├── include/
│   ├── hw.h              # Pines GPIO y constantes LCD
│   ├── log.h             # Macros logging (DEBUG_LOG)
│   ├── display.h         # Patrones PROGMEM, dígitos grandes
│   ├── timekeeping.h     # Struct TimePacked (bitfields)
│   ├── WiFiManager.h     # FSM conexión WiFi
│   ├── NtpClient.h       # FSM sincronización NTP
│   ├── ui.h              # showTime(), showStatus()
│   ├── secrets.h         # ⚠️ NO subir a Git (credenciales)
│   └── secrets.h.template # Plantilla pública
├── src/
│   ├── main.cpp          # setup() + loop()
│   ├── WiFiManager.cpp   # Implementación FSM
│   └── NtpClient.cpp     # Implementación FSM
├── platformio.ini        # Configuración PlatformIO
├── .gitignore            # Excluye secrets.h, .pio/, binarios
└── README.md             # Este archivo
```

## ⚙️ Configuración Avanzada

### Habilitar Debug Logs
Editar `platformio.ini`:
```ini
build_flags =
    -std=c++17
    -Os
    -DDEBUG_LOG
```

Luego compilar y ver en monitor:
```bash
platformio run --target upload && platformio device monitor --baud 115200
```

### Cambiar Servidor NTP
En `include/secrets.h`:
```cpp
inline constexpr std::string_view NTP_SERVER = "time.nist.gov";
```

### Cambiar Dirección LCD I2C
Si tu LCD está en `0x26` en lugar de `0x27`:
1. Editar `include/hw.h`:
```cpp
inline constexpr uint8_t LCD_ADDR = 0x26;  // Cambiar aquí
```
2. Recompilar

## 🐛 Troubleshooting

| Síntoma | Causa | Solución |
|---------|-------|----------|
| **Compilación falla** | `secrets.h` no existe | `cp include/secrets.h.template include/secrets.h` |
| **Display muestra basura** | Slot CGRAM 0 usado | Verificar `display.h`:DIGITS, usar `chars::BLANK` |
| **No conecta WiFi** | SSID/password incorrecto | Revisar `secrets.h`, activar `DEBUG_LOG` |
| **Hora no sincroniza** | WiFi no conectado | Esperar conexión (5-15s), revisar logs |
| **Display congelado** | `delay()` en loop | Revisar `main.cpp`, usar `millis()` |
| **LCD no aparece en I2C** | Dirección incorrecta (0x27?) | Usar `i2cdetect` o cambiar `LCD_ADDR` en `hw.h` |
| **Reset aleatorio** | Sin Watchdog Timer | Agregar `ESP.wdtEnable(WDTO_8S)` en setup() |

## 📈 Monitoreo y Diagnóstico

### Ver heap memory
```cpp
// En loop() o setup():
LOGF("[HEAP] Libre: %d bytes\n", ESP.getFreeHeap());
```

### Compilar y ver tamaño binario
```bash
platformio run
# Output: .pio/build/nodemcuv2/firmware.bin
```

### Conectar serial y ver logs
```bash
platformio device monitor --baud 115200
```

Logs esperados:
```
=== RELOJ NodeMCU ESP8266 NTP ===
Codigo organizado en multiples archivos
[WiFi] Conectando a RED_SSID...
[WiFi] Conectado (IP: 192.168.1.100)
[NTP] Configurando sincronizacion...
[NTP] Hora sincronizada
[NTP] Hora: 14:32:45
```

## 🔧 Problemas Conocidos y Soluciones

### 1. **Sin Watchdog Timer (WDT)**
**Problema**: Si el código se cuelga, el reloj no reinicia.  
**Solución**: Agregar en `src/main.cpp`:setup():
```cpp
ESP.wdtEnable(WDTO_8S);  // Reinicia si loop() tarda > 8 segundos
```

### 2. **Reconexión WiFi lenta**
**Problema**: Reintenta cada 30s sin exponential backoff.  
**Solución**: Próxima versión implementará backoff progresivo.

### 3. **Zona horaria hardcoded**
**Problema**: No se puede cambiar zona horaria sin recompilar.  
**Solución**: Usar SPIFFS + menú interactivo (futura feature).

### 4. **Credenciales WiFi en compile-time**
**Problema**: No se pueden cambiar sin recompilar.  
**Solución**: Implementar Smart Config o Portal Cautivo (futura feature).

### 5. **Sin detección de "NTP desincronizado"**
**Problema**: Si NTP falla, display sigue mostrando hora vieja.  
**Solución**: Agregar indicador "NO SYNC" en display cuando NTP FAILED.

## 💡 Tips de Desarrollo

- **No agregar `delay()` en `loop()`**: Usa `millis()` para timeouts no bloqueantes
- **CGRAM slots 1-8**: Nunca usar slot 0, usar `chars::BLANK` (ASCII 32) para espacios
- **Memoria flash limitada**: 4 MB total, ~2.8 MB disponible. Usar `PROGMEM` para datos
- **C++17 soportado**: NO usar inicialización designada (.field = value), usar constructores
- **Debug en serial**: Activar `DEBUG_LOG` en `platformio.ini` para ver eventos

## 📚 Referencias

- [LiquidCrystal_I2C Library](https://github.com/marcoschwartz/LiquidCrystal_I2C)
- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino)
- [NodeMCU Pinout](https://nodemcu.readthedocs.io/en/release/modules/)
- [PlatformIO Docs](https://docs.platformio.org/)

## 📝 Historial de Cambios

### v1.0.0 (2026-06-21)
- Arquitectura refactorizada con FSM no bloqueante
- Display con dígitos grandes (caracteres personalizados)
- WiFiManager con auto-reconnect
- NtpClient con resync automático cada 1 hora
- Indicadores de estado (W/T/*)

## 📄 Licencia

MIT (ver archivo `LICENSE` si existe)

## ✉️ Contacto / Issues

Si encuentras bugs o tienes sugerencias:
1. Activar `DEBUG_LOG` en `platformio.ini`
2. Reproducir el problema
3. Documentar logs y pasos
4. Reportar en GitHub Issues

---

**Última actualización**: 2026-06-21  
**Autor**: A.A.D.M    
**Estado**: Production Ready ✅
