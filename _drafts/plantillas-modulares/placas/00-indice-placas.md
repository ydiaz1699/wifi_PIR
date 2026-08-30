# 📚 Índice de Placas — Catálogo de Hardware

> Pegar junto con `00-core.md` **solo el archivo de la placa que estés usando**
> en el proyecto actual. Esta tabla es para decidir cuál, no para pegarla completa
> salvo que estés comparando opciones.

| Archivo | Placa | MCU | Clock | RAM | Flash | Vcc | Consumo Activo | Deep Sleep | WiFi/BT | Precio |
|---------|-------|-----|-------|-----|-------|-----|----------------|------------|---------|--------|
| `d1-mini-pro.md` | D1 Mini Pro | ESP8266 | 160 MHz | 80 KB | 16 MB | 3.3V | ~80 mA | ~20 µA | WiFi | $ |
| `nodemcu-v3.md` | NodeMCU v3 | ESP8266 | 80 MHz | 80 KB | 4 MB | 3.3V | ~80 mA | ~20 µA | WiFi | $ |
| `esp-01s.md` | ESP-01S | ESP8266 | 80 MHz | 80 KB | 1 MB | 3.3V | ~80 mA | ~20 µA | WiFi | $ |
| `esp32-devkit.md` | ESP32 DevKit | ESP32 | 240 MHz | 520 KB | 4 MB | 3.3V | ~120 mA | ~5 µA | WiFi+BT | $$ |
| `esp32-s3.md` | ESP32-S3 | ESP32-S3 | 240 MHz | 512 KB+8MB | 8 MB | 3.3V | ~150 mA | ~7 µA | WiFi+BT5 | $$$ |
| `esp32-c3.md` | ESP32-C3 | RISC-V | 160 MHz | 400 KB | 4 MB | 3.3V | ~90 mA | ~5 µA | WiFi+BT5 | $ |
| `arduino-uno.md` | Arduino Uno | ATmega328P | 16 MHz | 2 KB | 32 KB | 5V | ~45 mA | N/A | — | $ |
| `arduino-nano.md` | Arduino Nano | ATmega328P | 16 MHz | 2 KB | 32 KB | 5V | ~45 mA | N/A | — | $ |
| `arduino-mega.md` | Arduino Mega | ATmega2560 | 16 MHz | 8 KB | 256 KB | 5V | ~50 mA | N/A | — | $$ |
| `stm32-bluepill.md` | Blue Pill | STM32F103 | 72 MHz | 20 KB | 64 KB | 3.3V | ~35 mA | ~1 µA | — | $ |
| `stm32-blackpill.md` | Black Pill | STM32F401/F411 | 84-100 MHz | 96-128 KB | 256-512 KB | 3.3V | ~40 mA | ~1 µA | — | $$ |
| `rp2040-pico.md` | Pi Pico | RP2040 | 133 MHz | 264 KB | 2 MB | 3.3V | ~90 mA | ~1.9 µA | — | $ |

---

## 🔧 Criterios de Selección

**ESP8266 (D1 Mini, NodeMCU):**
- ✅ Necesitas WiFi barato
- ✅ Proyecto simple con IoT básico
- ✅ Consumo moderado aceptable
- ❌ No usar si necesitas Bluetooth o mucho procesamiento

**ESP32:**
- ✅ WiFi + Bluetooth necesarios
- ✅ Procesamiento dual-core requerido
- ✅ Más RAM/Flash necesarios
- ✅ ADC de mayor resolución (12-bit)
- ❌ Consumo más alto que ESP8266

**ESP32-S3:**
- ✅ AI/ML edge (instrucciones vectoriales)
- ✅ PSRAM grande (8MB) para buffers/cámara
- ✅ BLE 5.0 con mayor throughput
- ✅ USB nativo (JTAG/CDC)
- ❌ Consumo más alto, precio superior

**ESP32-C3:**
- ✅ WiFi+BLE 5.0 a bajo costo
- ✅ RISC-V (ecosistema abierto)
- ✅ Consumo eficiente
- ❌ Single-core, menos GPIOs

**AVR (Uno, Nano, Mega):**
- ✅ Proyectos educativos o simples
- ✅ Necesitas 5V lógico (sensores antiguos)
- ✅ Baja velocidad aceptable
- ✅ Compatibilidad con shields
- ❌ Sin conectividad inalámbrica nativa
- ❌ RAM muy limitada

**STM32 Blue Pill:**
- ✅ Procesamiento ARM 32-bit necesario
- ✅ ADC/DAC de alta resolución
- ✅ Control en tiempo real crítico
- ✅ Precio mínimo
- ❌ Curva de aprendizaje más pronunciada
- ❌ Programación requiere ST-Link

**STM32 Black Pill:**
- ✅ FPU (punto flotante) para DSP
- ✅ USB nativo sin UART bridge
- ✅ Más RAM/Flash que Blue Pill
- ✅ USB-C moderno
- ❌ Ligeramente más caro que Blue Pill

**RP2040:**
- ✅ PIO para protocolos personalizados
- ✅ Dual-core a bajo costo
- ✅ USB-C nativo
- ✅ Excelente documentación
- ❌ Sin WiFi/Bluetooth nativo (agregar módulo)

---

## 📝 Cómo agregar una placa nueva al catálogo

1. Copiar la estructura de un archivo existente en `placas/` como plantilla
2. Completar especificaciones técnicas
3. Agregar mapeo de pines completo
4. Documentar consideraciones críticas
5. Incluir `platformio.ini` base
6. Agregar la fila correspondiente a la tabla comparativa de este archivo
