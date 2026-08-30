#pragma once
#include <ESP8266WiFi.h>
#include <cstdint>

// ============================================================================
// WIFI: GESTIÓN CON RAII, AUTO-RECONNECT Y FSM
// Ver .ai/ARCHITECTURE.md para el diagrama de estados completo.
// ============================================================================
class WiFiManager {
public:
    enum class State : uint8_t {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        FAILED
    };

    WiFiManager(const WiFiManager&) = delete;
    WiFiManager& operator=(const WiFiManager&) = delete;
    WiFiManager(WiFiManager&&) = delete;
    WiFiManager& operator=(WiFiManager&&) = delete;

    WiFiManager();

    void begin(const char* ssid, const char* password) noexcept;
    void update() noexcept;

    [[nodiscard]] State state() const noexcept { return m_state; }
    [[nodiscard]] bool isConnected() const noexcept { return m_state == State::CONNECTED; }
    [[nodiscard]] bool isFailed() const noexcept { return m_state == State::FAILED; }

private:
    WiFiEventHandler m_onConnected;
    WiFiEventHandler m_onDisconnected;
    volatile State m_state = State::DISCONNECTED;
    uint32_t m_connectStart = 0;
    uint8_t m_attempts = 0;

    static constexpr uint8_t MAX_ATTEMPTS = 30;
    static constexpr uint32_t TIMEOUT_MS = 15000;
    static constexpr uint32_t RECONNECT_INTERVAL_MS = 30000;  // 30 seg reintento
};
