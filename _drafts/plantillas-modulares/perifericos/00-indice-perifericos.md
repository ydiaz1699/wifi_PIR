# 📚 Índice de Periféricos — Catálogo de Hardware

> Pegar junto con `00-core.md` y el archivo de placa **solo los periféricos
> que uses en el proyecto actual**, no el catálogo completo.

| Archivo | Módulo | Categoría | Voltaje | Protocolo |
|---------|--------|-----------|---------|-----------|
| `nrf24l01.md` | NRF24L01+ | Comunicación RF | 3.3V | SPI |
| `dht22.md` | DHT22 / AM2302 | Sensor ambiental | 3.3V–5.5V | Single-wire |
| `hc-sr04.md` | HC-SR04 | Sensor de distancia | 5V | Digital |
| `ssd1306.md` | SSD1306 OLED | Display | 3.3V–5V | I2C / SPI |
| `rc522.md` | RC-522 | RFID | 3.3V | SPI |
| `rele-5v.md` | Módulo Relé 5V | Actuador | 3.3V–5V | Digital |
| `mpu6050.md` | MPU6050 | IMU | 3.3V–5V | I2C |
| `rf433-rcswitch.md` | RF 433MHz | Comunicación inalámbrica | 5V | Digital/INT |

---

## 📝 Cómo agregar un módulo nuevo al catálogo

1. Copiar la estructura de un archivo existente en `perifericos/` como plantilla
2. Completar: voltaje, protocolo, pines, librería, notas críticas
3. Documentar niveles lógicos y conversiones necesarias
4. Agregar advertencias de seguridad si aplica
5. Agregar la fila correspondiente a la tabla de este archivo
