#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <array>
#include <pgmspace.h>
#include "hw.h"

// ============================================================================
// DISPLAY: DÍGITOS GRANDES CON PROGMEM
// CRÍTICO: slots CGRAM van de 1 a 8. NUNCA usar el slot 0 (basura visual).
// Ver .ai/SKILL.md regla #1.
// ============================================================================
namespace display {

    namespace segments {
        inline constexpr std::array<std::array<uint8_t, 8>, 8> PATTERNS PROGMEM = {{
            {{0b11100, 0b11110, 0b11110, 0b11110, 0b11110, 0b11110, 0b11110, 0b11100}},
            {{0b00111, 0b01111, 0b01111, 0b01111, 0b01111, 0b01111, 0b01111, 0b00111}},
            {{0b11111, 0b11111, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111}},
            {{0b11110, 0b11100, 0b00000, 0b00000, 0b00000, 0b00000, 0b11000, 0b11100}},
            {{0b01111, 0b00111, 0b00000, 0b00000, 0b00000, 0b00000, 0b00011, 0b00111}},
            {{0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b11111}},
            {{0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00111, 0b01111}},
            {{0b11111, 0b11111, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}},
        }};
    }

    namespace chars {
        inline constexpr uint8_t BLANK = 32;
        inline constexpr uint8_t C1 = 1;
        inline constexpr uint8_t C2 = 2;
        inline constexpr uint8_t C3 = 3;
        inline constexpr uint8_t C4 = 4;
        inline constexpr uint8_t C5 = 5;
        inline constexpr uint8_t C6 = 6;
        inline constexpr uint8_t C7 = 7;
        inline constexpr uint8_t C8 = 8;
    }

    using DigitPattern = std::array<uint8_t, 6>;

    inline constexpr std::array<DigitPattern, 10> DIGITS = {{
        {{chars::C2, chars::C8, chars::C1, chars::C2, chars::C6, chars::C1}},
        {{chars::BLANK, chars::BLANK, chars::C1, chars::BLANK, chars::BLANK, chars::C1}},
        {{chars::C5, chars::C3, chars::C1, chars::C2, chars::C6, chars::C6}},
        {{chars::C5, chars::C3, chars::C1, chars::C7, chars::C6, chars::C1}},
        {{chars::C2, chars::C6, chars::C1, chars::BLANK, chars::BLANK, chars::C1}},
        {{chars::C2, chars::C3, chars::C4, chars::C7, chars::C6, chars::C1}},
        {{chars::C2, chars::C3, chars::C4, chars::C2, chars::C6, chars::C1}},
        {{chars::C2, chars::C8, chars::C1, chars::BLANK, chars::BLANK, chars::C1}},
        {{chars::C2, chars::C3, chars::C1, chars::C2, chars::C6, chars::C1}},
        {{chars::C2, chars::C3, chars::C1, chars::C7, chars::C6, chars::C1}},
    }};

    inline LiquidCrystal_I2C lcd(hw::LCD_ADDR, hw::LCD_COLS, hw::LCD_ROWS);

    inline void initCustomChars() noexcept {
        std::array<uint8_t, 8> buffer;
        for (uint8_t i = 0; i < 8; ++i) {
            memcpy_P(buffer.data(), &segments::PATTERNS[i], 8);
            lcd.createChar(i + 1, buffer.data());
        }
    }

    template<uint8_t N>
    inline void drawDigit(int col) noexcept {
        static_assert(N < 10, "Digito debe estar entre 0-9");
        constexpr auto& pat = DIGITS[N];

        lcd.setCursor(col, 0);
        lcd.write(pat[0]);
        lcd.write(pat[1]);
        lcd.write(pat[2]);

        lcd.setCursor(col, 1);
        lcd.write(pat[3]);
        lcd.write(pat[4]);
        lcd.write(pat[5]);
    }

    inline void printBigDigit(uint8_t digit, int col) noexcept {
        static constexpr std::array<void(*)(int), 10> RENDERERS = {
            drawDigit<0>, drawDigit<1>, drawDigit<2>, drawDigit<3>, drawDigit<4>,
            drawDigit<5>, drawDigit<6>, drawDigit<7>, drawDigit<8>, drawDigit<9>
        };
        if (digit < 10) {
            RENDERERS[digit](col);
        }
    }

    struct ColonState {
        bool visible = true;
        uint32_t lastToggle = 0;
        static constexpr uint32_t INTERVAL_MS = 500;
    };

} // namespace display
