# Tarea: sacar los conceptos de "alarma" del core de IoTProtocol

## Contexto que debes tener antes de empezar

Este es un proyecto de firmware ESP8266 (PlatformIO/Arduino) con una librería
propia de comunicación llamada `IoTProtocol`, ubicada en `lib/IoTProtocol/`.
Esta librería es usada por dos proyectos de aplicación: `emisor_pir_unificado`
(sensor PIR + timbre) y `receptor_central_unificado` (central que recibe
eventos, los publica por MQTT y controla una bocina).

La intención a futuro es poder reutilizar `IoTProtocol` en proyectos
completamente distintos (por ejemplo un robot, un sensor de temperatura, un
gateway), sin tener que editar la librería cada vez. Para que eso sea
posible, la librería no debe conocer nada sobre "alarma doméstica": ni PIR,
ni timbre, ni humo, ni relés, ni bocina.

Ya se hizo una auditoría completa del código y se identificaron exactamente
tres puntos donde el core conoce conceptos de la aplicación de alarma. Fuera
de esos tres puntos, el resto de la librería (`IoTNode`, `IoTAuth`,
`IoTProtocol` framing/CRC/TLV, `IoTStorage`, `IoTConfigHandler`) ya está bien
separado y **no debe tocarse** en esta tarea salvo lo estrictamente necesario
para desenganchar los tres puntos contaminados.

No se trata de rediseñar el protocolo ni de cambiar el formato binario. Es
una extracción quirúrgica de tres bloques de código, moviéndolos del archivo
de la librería a un archivo nuevo dentro de cada proyecto de aplicación.

---

## Problema 1: `EventCode` vive dentro del core

**Dónde está:** `lib/IoTProtocol/IoTProtocol.h`, `enum class EventCode`.

**Qué tiene mal:** define valores como `MOTION`, `DOOR_OPEN`, `TIMBRE`,
`SMOKE`, `TAMPER`, `LOW_BATTERY`, `GAS_DETECTED`, etc. Estos son eventos
específicos de una alarma doméstica. Un proyecto futuro que no sea una
alarma (por ejemplo telemetría de un robot) no tiene ningún evento parecido
a estos, y aun así tendría que arrastrar este enum si usa la librería.

**Objetivo:** que `IoTProtocol.h` no defina ningún evento concreto. El core
solo debe transportar un identificador numérico de evento (un byte), sin
saber qué significa. El significado de ese número lo define cada proyecto
de aplicación por su cuenta.

**Cómo debería cambiar:**
- Sacar `enum class EventCode` completo de `IoTProtocol.h`.
- Todo el código de la librería que hoy recibe o produce un `EventCode`
  (particularmente `IoTNode::sendEvent`) debe pasar a trabajar con un tipo
  numérico simple (un entero de 8 bits) en vez de con el enum.
- Crear un archivo nuevo dentro de cada proyecto de aplicación que sí use
  eventos de alarma (por ahora, `emisor_pir_unificado` y
  `receptor_central_unificado`) que contenga el equivalente al antiguo
  `EventCode`, con los mismos valores numéricos que tenía antes (para no
  romper la compatibilidad de red con dispositivos que ya estén en campo).
- Todo el código de aplicación que hoy usa `EventCode::MOTION`,
  `EventCode::TIMBRE`, etc. debe seguir funcionando igual, pero refiriéndose
  al nuevo enum definido en la aplicación, no al de la librería.

**Resultado esperado:** `IoTProtocol.h` ya no contiene ningún nombre de
evento de alarma. La librería puede enviar y recibir "un evento genérico
identificado por número" sin saber su significado. La aplicación de alarma
sigue comportándose exactamente igual que antes, solo que ahora define sus
propios códigos de evento en su propio archivo.

---

## Problema 2: `DeviceType` vive dentro del core

**Dónde está:** `lib/IoTProtocol/IoTProtocol.h`, `enum class DeviceType`.

**Qué tiene mal:** define tipos como `PIR_SENSOR`, `RELAY`, `SMOKE_SENSOR`,
`DOOR_SENSOR`, `TEMP_SENSOR`, etc. Igual que con `EventCode`, esto es
vocabulario de una alarma doméstica metido en la librería genérica. Se usa
en el mensaje `HELLO` (discovery) para que un nodo le diga a la central qué
tipo de dispositivo es.

