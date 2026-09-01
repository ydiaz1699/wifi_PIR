# Simulador reproducible del wire V4

`tools/iot_simulator.py` es un harness en Python estándar para la unificación V4.1. No importa, compila ni pretende ejecutar `IoTNode` C++ o hardware: modela de forma explícita el frame wire, el receptor y una política determinista de reintento.

## Ejecución

Desde la raíz de `wifi_PIR`:

```bash
python3 tools/iot_simulator.py --scenario all
```

Para ver los escenarios disponibles o ejecutar uno:

```bash
python3 tools/iot_simulator.py --list
python3 tools/iot_simulator.py --scenario hmac-policies
```

La salida es legible y termina con un código distinto de cero si algún escenario falla. Los escenarios cubren: paquete válido y round-trip, CRC inválido, TLV truncado, HMAC válida/incorrecta/ausente con política `required`, duplicado, SEQ fuera de orden dentro de ventana, replay fuera de ventana, BOOT_ID nuevo, pérdida/reintento y dos sensores simultáneos.

## Tests

El repositorio ya tiene pruebas PlatformIO C++; las pruebas nuevas son independientes y usan `unittest` de la biblioteca estándar:

```bash
python3 -m unittest discover -s tests/python -p 'test_*.py' -v
```

La clave HMAC del simulador (`wifi-pir-python-test-key`) es un valor fijo exclusivo para tests y no se lee desde `secrets.h`. No usarla en dispositivos.

## Correspondencia con V4.1

- `MAGIC`: `A5 5A`; versión wire `0x41`.
- Cabecera: 14 bytes, enteros big-endian, `BOOT_ID` de 16 bits y `SEQ` de 32 bits.
- Payload máximo: 64 bytes de TLV `TAG | LENGTH | VALUE`.
- CRC16-CCITT: polinomio `0x1021`, inicial `0xFFFF`, sobre MAGIC hasta el payload.
- HMAC-SHA256 truncado a cuatro bytes: TLV `0xF0` final, marcado por `AUTHENTICATED (0x10)`, cubriendo los campos de cabecera definidos por `IoTAuth.cpp` y los TLV anteriores.
- Ventana de deduplicación: ocho SEQ por sesión `src + BOOT_ID` para mensajes `RELIABLE`; un BOOT_ID nuevo reinicia la ventana.

El simulador solo añade archivos Python/documentación. No modifica `emisor_pir/` ni `receptor_bocina/` (V3).
