# Host compile check

Este check usa las fuentes reales de `lib/IoTProtocol` y `lib/AlarmProfile`, pero reemplaza Arduino/ESP8266/WiFiUDP por stubs mínimos en `compat/`.

Valida tres capas:

1. conversiones `AlarmProfile::toWire()` y `toCoreTlvTag()`;
2. construcción, serialización, deserialización, lectura TLV y rechazo de CRC/paquetes truncados;
3. compilación/enlace de `IoTNode` y sus llamadas reales a `begin()`, `sendHello()` y `sendEvent()`.

No valida una red WiFi/UDP real ni el comportamiento de un ESP8266 físico.

## Ejecutar

Desde la raíz del repositorio:

```bash
g++ -std=c++14 -Wall -Wextra \
  -Itests/host_compile/compat \
  -Ilib/IoTProtocol \
  -Ilib/AlarmProfile \
  tests/host_compile/check.cpp \
  tests/host_compile/compat/arduino_stub.cpp \
  lib/IoTProtocol/IoTProtocol.cpp \
  lib/IoTProtocol/IoTNode.cpp \
  -o /tmp/wifi_pir_host_check

/tmp/wifi_pir_host_check
```

El resultado esperado es:

```text
OK: host compile check wifi_PIR
```

Los warnings sobre `memset` en `IoTNode.cpp` son advertencias del compilador sobre el código existente; no indican que este check haya fallado. Cualquier `FAIL:` o un código de salida distinto de `0` sí indica un problema.
