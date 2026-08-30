#include <Arduino.h>
#include <NimBLEDevice.h>

// ─────────────────────────────────────────────────────────────────────────────
// CONFIGURACIÓN DEL DISPOSITIVO AUTORIZADO
// ─────────────────────────────────────────────────────────────────────────────
static const char TARGET_MAC[]  = "09:1c:2d:e3:42:47";
static const char TARGET_NAME[] = "Watch10";
static const NimBLEUUID TARGET_UUID("e7fe");

// ─────────────────────────────────────────────────────────────────────────────
// PARÁMETROS PKE
// ─────────────────────────────────────────────────────────────────────────────
const int    RSSI_UMBRAL       = -78;    // dBm mínimo para autorizar
const unsigned long TIMEOUT_MS = 5000;   // ms sin detección → revocar
const int    PIN_RELAY         = 16;     // Salida: HIGH = desbloqueado
const int    PIN_LED           = 2;      // LED onboard = estado

// ─────────────────────────────────────────────────────────────────────────────
// ESTADO
// ─────────────────────────────────────────────────────────────────────────────
static volatile bool     autorizado      = false;
static volatile unsigned long tUltDetect = 0;
static volatile int      ultimoRSSI      = -100;

// ─────────────────────────────────────────────────────────────────────────────
// CALLBACK DE ESCANEO
// ─────────────────────────────────────────────────────────────────────────────
class PKECallback : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* dev) override {
        if (dev == nullptr) return;

        // Filtrar por MAC
        if (dev->getAddress().toString() != TARGET_MAC) return;

        // Filtrar por nombre si está disponible
        if (!dev->getName().empty() && dev->getName() != TARGET_NAME) return;

        // Filtrar por UUID de servicio anunciado si existe
        if (!dev->isAdvertisingService(TARGET_UUID)) return;

        int rssi = dev->getRSSI();
        ultimoRSSI = rssi;

        if (rssi >= RSSI_UMBRAL) {
            autorizado = true;
            tUltDetect = millis();
            Serial.printf("[PKE] AUTH OK | %s | RSSI: %d dBm\n", TARGET_NAME, rssi);
        } else {
            Serial.printf("[PKE] Detectado pero lejos | RSSI: %d dBm (min: %d)\n", rssi, RSSI_UMBRAL);
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);
    digitalWrite(PIN_LED, LOW);

    Serial.println("\n=== PKE - Passive Keyless Entry ===");
    Serial.printf("Dispositivo: %s [%s]\n", TARGET_NAME, TARGET_MAC);
    Serial.printf("UUID objetivo: %s | RSSI umbral: %d dBm | Timeout: %lu ms\n", TARGET_UUID.toString().c_str(), RSSI_UMBRAL, TIMEOUT_MS);

    NimBLEDevice::init("PKE_ESP32");
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(new PKECallback(), true);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);
    scan->start(0, false);  // Escaneo continuo
}

// ─────────────────────────────────────────────────────────────────────────────
// LOOP
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    // Revocar si timeout
    if (autorizado && (millis() - tUltDetect > TIMEOUT_MS)) {
        autorizado = false;
        Serial.println("[PKE] REVOCADO - dispositivo fuera de rango");
    }

    // Actualizar salidas
    digitalWrite(PIN_RELAY, autorizado ? HIGH : LOW);
    digitalWrite(PIN_LED, autorizado ? HIGH : LOW);

    delay(100);
}
