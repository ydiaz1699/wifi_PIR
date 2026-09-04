#include "IoTStorage.h"
#include "LittleFS.h"

#include <cstdio>
#include <cstring>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        return false;
    }
    return true;
}

bool check_atomic_config_and_checksum() {
    HostLittleFS::reset();
    IoTStorage storage;
    if (!expect(storage.begin(), "LittleFS host stub no montó")) return false;

    std::strcpy(storage.config().deviceName, "12345678901234567890123");
    if (!expect(storage.saveConfig(), "saveConfig no escribió el temporal")) return false;
    if (!expect(storage.loadConfig(), "config válida con checksum fue rechazada")) return false;
    if (!expect(std::strcmp(storage.config().deviceName, "12345678901234567890123") == 0,
                "nombre de 23 caracteres no sobrevivió storage")) return false;

    const std::size_t fullSize = HostLittleFS::size("/iot/config.json");
    HostLittleFS::truncate("/iot/config.json", fullSize / 2);
    std::strcpy(storage.config().deviceName, "MUTATED");
    if (!expect(!storage.loadConfig(), "config truncada fue aceptada")) return false;
    if (!expect(std::strcmp(storage.config().deviceName, "IoT Device") == 0,
                "config truncada dejó valores parciales aplicados")) return false;
    if (!expect(HostLittleFS::size("/iot/config.json") == fullSize / 2,
                "loadConfig modificó el archivo final truncado")) return false;

    if (!expect(storage.saveConfig(), "saveConfig no pudo recuperar desde temporal")) return false;
    return expect(storage.loadConfig(), "config recuperada no volvió a ser válida");
}

bool check_boot_id_degraded_state() {
    HostLittleFS::reset();
    IoTStorage storage;
    if (!expect(storage.begin(), "LittleFS host stub no montó para BOOT_ID")) return false;
    const uint16_t first = storage.getBootId();
    if (!expect(first != 0 && storage.isBootIdPersistent(),
                "BOOT_ID normal no quedó marcado como persistente")) return false;

    HostLittleFS::setFailWrites(true);
    const uint16_t degraded = storage.getBootId();
    return expect(degraded != 0 && !storage.isBootIdPersistent(),
                  "fallo de escritura no activó BOOT_ID degradado");
}

bool check_mount_failure_is_non_destructive() {
    HostLittleFS::reset();
    IoTStorage healthy;
    if (!expect(healthy.begin(), "LittleFS host stub no montó para mount failure")) return false;
    std::strcpy(healthy.config().deviceName, "preserved");
    if (!expect(healthy.saveConfig(), "no se pudo preparar config para mount failure")) return false;
    const std::size_t before = HostLittleFS::size("/iot/config.json");

    HostLittleFS::setFailBegins(2);
    IoTStorage degraded;
    if (!expect(!degraded.begin(), "fallo de montaje simulado fue ocultado")) return false;
    if (!expect(!degraded.isMounted(), "storage degradado quedó marcado como montado")) return false;
    if (!expect(HostLittleFS::size("/iot/config.json") == before,
                "fallo de montaje alteró la configuración existente")) return false;
    const uint16_t fallback = degraded.getBootId();
    if (!expect(fallback != 0 && !degraded.isBootIdPersistent(),
                "fallo de montaje no produjo BOOT_ID degradado")) return false;

    HostLittleFS::setFailBegins(0);
    if (!expect(degraded.retryMount(), "retryMount no recuperó LittleFS")) return false;
    if (!expect(degraded.isMounted(), "retryMount no marcó LittleFS como montado")) return false;
    if (!expect(!degraded.isBootIdPersistent(),
                "retryMount reactivó persistencia después de consumir fallback")) return false;
    return expect(degraded.loadConfig() &&
                      std::strcmp(degraded.config().deviceName, "preserved") == 0,
                  "configuración preservada no pudo recargarse tras retryMount");
}


}  // namespace

int main() {
    if (!check_atomic_config_and_checksum()) return 1;
    if (!check_boot_id_degraded_state()) return 2;
    if (!check_mount_failure_is_non_destructive()) return 3;
    std::puts("OK: storage hardening check wifi_PIR");
    return 0;
}
