// ============================================================================
// Reloj Digital NodeMCU ESP8266 + LCD I2C + NTP
// Ver .ai/ARCHITECTURE.md para el diagrama de estados completo.
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

WiFiManager* g_wifi = nullptr;
NtpClient* g_ntp = nullptr;
display::ColonState g_colon;
timekeeping::TimePacked g_currentTime;

void setup() {
    Serial.begin(115200);
    delay(1000);

    LOGLN(F("\n=== RELOJ NodeMCU ESP8266 NTP ==="));

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

void loop() {
    const uint32_t now = millis();

    g_wifi->update();

    if (g_wifi->isConnected()) {
        if (g_ntp->state() == NtpClient::State::IDLE) {
            g_ntp->begin();
        }
        g_ntp->update();
    }

    static uint32_t lastTimeUpdate = 0;
    if (now - lastTimeUpdate >= 200) {
        lastTimeUpdate = now;
        g_currentTime = timekeeping::TimePacked::fromSystem();
    }

    if (now - g_colon.lastToggle >= display::ColonState::INTERVAL_MS) {
        g_colon.lastToggle = now;
        g_colon.visible = !g_colon.visible;
    }

    ui::showTime(g_currentTime, g_colon.visible);
    ui::showStatus(
        g_wifi->isConnected(),
        g_ntp->isSynced(),
        g_ntp->isSynced()
    );
}
