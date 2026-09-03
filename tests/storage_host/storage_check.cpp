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

}  // namespace

int main() {
    if (!check_atomic_config_and_checksum()) return 1;
    if (!check_boot_id_degraded_state()) return 2;
    std::puts("OK: storage hardening check wifi_PIR");
    return 0;
}
