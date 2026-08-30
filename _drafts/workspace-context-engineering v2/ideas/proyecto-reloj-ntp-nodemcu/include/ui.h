#pragma once
#include "display.h"
#include "timekeeping.h"

// ============================================================================
// UI: GESTIÓN DE ESTADO EN DISPLAY
// ============================================================================
namespace ui {

    inline void showStatus(bool wifi, bool synced, bool ntp) noexcept {
        display::lcd.setCursor(13, 1);
        display::lcd.print(wifi ? F("W") : F("-"));
        display::lcd.setCursor(14, 1);
        display::lcd.print(synced ? F("T") : F("-"));
        display::lcd.setCursor(15, 1);
        display::lcd.print(ntp ? F("*") : F("-"));
    }

    inline void showTime(const timekeeping::TimePacked& t, bool showColon) noexcept {
        display::printBigDigit(t.hour / 10, 0);
        display::printBigDigit(t.hour % 10, 3);
        display::printBigDigit(t.minute / 10, 7);
        display::printBigDigit(t.minute % 10, 10);

        display::lcd.setCursor(14, 0);
        display::lcd.print(t.is_am ? F("AM") : F("PM"));

        display::lcd.setCursor(6, 0);
        display::lcd.print(showColon ? F(".") : F(" "));
        display::lcd.setCursor(6, 1);
        display::lcd.print(showColon ? F(".") : F(" "));
    }

} // namespace ui
