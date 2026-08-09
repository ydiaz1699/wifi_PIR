/**
 * IoTStorage V4.3 — Implementación LittleFS
 *
 * Usa LittleFS (incluido en ESP8266 Arduino core) para persistir:
 * - Boot counter: archivo simple con un número
 * - Config: JSON minificado con ArduinoJson
 * - Auth key: archivo binario
 */

#include "IoTStorage.h"
#include <LittleFS.h>

// Paths en el filesystem
static const char* PATH_BOOT_COUNT = "/iot/boot_count";
static const char* PATH_CONFIG     = "/iot/config.json";
static const char* PATH_AUTH_KEY   = "/iot/auth.key";

// ============================================================
// Constructor
// ============================================================

IoTStorage::IoTStorage()
    : _bootCount(0), _mounted(false)
{
    _setDefaults();
}

// ============================================================
// Inicialización
// ============================================================

bool IoTStorage::begin() {
    if (!LittleFS.begin()) {
        // Primer uso: formatear
        LittleFS.format();
        if (!LittleFS.begin()) {
            _mounted = false;
            return false;
        }
    }
    _mounted = true;

    // Crear directorio si no existe
    if (!LittleFS.exists("/iot")) {
        LittleFS.mkdir("/iot");
    }

    // Leer boot counter
    _readBootCount();

    return true;
}

// ============================================================
// Boot Counter → BOOT_ID
// ============================================================

uint16_t IoTStorage::getBootId() {
    if (!_mounted) return (uint16_t)(micros() & 0xFFFF);  // Fallback si no hay FS

    _bootCount++;
    _writeBootCount();

    // Wrapping: nunca usar 0
    uint16_t id = (uint16_t)(_bootCount & 0xFFFF);
    if (id == 0) id = 1;
    return id;
}

bool IoTStorage::_readBootCount() {
    if (!LittleFS.exists(PATH_BOOT_COUNT)) {
        _bootCount = 0;
        return true;
    }

    File f = LittleFS.open(PATH_BOOT_COUNT, "r");
    if (!f) return false;

    String line = f.readStringUntil('\n');
    f.close();

    _bootCount = line.toInt();
    return true;
}

bool IoTStorage::_writeBootCount() {
    File f = LittleFS.open(PATH_BOOT_COUNT, "w");
    if (!f) return false;

    f.println(_bootCount);
    f.close();
    return true;
}

// ============================================================
// Configuración — Load/Save JSON simplificado
// (Sin ArduinoJson para no agregar dependencia extra a la lib)
// Formato simple: key=value por línea
// ============================================================

bool IoTStorage::loadConfig() {
    if (!_mounted || !LittleFS.exists(PATH_CONFIG)) {
        _setDefaults();
        return false;
    }

    File f = LittleFS.open(PATH_CONFIG, "r");
    if (!f) return false;

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        int sep = line.indexOf('=');
        if (sep < 0) continue;

        String key = line.substring(0, sep);
        String val = line.substring(sep + 1);

        if (key == "name") {
            val.toCharArray(_config.deviceName, IOT_STORAGE_NAME_MAX);
        } else if (key == "id") {
            _config.deviceId = (uint8_t)val.toInt();
        } else if (key == "central_ip") {
            _config.centralIP.fromString(val);
        } else if (key == "udp_port") {
            _config.udpPort = (uint16_t)val.toInt();
        } else if (key == "heartbeat_ms") {
            _config.heartbeatIntervalMs = (uint32_t)val.toInt();
        } else if (key == "antirebote_ms") {
            _config.antireboteMs = (uint32_t)val.toInt();
        } else if (key == "auth_enabled") {
            _config.authEnabled = (val == "1" || val == "true");
        } else if (key == "auth_key_len") {
            _config.authKeyLen = (uint8_t)val.toInt();
        } else if (key == "config_version") {
            _config.configVersion = (uint8_t)val.toInt();
        }
    }

    f.close();
    return true;
}

bool IoTStorage::saveConfig() {
    if (!_mounted) return false;

    File f = LittleFS.open(PATH_CONFIG, "w");
    if (!f) return false;

    f.printf("name=%s\n", _config.deviceName);
    f.printf("id=%d\n", _config.deviceId);
    f.printf("central_ip=%s\n", _config.centralIP.toString().c_str());
    f.printf("udp_port=%d\n", _config.udpPort);
    f.printf("heartbeat_ms=%lu\n", (unsigned long)_config.heartbeatIntervalMs);
    f.printf("antirebote_ms=%lu\n", (unsigned long)_config.antireboteMs);
    f.printf("auth_enabled=%d\n", _config.authEnabled ? 1 : 0);
    f.printf("auth_key_len=%d\n", _config.authKeyLen);
    f.printf("config_version=%d\n", _config.configVersion);

    f.close();
    return true;
}

void IoTStorage::resetConfig() {
    _setDefaults();
    if (_mounted) {
        LittleFS.remove(PATH_CONFIG);
        LittleFS.remove(PATH_AUTH_KEY);
    }
}

void IoTStorage::_setDefaults() {
    memset(&_config, 0, sizeof(_config));
    strncpy(_config.deviceName, "IoT Device", IOT_STORAGE_NAME_MAX);
    _config.deviceId = 0x02;
    _config.centralIP = IPAddress(192, 168, 0, 201);
    _config.udpPort = 4210;
    _config.heartbeatIntervalMs = 60000;
    _config.antireboteMs = 2000;
    _config.authEnabled = false;
    _config.authKeyLen = 0;
    _config.configVersion = 1;
}

// ============================================================
// Auth Key
// ============================================================

bool IoTStorage::loadAuthKey() {
    if (!_mounted || !LittleFS.exists(PATH_AUTH_KEY)) return false;

    File f = LittleFS.open(PATH_AUTH_KEY, "r");
    if (!f) return false;

    size_t bytesRead = f.read(_config.authKey, IOT_STORAGE_KEY_MAX);
    f.close();

    if (bytesRead > 0 && bytesRead <= IOT_STORAGE_KEY_MAX) {
        _config.authKeyLen = (uint8_t)bytesRead;
        return true;
    }
    return false;
}

bool IoTStorage::saveAuthKey(const uint8_t* key, uint8_t len) {
    if (!_mounted) return false;
    if (len > IOT_STORAGE_KEY_MAX) len = IOT_STORAGE_KEY_MAX;

    File f = LittleFS.open(PATH_AUTH_KEY, "w");
    if (!f) return false;

    f.write(key, len);
    f.close();

    memcpy(_config.authKey, key, len);
    _config.authKeyLen = len;
    return true;
}

// ============================================================
// Utilidades
// ============================================================

bool IoTStorage::format() {
    bool ok = LittleFS.format();
    if (ok) {
        LittleFS.begin();
        LittleFS.mkdir("/iot");
        _bootCount = 0;
        _setDefaults();
    }
    return ok;
}

uint32_t IoTStorage::freeSpace() const {
    if (!_mounted) return 0;
    FSInfo info;
    LittleFS.info(info);
    return info.totalBytes - info.usedBytes;
}
