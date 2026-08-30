#pragma once
#include <cstdint>

// ============================================================================
// CONSTANTES HARDWARE (NodeMCU ESP8266 / ESP-12E)
// Ver ../../boards/esp8266-nodemcu-v2.md para specs genéricas de la placa.
// ============================================================================
namespace hw {
    inline constexpr uint8_t PIN_SDA = 4;   // GPIO4 = D2
    inline constexpr uint8_t PIN_SCL = 5;   // GPIO5 = D1
    inline constexpr uint8_t LCD_ADDR = 0x27;
    inline constexpr uint8_t LCD_COLS = 16;
    inline constexpr uint8_t LCD_ROWS = 2;
}
