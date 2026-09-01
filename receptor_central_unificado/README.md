# Candidata unificada V3+V4 — central

Este directorio es la candidata unificada V3+V4 de la central, construida sobre la base actual de `receptor_central_v4/`.

- Conserva el núcleo común de `lib/IoTProtocol` y el comportamiento V3 compatible de MQTT, Home Assistant, alarma y bocina.
- `receptor_bocina/` permanece sin modificaciones como respaldo congelado de V3.
- Esta candidata no debe promocionarse ni sustituir a V3 hasta pasar el gate documentado en [`docs/MATRIZ_UNIFICACION_V3_V4.md`](../docs/MATRIZ_UNIFICACION_V3_V4.md).
- Los secretos y artefactos de compilación no forman parte de esta candidata; usa `secrets.h` y `network_config.h` locales/configurados fuera del control de versiones al compilar.

## Compilación

Desde la raíz del repositorio:

```bash
python3 -m platformio run -d receptor_central_unificado -e receptor_central_unificado
python3 -m platformio run -d receptor_central_unificado -e receptor_central_unificado_ota
```

El entorno OTA conserva la configuración de V4. La promoción de OTA requiere validar por separado su protección, conectividad entre subredes y recuperación serial.
