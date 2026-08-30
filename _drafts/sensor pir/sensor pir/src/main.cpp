// src/main.cpp
#include <Arduino.h>

#define PIN_PIR  2
#define PIN_LED  13

const unsigned long TIEMPO_PRESENCIA_MS = 5000;
const unsigned long DEBOUNCE_MS         = 200;

volatile bool movimientoDetectado = false;
unsigned long ultimaDeteccion     = 0;
bool          estadoLED           = false;

void isrPIR() {
    if (digitalRead(PIN_PIR) == HIGH) {
        movimientoDetectado = true;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("=== PIR Arduino Uno ==="));
    pinMode(PIN_PIR, INPUT);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
    attachInterrupt(digitalPinToInterrupt(PIN_PIR), isrPIR, CHANGE);
}

void loop() {
    unsigned long ahora = millis();

    // Debug: ver estado crudo del pin
    Serial.println(digitalRead(PIN_PIR));
    delay(200);

    if (movimientoDetectado) {
        noInterrupts();
        movimientoDetectado = false;
        interrupts();

        if ((ahora - ultimaDeteccion) >= DEBOUNCE_MS) {
            ultimaDeteccion = ahora;
            if (!estadoLED) Serial.println(F(">> Movimiento detectado"));
            estadoLED = true;
            digitalWrite(PIN_LED, HIGH);
        }
    }

    if (estadoLED && (ahora - ultimaDeteccion) >= TIEMPO_PRESENCIA_MS) {
        estadoLED = false;
        digitalWrite(PIN_LED, LOW);
        Serial.println(F(">> Sin movimiento"));
    }
}