# Bootstrap del canal reliable V4

## Contrato

Cuando el emisor V4 arranca, el primer `HELLO` se envía como reliable. La central puede responder con un `ACK` auténtico y estructuralmente válido antes de que el emisor tenga un `RemoteDevice` para la central.

`IoTNode` acepta ese primer ACK únicamente si se cumplen todas estas condiciones:

- existe un reliable activo y está esperando ACK;
- `SRC` coincide con el destino del reliable;
- `SEQ` coincide con el paquete reliable en vuelo;
- `BOOT_ID` del ACK es distinto de cero;
- no había una sesión `BOOT_ID` conocida para ese remoto (un remoto creado por `registerRemote()` con `bootId=0` sigue siendo desconocido).

La autenticación, cuando está habilitada, y la validación estructural ocurren antes de esta lógica. Al aceptar el bootstrap, el ACK crea o actualiza el `RemoteDevice`, aprende el `BOOT_ID` recibido y lo copia a `expectedBootId` del canal reliable. El endpoint del ACK también queda registrado mediante el marcado normal de liveness.

## Validación posterior y `BOOT_ID=0`

Después del bootstrap, los ACK requieren el `BOOT_ID` conocido tanto en `RemoteDevice` como en `expectedBootId`. Un ACK de otra sesión, con `BOOT_ID` antiguo, remoto desconocido fuera de un reliable activo, `SRC/SEQ` no coincidentes o sin reliable esperando confirmación se ignora y no crea sesión.

`BOOT_ID=0` permanece reservado como sentinel interno de `registerRemote(id, ip, port)`: significa “endpoint conocido, sesión aún desconocida”. No es válido en el wire para ningún paquete entrante, no puede confirmar un reliable ni convertirse en una sesión aprendida. Un `begin(0)` local también se normaliza al valor reservado no cero (`1`).

Este comportamiento se aplica a la biblioteca `IoTNode` usada por `emisor_pir_v4` y `receptor_central_v4`; no modifica V3. La suite host existente cubre el codec `IoTProtocol`, pero no puede probar honestamente el flujo UDP de `IoTNode` porque no incluye un entorno host para `WiFiUDP` ni un simulador de ambos nodos.
