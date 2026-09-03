# Candidata unificada V3+V4 — emisor

Este directorio es la candidata unificada V3+V4 del emisor, construida sobre la base actual de `emisor_pir_unificado/`.

- Comparte `lib/IoTProtocol` con el receptor central unificado y el resto de la línea V4.
- `legacy/emisor_pir/` permanece sin modificaciones como respaldo congelado de V3.
- Esta candidata no debe promocionarse ni sustituir a V3 hasta pasar el gate documentado en [`docs/MATRIZ_UNIFICACION_V3_V4.md`](../docs/MATRIZ_UNIFICACION_V3_V4.md).
- Los secretos y artefactos de compilación no forman parte de esta candidata; usa la configuración compartida de la raíz al compilar localmente.
