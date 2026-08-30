//docs/notas.md

# PIR — Arduino Uno

## Hardware
- Sensor: HC-SR501
- Pin OUT → D2 (INT0)
- LED integrado → D13

## Conexión
| HC-SR501 | Arduino Uno |
|----------|-------------|
| VCC      | 5V          |
| GND      | GND         |
| OUT      | D2 (INT0)   |

## Notas
- Calibración inicial: 30–60 seg tras encendido (falsos positivos normales)
- Jumper H = retrigger / L = single shot
- Si se agrega RF 433MHz: mover PIR a D3 (INT1)