**Objetivo:** que el core no tenga que saber qué tipos de dispositivo
existen. El mecanismo de discovery (`HELLO`) debe seguir funcionando igual,
transportando un identificador numérico de tipo de dispositivo, pero sin que
la librería defina cuáles son los tipos válidos.

**Cómo debería cambiar:**
- Sacar `enum class DeviceType` completo de `IoTProtocol.h`.
- La firma de `IoTNode::sendHello` y cualquier otro punto de la librería que
  hoy recibe un `DeviceType` debe pasar a recibir un entero de 8 bits en su
  lugar.
- Definir el equivalente a `DeviceType` (con los mismos valores numéricos
  que antes) en el mismo archivo nuevo de aplicación mencionado en el
  Problema 1, ya que conceptualmente pertenece al mismo lugar.
- Actualizar `device_config.h` de `emisor_pir_unificado` (que hoy usa
  `DeviceType::PIR_SENSOR`) para que apunte al nuevo enum de aplicación.

**Resultado esperado:** `IoTProtocol.h` ya no contiene ningún tipo de
dispositivo de alarma. El mensaje HELLO sigue funcionando exactamente igual
en el wire (mismos bytes), pero la librería ya no necesita saber qué es un
"PIR_SENSOR".

---

## Problema 3: los `TlvTag` de estado (0xA0–0xA6) son específicos de alarma

**Dónde está:** `lib/IoTProtocol/IoTProtocol.h`, dentro de `enum class TlvTag`,
el bloque comentado como "State (0xA0–0xAF)": `STATE_MOTION`, `STATE_DOOR`,
`STATE_RELAY`, `STATE_BUTTON`, `STATE_ALARM`, `STATE_SMOKE`, `STATE_FLOOD`.

**Qué tiene mal:** a diferencia de los demás bloques de `TlvTag` (que son
genéricos: temperatura, batería, RSSI, contadores, diagnóstico), este bloque
específico describe el estado de sensores de una alarma doméstica. El resto
de `TlvTag` debe permanecer en el core sin cambios porque sí es reutilizable
por cualquier proyecto (ejemplo: cualquier dispositivo puede querer reportar
`FREE_HEAP` o `UPTIME_SEC`).

**Objetivo:** que el rango de tags 0xA0–0xAF dedicado a "estado de
aplicación" quede reservado por el core (para que dos aplicaciones distintas
no elijan el mismo número por accidente y se generen colisiones), pero que
los nombres concretos de esos tags salgan del core.

**Cómo debería cambiar:**
- Sacar del `enum class TlvTag` en `IoTProtocol.h` las entradas
  `STATE_MOTION`, `STATE_DOOR`, `STATE_RELAY`, `STATE_BUTTON`, `STATE_ALARM`,
  `STATE_SMOKE`, `STATE_FLOOD`.
- Dejar en `IoTProtocol.h`, en el lugar donde estaban, un comentario claro
  indicando que el rango 0xA0–0xAF está reservado para "tags de estado de
  aplicación" y que cada proyecto debe definir sus propios tags dentro de
  ese rango sin que la librería los conozca.
- Definir los mismos siete tags, con los mismos valores numéricos que tenían
  antes, en el archivo nuevo de aplicación (el mismo del Problema 1 y 2).
- Actualizar todo el código que hoy usa `TlvTag::STATE_MOTION`,
  `TlvTag::STATE_BUTTON`, etc. (aparece en `emisor_pir_unificado/main.cpp`
  al armar el `STATE_REPORT`, y en `receptor_central_unificado/event_handler.cpp`
  al leerlo) para que usen los tags nuevos definidos en la aplicación.

**Resultado esperado:** `IoTProtocol.h` ya no define ningún tag de estado
específico de alarma, solo documenta que ese rango de números está
reservado para que la aplicación lo use a su criterio. El formato del
paquete `STATE_REPORT` en el wire no cambia en absoluto.

---

## Dónde debe vivir el nuevo código de aplicación

