# 🔌 Periférico: HC-SR04

| Atributo | Valor |
|----------|-------|
| **Categoría** | Sensor de distancia |
| **Voltaje señal** | 5V (Trig input, Echo output) |
| **Voltaje alimentación** | 5V |
| **Pines** | Trig (input), Echo (output 5V) |
| **Rango** | 2cm – 400cm |
| **Resolución** | ~3mm |
| **Nota** | Echo es 5V — usar divisor resistivo o level shifter con ESP32/ESP8266 (3.3V). El pin Echo puede dañar GPIO 3.3V si se conecta directamente. |
