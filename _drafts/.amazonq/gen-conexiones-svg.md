# Genera diagrama de conexiones (drawio SVG)

## Objetivo
Crear un archivo `docs/conexiones.drawio.svg` con el diagrama de conexiones del proyecto actual.

## Instrucciones

1. **Analiza el proyecto**: Lee los archivos fuente del workspace para identificar:
   - Microcontrolador/placa principal (Arduino, ESP32, RPi, STM32, etc.)
   - Sensores, actuadores y módulos conectados
   - Pines utilizados (digitales, analógicos, I2C, SPI, UART)
   - Voltajes de operación

2. **Genera el SVG** en formato drawio editable (con el XML embebido en el atributo `content` del tag `<svg>`):
   - Un rectángulo redondeado por cada componente (color azul `#dae8fc` para la placa principal, verde `#d5e8d4` para periféricos, naranja `#fff2cc` para actuadores)
   - Etiquetas de pines visibles en cada componente
   - Flechas de conexión entre pines con colores por tipo:
     - Rojo `#ff0000` → alimentación (VCC/5V/3.3V)
     - Negro `#000000` → GND
     - Verde `#00aa00` → señal/datos
     - Azul `#0000ff` → bus (I2C/SPI)
   - Etiqueta en cada flecha indicando el tipo de conexión
   - Fuente Helvetica, tamaño 12px para pines, 14px bold para nombres de componentes

3. **Guarda** el archivo en `docs/conexiones.drawio.svg`. Crea la carpeta `docs/` si no existe.

## Criterios de calidad
- El SVG debe ser abrible y editable en draw.io/diagrams.net
- Cada componente debe mostrar SOLO los pines que se usan en el proyecto
- El layout debe ser horizontal: periféricos a la izquierda, placa principal a la derecha
- Dimensiones proporcionales al número de componentes (~160px ancho por componente, ~40px por pin de alto)