Los tres bloques que se sacan del core (evento, tipo de dispositivo, tags de
estado) son conceptualmente una sola cosa: el vocabulario propio de la
aplicación de alarma. Deben quedar juntos en un único archivo nuevo,
compartido por `emisor_pir_unificado` y `receptor_central_unificado`, en un
lugar accesible por ambos proyectos (por ejemplo, junto al resto de
`lib/IoTProtocol` pero como una librería o archivo separado, claramente
distinguible de la librería genérica — no dentro de `IoTProtocol.h`).

Ese archivo debe dejar explícito en su propio nombre y en un comentario de
cabecera que es específico del dominio "alarma doméstica" y que un proyecto
distinto (robot, sensor de temperatura, etc.) no debería incluirlo, sino
crear el suyo propio siguiendo el mismo patrón.

---

## Restricciones que no se pueden violar

1. **Ningún byte cambia en el wire.** Todos los valores numéricos de los
   eventos, tipos de dispositivo y tags de estado deben ser exactamente los
   mismos que tenían antes de mover el código. Un emisor con el firmware
   viejo y una central con el firmware nuevo (o viceversa) deben poder
   seguir hablando entre sí sin cambios de comportamiento.
2. **No tocar `IoTNode`, `IoTAuth`, `IoTStorage`, `IoTConfigHandler` ni el
   framing de `IoTProtocol.cpp`/`IoTProtocol.h`** más allá de lo
   estrictamente necesario para que dejen de depender de `EventCode` y
   `DeviceType` (es decir, cambiar sus firmas de función para recibir un
   entero en vez del enum específico). No se debe aprovechar esta tarea para
   rediseñar la cola, el ACK, la deduplicación, el auth ni el storage.
3. **No agregar funcionalidades nuevas.** Esta tarea es exclusivamente mover
   código de un lugar a otro y ajustar las firmas de función que dependían
   de lo movido. No se debe agregar capability discovery, nuevos tipos de
   mensaje, nuevos campos, ni cambiar el comportamiento de ningún sensor.
4. **Los dos proyectos de aplicación deben seguir compilando y comportándose
   exactamente igual que antes**, incluyendo los logs por Serial (mismos
   mensajes, mismos valores).
5. **No modificar `docs/`, los archivos de `_drafts/`, ni ningún otro
   directorio del repositorio** que no sea `lib/IoTProtocol/`,
   `emisor_pir_unificado/` y `receptor_central_unificado/`.

---

## Cómo verificar que la tarea quedó bien hecha

- `IoTProtocol.h` no debe contener las palabras `MOTION`, `TIMBRE`,
  `DOOR_OPEN`, `PIR_SENSOR`, `RELAY`, `SMOKE_SENSOR` en ninguna parte.
- El nuevo archivo de aplicación debe contener exactamente los mismos
  valores numéricos que tenían antes `EventCode`, `DeviceType` y los siete
  `TlvTag` de estado, sin ningún valor cambiado, agregado ni eliminado.
- Los tres proyectos (`lib/IoTProtocol`, `emisor_pir_unificado`,
  `receptor_central_unificado`) deben compilar sin errores.
- Si existen tests (C++, Python, o el simulador mencionados en el proyecto),
  deben seguir pasando con el mismo resultado que antes del cambio.
- El comportamiento en runtime debe ser indistinguible del actual: mismos
  eventos enviados ante los mismos estímulos de hardware, mismos mensajes
  MQTT publicados, mismo formato de `STATE_REPORT`.

---

## Entregable esperado

Al terminar, se debe poder mostrar, para cada uno de los tres problemas
descritos arriba:

- Qué se sacó exactamente de `IoTProtocol.h` (o de qué otro archivo de la
  librería, si aplicó algún ajuste de firma).
- A qué archivo nuevo se movió y con qué nombre quedó.
- Qué archivos de `emisor_pir_unificado` y `receptor_central_unificado`
  se tuvieron que ajustar para apuntar al lugar nuevo, y qué cambió en cada
  uno (a nivel de "qué línea o qué referencia", no hace falta el diff
  completo si el resumen es claro).
- Confirmación de que los valores numéricos no cambiaron.
- Confirmación de que compiló y, si corresponde, que los tests existentes
  siguen pasando.
