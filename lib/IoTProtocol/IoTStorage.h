/**
 * IoTStorage V4.3 — Persistencia LittleFS para IoTProtocol
 *
 * Proporciona:
 * - Boot counter persistente (reemplaza random BOOT_ID → determinístico e incremental)
 * - Almacenamiento de configuración del dispositivo (JSON)
 * - Clave de autenticación persistente
 *
 * Archivos en LittleFS:
 *   /iot/boot_count    — uint32_t en texto (1 línea)
 *   /iot/config.json   — configuración del dispositivo
 *   /iot/auth.key      — clave HMAC (binario, 16-32 bytes)
 *
 * Uso:
 *   IoTStorage storage;
 *   storage.begin();
 *   uint16_t bootId = storage.getBootId();  // Incrementa y persiste
 *
 *   // Leer/escribir config
 *   storage.setDeviceName("PIR Entrada");
 *   storage.setCentralIP(IPAddress(192,168,0,201));
 *   storage.save();
 *
 * Nota: LittleFS debe formatearse una vez en primera instalación.
 * begin() hace format automático si no puede montar.
 */

#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>

// Tamaños máximos
#define IOT_STORAGE_NAME_MAX     24
#define IOT_STORAGE_KEY_MAX      32

// Configuración persistente
struct IoTConfig {
    // Identidad
    char     deviceName[IOT_STORAGE_NAME_MAX];
    uint8_t  deviceId;

    // Red
    IPAddress centralIP;
    uint16_t  udpPort;

    // Timing
    uint32_t heartbeatIntervalMs;
    uint32_t antireboteMs;

    // Auth
    uint8_t  authKey[IOT_STORAGE_KEY_MAX];
    uint8_t  authKeyLen;
    bool     authEnabled;

    // Versión de config (para detectar cambios)
    uint8_t  configVersion;
};

class IoTStorage {
public:
    IoTStorage();

    /**
     * Inicializar LittleFS. Si no puede montar, formatea.
     * @return true si montó correctamente
     */
    bool begin();

    /**
     * Obtiene el BOOT_ID actual (incrementa el counter y persiste).
     * Llamar una vez al boot. Es uint16_t (wraps en 65535 → 1).
     */
    uint16_t getBootId();

    /**
     * Lee el boot counter sin incrementar.
     */
    uint32_t getBootCount() const { return _bootCount; }

    // --- Configuración ---

    /**
     * Carga la configuración desde /iot/config.json.
     * Si no existe, usa valores por defecto.
     */
    bool loadConfig();

    /**
     * Guarda la configuración actual a /iot/config.json.
     */
    bool saveConfig();

    /**
     * Resetear configuración a valores por defecto.
     */
    void resetConfig();

    /**
     * Acceso a la config actual (lectura/escritura directa).
     */
    IoTConfig& config() { return _config; }
    const IoTConfig& config() const { return _config; }

    // --- Auth key ---

    /**
     * Carga la clave de /iot/auth.key.
     * @return true si hay clave guardada
     */
    bool loadAuthKey();

    /**
     * Guarda la clave a /iot/auth.key.
     */
    bool saveAuthKey(const uint8_t* key, uint8_t len);

    // --- Utilidades ---

    /**
     * Formatea LittleFS (borra todo).
     */
    bool format();

    /**
     * Espacio libre en LittleFS (bytes).
     */
    uint32_t freeSpace() const;

private:
    uint32_t _bootCount;
    IoTConfig _config;
    bool _mounted;

    bool _readBootCount();
    bool _writeBootCount();
    void _setDefaults();
};
