#pragma once
#include <Arduino.h>
#include <cstdint>

// ============================================================================
// TIEMPO: ESTRUCTURA PACKED (ahorro de RAM)
// NOTA: constructor posicional a propósito — C++17, NO designated init.
// Ver .ai/SKILL.md regla #3.
// ============================================================================
namespace timekeeping {

    struct TimePacked {
        uint8_t hour : 4;    // 1-12  -> 4 bits
        uint8_t minute : 6;  // 0-59  -> 6 bits
        uint8_t second : 6;  // 0-59  -> 6 bits
        uint8_t is_am : 1;   // 0/1   -> 1 bit

        constexpr TimePacked() noexcept : hour(12), minute(0), second(0), is_am(1) {}

        constexpr TimePacked(uint8_t h, uint8_t m, uint8_t s, uint8_t a) noexcept
            : hour(h), minute(m), second(s), is_am(a) {}

        [[nodiscard]] static TimePacked fromSystem() noexcept {
            struct tm ti;
            if (!getLocalTime(&ti)) {
                return TimePacked{};
            }
            const uint8_t h24 = static_cast<uint8_t>(ti.tm_hour);
            uint8_t h12 = h24 % 12;
            if (h12 == 0) h12 = 12;

            return TimePacked{
                h12,
                static_cast<uint8_t>(ti.tm_min),
                static_cast<uint8_t>(ti.tm_sec),
                static_cast<uint8_t>(h24 < 12)
            };
        }
    };
    static_assert(sizeof(TimePacked) <= 3, "TimePacked debe ocupar maximo 3 bytes");

} // namespace timekeeping
