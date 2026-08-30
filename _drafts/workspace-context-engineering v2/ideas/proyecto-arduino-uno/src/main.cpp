#include <Arduino.h>
#include <RCSwitch.h>

// NOTE: Receptor RF 433MHz en D2 (INT0) — ver .ai/HARDWARE.md

RCSwitch mySwitch = RCSwitch();

void setup() {
    Serial.begin(115200);
    mySwitch.enableReceive(digitalPinToInterrupt(2));
}

void loop() {
    if (mySwitch.available()) {
        Serial.print(F("Codigo recibido: "));
        Serial.println(mySwitch.getReceivedValue());
        mySwitch.resetAvailable();
    }
}
