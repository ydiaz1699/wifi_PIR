#pragma once
#include <cstdint>

// ============================================================================
// NTP: CLIENTE NO BLOQUEANTE CON FSM Y RESYNC PERIODICO
// ============================================================================
class NtpClient {
public:
    enum class State : uint8_t {
        IDLE,
        SYNCING,
        SYNCED,
        FAILED
    };

    void begin() noexcept;
    void update() noexcept;
    void restart() noexcept;

    [[nodiscard]] State state() const noexcept { return m_state; }
    [[nodiscard]] bool isSynced() const noexcept { return m_state == State::SYNCED; }
    [[nodiscard]] bool isFirstSync() const noexcept { return m_firstSync; }

private:
    State m_state = State::IDLE;
    uint32_t m_lastAttempt = 0;
    uint8_t m_retries = 0;
    bool m_firstSync = true;

    static constexpr uint8_t MAX_RETRIES = 20;
    static constexpr uint32_t RETRY_INTERVAL_MS = 500;
    static constexpr uint32_t RESYNC_INTERVAL_MS = 3600000UL;  // 1 hora
    static constexpr uint32_t FAIL_RETRY_MS = 300000UL;          // 5 min tras fallo
};
