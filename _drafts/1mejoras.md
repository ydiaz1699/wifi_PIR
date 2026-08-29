Los archivos que edité están en mi entorno sandbox (el clon temporal del repo que hice para analizar/editar), **no en tu computadora**. No tengo acceso a tu copia local real — solo puedo mostrarte el contenido para que lo apliques tú mismo.

Aquí está el contenido completo y actualizado de los 5 archivos modificados, listos para copiar/pegar:## 1. `receptor_bocina/include/hal.h` (archivo completo)

```cpp
#pragma once
#include <Arduino.h>

class Led {
    int _pin;
    bool _state;
public:
    explicit Led(int pin);
    void begin();
    void on();
    void off();
    void toggle();
    bool isOn() const;
};

class Buzzer {
    int _pin;
    bool _state;
    unsigned long _offTime;
    Led* _led;

    // Patrón intermitente (sirena) — no bloqueante
    bool _patronActivo;
    unsigned long _patronOnMs;
    unsigned long _patronOffMs;
    unsigned long _patronFinTime;       // millis() en que termina TODO el patrón
    unsigned long _patronProximoToggle; // millis() del próximo cambio ON/OFF

public:
    explicit Buzzer(int pin);
    void begin();
    void on();
    void off();
    void timedOn(unsigned long ms);
    void sirenOn(unsigned long onMs, unsigned long offMs, unsigned long totalMs);
    void setLed(Led* led);
    void loop();
    bool isOn() const;
    bool isBusy() const;  // true si suena algo (beep simple O patrón intermitente)
};
```## 2. `receptor_bocina/src/hal.cpp` (archivo completo)

```cpp
#include "hal.h"

Led::Led(int pin) : _pin(pin), _state(false) {}
void Led::begin() { pinMode(_pin, OUTPUT); off(); }
void Led::on() { digitalWrite(_pin, HIGH); _state = true; }
void Led::off() { digitalWrite(_pin, LOW); _state = false; }
void Led::toggle() { _state ? off() : on(); }
bool Led::isOn() const { return _state; }

Buzzer::Buzzer(int pin) : _pin(pin), _state(false), _offTime(0), _led(nullptr),
    _patronActivo(false), _patronOnMs(0), _patronOffMs(0), _patronFinTime(0), _patronProximoToggle(0) {}
void Buzzer::begin() { pinMode(_pin, OUTPUT); off(); }
void Buzzer::setLed(Led* led) { _led = led; }
void Buzzer::on() { _patronActivo = false; digitalWrite(_pin, HIGH); _state = true; if (_led) _led->on(); }
void Buzzer::off() { _patronActivo = false; digitalWrite(_pin, LOW); _state = false; _offTime = 0; if (_led) _led->off(); }
void Buzzer::timedOn(unsigned long ms) { on(); _offTime = millis() + ms; }

// Patrón intermitente no bloqueante: alterna ON/OFF cada onMs/offMs
// durante totalMs, y luego se apaga solo. Usado para la sirena de MOTION,
// distinguible del beep corto y fijo del timbre (timedOn).
void Buzzer::sirenOn(unsigned long onMs, unsigned long offMs, unsigned long totalMs) {
    unsigned long ahora = millis();
    _patronActivo = true;
    _patronOnMs = onMs;
    _patronOffMs = offMs;
    _patronFinTime = ahora + totalMs;
    _offTime = 0;  // el patrón maneja su propio apagado, no usa timedOn

    digitalWrite(_pin, HIGH);
    _state = true;
    if (_led) _led->on();
    _patronProximoToggle = ahora + onMs;
}

void Buzzer::loop() {
    unsigned long ahora = millis();

    if (_patronActivo) {
        if (ahora >= _patronFinTime) {
            // Fin del patrón completo: apagar y limpiar estado
            digitalWrite(_pin, LOW);
            _state = false;
            _patronActivo = false;
            if (_led) _led->off();
            return;
        }
        if (ahora >= _patronProximoToggle) {
            if (_state) {
                digitalWrite(_pin, LOW);
                _state = false;
                if (_led) _led->off();
                _patronProximoToggle = ahora + _patronOffMs;
            } else {
                digitalWrite(_pin, HIGH);
                _state = true;
                if (_led) _led->on();
                _patronProximoToggle = ahora + _patronOnMs;
            }
        }
        return;
    }

    // Beep simple (timedOn), sin patrón activo
    if (_state && _offTime && ahora >= _offTime) off();
}

bool Buzzer::isOn() const { return _state; }

// Distinto de isOn(): durante un patrón intermitente, _state alterna entre
// true/false en cada toggle, pero el buzzer sigue "ocupado" sonando la sirena
// completa. isBusy() cubre ambos casos para código que necesita saber
// "¿está sonando algo ahora mismo, aunque sea a intervalos?".
bool Buzzer::isBusy() const { return _state || _patronActivo; }
```

