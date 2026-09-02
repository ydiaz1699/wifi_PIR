# wifi_PIR Virtual Lab

`tools/virtual_lab` es un laboratorio funcional determinista para validar la interacción entre el emisor PIR/timbre, la red UDP, la central, la bocina, MQTT y Home Assistant sin hardware.

## Ejecutar

Desde la raíz del repositorio:

```bash
python3 -m tools.virtual_lab --list
python3 -m tools.virtual_lab --scenario all
python3 -m tools.virtual_lab --scenario simultaneous-inputs
python3 -m tools.virtual_lab --scenario mqtt-local-recovery
python3 -m tools.virtual_lab --scenario all --json
```

No requiere paquetes externos: usa únicamente la biblioteca estándar de Python y reutiliza el codec/HMAC de `tools/iot_simulator.py`.

## Escenarios

- `normal-ha`: PIR, bocina temporizada, MQTT y siete entidades de discovery.
- `pir-sustained`: PIR mantenido en HIGH; no repite eventos.
- `simultaneous-inputs`: PIR y timbre independientes en la misma ventana.
- `loss-first-event`: se pierde el primer EVENT y se recupera por retransmisión.
- `ack-loss-duplicate`: se pierde el ACK; el duplicado se reconoce sin repetir el efecto.
- `mqtt-local-recovery`: la alarma sigue funcionando en LOCAL y recupera HA al volver el broker.
- `auth-required`: HELLO, EVENT y ACK con HMAC obligatoria.
- `replay-rejected`: un evento fuera de la ventana BOOT_ID/SEQ no vuelve a activar la alarma.

Los tiempos son virtuales y están reducidos para que los escenarios terminen rápido. La lógica conserva las relaciones importantes del firmware: antirrebote independiente, reliable con un paquete en vuelo, ACK real, deduplicación, prioridad UDP sobre MQTT, modo LOCAL/HA, bocina no bloqueante y discovery retained.

## Arquitectura

```text
VirtualClock
    ├── VirtualNetwork      UDP lógico + pérdidas, duplicados y corrupción
    ├── VirtualEmitter      PIR/timbre, flancos, antirrebote y reliable
    ├── VirtualCentral      registry, dedup, eventos y alarma
    ├── VirtualBuzzer       ON/OFF temporizado y trazas
    ├── MemoryMqttBroker    retained, subscriptions y disponibilidad
    └── VirtualHA           discovery, estados y comandos
```

La red captura cada datagrama y la traza registra transmisiones, drops, retries, ACKs, entregas, duplicados, bocina, MQTT y cambios de modo.

## Qué sí valida

- Flujo PIR/timbre → EVENT → ACK → central.
- PIR sostenido y antirrebote.
- Eventos simultáneos.
- Pérdida de paquetes y retransmisiones.
- Pérdida de ACK y deduplicación.
- BOOT_ID/SEQ y replay fuera de ventana.
- Autenticación HMAC requerida.
- Bocina LOCAL sin broker.
- Recuperación de MQTT y publicación de discovery para HA.

## Qué no sustituye

Este laboratorio **no es Proteus** y no pretende simular la electrónica del ESP8266. No modela voltajes, consumo, ruido eléctrico, temporización de GPIO, WiFi de radio ni ejecuta directamente el firmware C++/Arduino. Es un modelo de integración de protocolo y aplicación para encontrar errores de flujo antes de probar en los equipos reales.

La validación final todavía requiere compilar el firmware con PlatformIO y probar el hardware real. El simulador no debe usarse como evidencia de que una compilación o una prueba física pasó.
