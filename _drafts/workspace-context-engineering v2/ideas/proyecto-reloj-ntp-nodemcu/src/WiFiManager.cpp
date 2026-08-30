#include "WiFiManager.h"
#include "display.h"
#include "log.h"

WiFiManager::WiFiManager() {
    m_onConnected = WiFi.onStationModeGotIP(
        [this](const WiFiEventStationModeGotIP& event) {
            m_state = State::CONNECTED;
            char ip_buf[16];
            event.ip.toString().toCharArray(ip_buf, sizeof(ip_buf));
            LOGLN(F("[WiFi] Conectado"));
            LOGF("[WiFi] IP: %s\n", ip_buf);
        });

    m_onDisconnected = WiFi.onStationModeDisconnected(
        [this](const WiFiEventStationModeDisconnected& event) {
            (void)event;
            if (m_state == State::CONNECTED) {
                LOGLN(F("[WiFi] Desconectado"));
            }
            m_state = State::DISCONNECTED;
        });
}

void WiFiManager::begin(const char* ssid, const char* password) noexcept {
    LOGLN(F("[WiFi] Iniciando conexion..."));

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(ssid, password);

    m_state = State::CONNECTING;
    m_connectStart = millis();
    m_attempts = 0;

    display::lcd.clear();
    display::lcd.setCursor(0, 0);
    display::lcd.print(F("Conectando WiFi"));
}

void WiFiManager::update() noexcept {
    switch (m_state) {
        case State::CONNECTING: {
            if (WiFi.status() == WL_CONNECTED) {
                m_state = State::CONNECTED;
                display::lcd.clear();
                display::lcd.setCursor(0, 0);
                display::lcd.print(F("WiFi conectado"));
                delay(1000);  // Excepcion deliberada - ver .ai/SKILL.md
                return;
            }

            if (millis() - m_connectStart >= 400) {
                m_connectStart = millis();
                m_attempts++;

                static uint8_t dotPos = 0;
                display::lcd.setCursor(dotPos % 16, 1);
                display::lcd.print(F("."));
                dotPos++;

                if (dotPos >= 16) {
                    display::lcd.setCursor(0, 1);
                    display::lcd.print(F("                "));
                    dotPos = 0;
                }

                if (m_attempts >= MAX_ATTEMPTS) {
                    m_state = State::FAILED;
                    display::lcd.clear();
                    display::lcd.setCursor(0, 0);
                    display::lcd.print(F("Sin WiFi..."));
                    LOGLN(F("[WiFi] Fallo conexion"));
                    delay(1000);  // Excepcion deliberada - ver .ai/SKILL.md
                }
            }
            break;
        }

        case State::CONNECTED:
            break;

        case State::FAILED:
            if (millis() - m_connectStart >= RECONNECT_INTERVAL_MS) {
                m_state = State::DISCONNECTED;
            }
            break;

        case State::DISCONNECTED:
            break;
    }
}