## 3. `receptor_bocina/include/config.h` — solo cambia este bloque

Busca:
```cpp
extern const unsigned long DURACION_BOCINA_MS;
extern const unsigned long DURACION_TIMBRE_MS;
```
Reemplázalo por:
```cpp
extern const unsigned long DURACION_BOCINA_MS;
extern const unsigned long SIRENA_ON_MS;
extern const unsigned long SIRENA_OFF_MS;
extern const unsigned long DURACION_TIMBRE_MS;
```

## 4. `receptor_bocina/src/config.cpp` — solo cambia este bloque

Busca:
```cpp
const unsigned long DURACION_BOCINA_MS = 1000;           // Alarma motion: 1 segundo
const unsigned long DURACION_TIMBRE_MS = 500;            // Timbre: 500ms
```
Reemplázalo por:
```cpp
const unsigned long DURACION_BOCINA_MS = 4000;           // Alarma motion: sirena intermitente, 4s totales
const unsigned long SIRENA_ON_MS = 200;                  // Sirena: tiempo encendido por ciclo
const unsigned long SIRENA_OFF_MS = 200;                 // Sirena: tiempo apagado por ciclo
const unsigned long DURACION_TIMBRE_MS = 500;            // Timbre: beep corto fijo, sin cambios
```

## 5. `receptor_bocina/src/alarma.cpp` — solo cambia este bloque

Busca:
```cpp
static void activarAlarmaMotion() {
    LOG_INFO("MOTION: activando alarma");
    if (haDisponible) {
        mqtt.publish(TOPIC_EVENTO, "detectado");
    }
    if (modoActual == "armado") {
        buzzer.timedOn(DURACION_BOCINA_MS);
    }
}

static void activarTimbre() {
    LOG_INFO("TIMBRE: sonido corto de aviso");
    if (haDisponible) {
        mqtt.publish(TOPIC_TIMBRE, "presionado");
    }
    // Timbre SIEMPRE suena (independiente del modo armado/desarmado)
    buzzer.timedOn(DURACION_TIMBRE_MS);
}
```
Reemplázalo por:
```cpp
static void activarAlarmaMotion() {
    LOG_INFO("MOTION: activando alarma");
    if (haDisponible) {
        mqtt.publish(TOPIC_EVENTO, "detectado");
    }
    if (modoActual == "armado") {
        // Sirena intermitente (distinta al beep corto del timbre)
        buzzer.sirenOn(SIRENA_ON_MS, SIRENA_OFF_MS, DURACION_BOCINA_MS);
    }
}

static void activarTimbre() {
    LOG_INFO("TIMBRE: sonido corto de aviso");
    if (haDisponible) {
        mqtt.publish(TOPIC_TIMBRE, "presionado");
    }
    // Timbre SIEMPRE suena (independiente del modo armado/desarmado)
    // Beep corto y fijo, sin cambios — se distingue claramente de la sirena
    buzzer.timedOn(DURACION_TIMBRE_MS);
}
```

## 6. `receptor_bocina/src/mqtt_cliente.cpp` — 2 cambios puntuales

Busca (línea ~130):
```cpp
            if (!buzzer.isOn()) {
```
Reemplaza por:
```cpp
            if (!buzzer.isBusy()) {
```

Busca (línea ~158):
```cpp
            // NO intentar reconectar si la bocina está sonando
            if (buzzer.isOn()) return;
```
Reemplaza por:
```cpp
            // NO intentar reconectar si la bocina está sonando (beep o sirena)
            if (buzzer.isBusy()) return;
```

---

Después de aplicar los 6 cambios, compila y sube:
```bash
cd receptor_bocina
pio run -t upload
```

¿Quieres que actualice también el resumen de sesión (`RESUMEN_SESION_MQTT_2026-08-17.md`) agregando este cambio, por si continúas mañana?