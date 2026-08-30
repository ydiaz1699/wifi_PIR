//copilot-instructions.md:
# Copilot Instructions: PKE ESP32

Este repositorio es un proyecto de **PKE (Passive Keyless Entry) para ESP32** usando la librería `NimBLE-Arduino`.

## Objetivo
Generar o corregir código para un ESP32 que autorice un relé cuando se detecte un dispositivo BLE específico cerca, y revoque la autorización cuando el dispositivo desaparezca.

## Datos del dispositivo autorizado
- MAC: `09:1c:2d:e3:42:47`
- Nombre: `Watch10`
- UUID: `0xe7fe`
- RSSI umbral: `-80 dBm`
- Timeout de revocación: `5000 ms`

## Reglas de implementación
- Usar `NimBLEDevice`, `NimBLEScan` y `NimBLEAdvertisedDevice`.
- Filtrar el dispositivo autorizado por MAC y/o UUID.
- Comparar direcciones BLE usando `dev->getAddress().toString()` o la API correcta de NimBLE.
- Actualizar el estado de autorización en `onResult()` y revocar cuando no haya detección por más de `TIMEOUT_MS`.
- Controlar el relé con `PIN_RELAY` y el LED de estado con `PIN_LED`.
- Mantener comentarios y nombres de variables en español.
- El código debe compilar con `platformio run` en board `esp32dev`.
- Preferir lógica simple, clara y compatible con la librería NimBLE actual.

## Estilo de respuesta
- Responder en español.
- Incluir solo el código necesario y una breve explicación.
- No agregar contenido irrelevante.

## Nota
Este archivo es una guía para agentes de asistencia de código y no forma parte del firmware. Mantenerlo actualizado con la configuración del proyecto.
