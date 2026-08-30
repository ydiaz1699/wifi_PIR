// ============================================================================
// Reloj Digital NodeMCU ESP8266 + LCD I2C + NTP
// Código dividido en varios archivos (.h/.cpp) dentro de include/ y src/,
// equivalente a usar pestañas en el IDE de Arduino.
// ============================================================================

#include <Arduino.h>
#include <Wire.h>

#include "secrets.h"
#include "log.h"
#include "hw.h"
#include "display.h"
#include "timekeeping.h"
#include "WiFiManager.h"
#include "NtpClient.h"
#include "ui.h"

// ============================================================================
// VARIABLES GLOBALES (minimizadas)
// ============================================================================
WiFiManager* g_wifi = nullptr;
NtpClient* g_ntp = nullptr;
display::ColonState g_colon;
timekeeping::TimePacked g_currentTime;

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    LOGLN(F("\n=== RELOJ NodeMCU ESP8266 NTP ==="));
    LOGLN(F("Codigo organizado en multiples archivos"));

    Wire.begin(hw::PIN_SDA, hw::PIN_SCL);

    display::lcd.init();
    display::lcd.backlight();
    display::lcd.clear();
    display::lcd.setCursor(0, 0);
    display::lcd.print(F("Iniciando reloj"));

    display::initCustomChars();

    static WiFiManager wifiInstance;
    static NtpClient ntpInstance;
    g_wifi = &wifiInstance;
    g_ntp = &ntpInstance;

    g_wifi->begin(secrets::WIFI_SSID.data(), secrets::WIFI_PASSWORD.data());

    LOGLN(F("=== SETUP COMPLETADO ===\n"));
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================
void loop() {
    const uint32_t now = millis();

    // --- Actualizar WiFi (FSM no bloqueante) ---
    g_wifi->update();

    // --- Actualizar NTP cuando WiFi este conectado ---
    if (g_wifi->isConnected()) {
        if (g_ntp->state() == NtpClient::State::IDLE) {
            g_ntp->begin();
        }
        g_ntp->update();
    }

    // --- Actualizar hora desde sistema cada 200ms ---
    static uint32_t lastTimeUpdate = 0;
    if (now - lastTimeUpdate >= 200) {
        lastTimeUpdate = now;
        g_currentTime = timekeeping::TimePacked::fromSystem();
    }

    // --- Parpadeo de dos puntos cada 500ms ---
    if (now - g_colon.lastToggle >= display::ColonState::INTERVAL_MS) {
        g_colon.lastToggle = now;
        g_colon.visible = !g_colon.visible;
    }

    // --- Renderizar display ---
    ui::showTime(g_currentTime, g_colon.visible);
    ui::showStatus(
        g_wifi->isConnected(),
        g_ntp->isSynced(),
        g_ntp->isSynced()
    );
}
