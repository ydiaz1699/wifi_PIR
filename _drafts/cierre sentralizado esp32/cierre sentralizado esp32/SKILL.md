//SKILL.md:
# Skill: PKE ESP32 con NimBLE

## Propósito

Esta skill define cómo generar o corregir código para un **PKE (Passive Keyless Entry)** basado en **ESP32** con la librería **NimBLE-Arduino**. El objetivo es autorizar un relé cuando se detecta un dispositivo BLE específico y revocar la autorización cuando el dispositivo deja de estar presente.

## Flujo de trabajo

1. **Detectar la intención**
   - El usuario pide un PKE para un dispositivo con datos específicos: MAC, RSSI, nombre y UUID.
   - La skill debe extraer esos valores y generar código que use esa configuración.

2. **Configurar el proyecto ESP32**
   - Usar `ESP32 Dev Module` / `esp32dev` en PlatformIO.
   - Usar `NimBLEDevice`, `NimBLEScan`, `NimBLEAdvertisedDevice`.

3. **Construir la lógica de escaneo**
   - Inicializar BLE con `NimBLEDevice::init()`.
   - Crear un callback heredando de `NimBLEScanCallbacks`.
   - En `onResult()`:
     - comprobar `dev != nullptr`
     - comparar `dev->getAddress().toString()` con la MAC autorizada
     - comparar `dev->getName()` con el nombre autorizado si existe
     - comprobar `dev->isAdvertisingService(TARGET_UUID)` si el UUID está disponible
     - leer el RSSI con `dev->getRSSI()` y comparar con el umbral
     - si cumple todos los filtros, marcar `autorizado = true` y actualizar `tUltDetect`

4. **Controlar salidas**
   - Usar `PIN_RELAY` para activar/desactivar el relé.
   - Usar `PIN_LED` para indicar estado de autorización.
   - En `loop()`, revocar autorización si `millis() - tUltDetect > TIMEOUT_MS`.

5. **Mantener estilo y calidad**
   - Comentarios y nombres de variables en español.
   - Mantener código claro y simple.
   - No introducir lógica innecesaria ni dependencias extras.
   - Asegurarse de que compile con `platformio run`.

## Decisiones clave

- Preferir la comparación de dirección BLE con `dev->getAddress().toString()` y `std::string` para evitar incompatibilidades de constructor.
- Usar `TARGET_UUID` en formato hexadecimal sin prefijo `0x` cuando la librería lo requiera.
- Mantener el umbral de RSSI en un valor definido por el usuario (`-78 dBm` en este caso).
- Revisar que el dispositivo también coincida por nombre si la publicidad lo incluye.
- Si el dispositivo no anuncia el UUID, el filtro de MAC y nombre sigue siendo válido, pero el UUID mejora la precisión.

## Criterios de salida

La respuesta debe incluir:
- Código completo y funcional para `src/main.cpp`.
- Valores actualizados para `TARGET_MAC`, `TARGET_NAME`, `TARGET_UUID`, `RSSI_UMBRAL`, `TIMEOUT_MS`.
- Breve explicación de los cambios y el comportamiento.
- Comentarios en español.
- Nada irrelevante.

## Ejemplos de prompts

- "Crear un PKE para dispositivo 09:1c:2d:e3:42:47 con nombre Watch10, UUID 0xe7fe y RSSI -78 dBm usando ESP32."
- "Corrige el código para que el ESP32 autorice solo el Watch10 con RSSI mayor a -78 y UUID 0xe7fe."
- "Genera `src/main.cpp` para un PKE ESP32 que revoca la autorización tras 5 segundos sin detección."

## Notas adicionales

- Esta skill es específica del repositorio `cierre sentralizado esp32` y debe conservar la estructura y estilo ya presentes.
- Si hay un archivo `copilot-instructions.md`, se debe usar como referencia para valores actuales y formato del proyecto.
