# 🖥️ Placa: Arduino Nano (ATmega328P)

## 🧩 Hardware Principal

- **Placa:** Arduino Nano (ATmega328P)
- **MCU:** ATmega328P
- **Clock:** 16 MHz
- **RAM:** 2 KB
- **Flash:** 32 KB
- **Alimentación:** 5V USB (Mini-B) / 7–12V VIN
- **Dimensiones:** 45 mm × 18 mm (más compacta que Uno)

## ⚠️ Consideraciones

- Mismo MCU que Uno — misma programación, mismo mapeo de pines lógico
- **USB Mini-B** (obsoleto, considerar Nano V3.3 con USB-C)
- **Sin barrel jack** — alimentar por VIN o USB
- Ideal para proyectos compactos
- Ver `arduino-uno.md` para mapeo de pines completo, niveles de voltaje y
  consideraciones críticas de RAM (son idénticas en el ATmega328P)

## 📡 `platformio.ini`

```ini
[env:nanoatmega328]
platform = atmelavr
board = nanoatmega328
framework = arduino
monitor_speed = 115200
monitor_port = /dev/ttyUSB0       ; Linux/Mac
; monitor_port = COM3             ; Windows
```
