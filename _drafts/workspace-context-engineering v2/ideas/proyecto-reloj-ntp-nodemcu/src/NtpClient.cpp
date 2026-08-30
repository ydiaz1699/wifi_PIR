#include "NtpClient.h"
#include <Arduino.h>
#include "display.h"
#include "secrets.h"
#include "log.h"

void NtpClient::begin() noexcept {
    if (m_state != State::IDLE) return;

    LOGLN(F("[NTP] Configurando sincronizacion..."));
    configTime(
        secrets::GMT_OFFSET_SEC,
        secrets::DAYLIGHT_OFFSET_SEC,
        secrets::NTP_SERVER.data()
    );

    m_state = State::SYNCING;
    m_lastAttempt = millis();
    m_retries = 0;

    display::lcd.clear();
    display::lcd.setCursor(0, 0);
    display::lcd.print(F("Sincronizando"));
    display::lcd.setCursor(0, 1);
    display::lcd.print(F("hora (NTP)..."));
}

void NtpClient::update() noexcept {
    switch (m_state) {
        case State::SYNCING: {
            if (millis() - m_lastAttempt < RETRY_INTERVAL_MS) {
                return;
            }

            struct tm timeinfo;
            if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
                m_state = State::SYNCED;
                m_retries = 0;
                m_firstSync = false;
                m_lastAttempt = millis();

                LOGLN(F("[NTP] Hora sincronizada"));
                LOGF("[NTP] Hora: %02d:%02d:%02d\n",
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

                display::lcd.clear();
                return;
            }

            m_retries++;
            if (m_retries >= MAX_RETRIES) {
                m_state = State::FAILED;
                LOGLN(F("[NTP] Fallo sincronizacion"));
                display::lcd.clear();
                return;
            }

            m_lastAttempt = millis();
            break;
        }

        case State::SYNCED: {
            if (millis() - m_lastAttempt >= RESYNC_INTERVAL_MS) {
                LOGLN(F("[NTP] Resincronizacion programada"));
                m_state = State::IDLE;
            }
            break;
        }

        case State::FAILED: {
            if (millis() - m_lastAttempt >= FAIL_RETRY_MS) {
                LOGLN(F("[NTP] Reintentando tras fallo..."));
                m_state = State::IDLE;
            }
            break;
        }

        case State::IDLE:
            break;
    }
}

void NtpClient::restart() noexcept {
    m_state = State::IDLE;
    m_retries = 0;
}
