Aquí están los tres archivos completos actualizados para que los copies tal cual sobre tu copia local.

## 1. `docs/BUGS_FIXED.md` — solo agregar al final (después de BUG-010)Copia y pega este bloque completo al final de tu `docs/BUGS_FIXED.md` (después de `## BUG-010` y su regla):

```markdown

---

## BUG-011: Discovery MQTT publica "null" en vez del JSON — el dispositivo no aparece en Home Assistant

**Síntoma**: El receptor se conecta al broker MQTT correctamente (`MQTT conectado OK`, log muestra `Discovery MQTT publicado (7 entidades, 1 dispositivo)`), pero el dispositivo "Alarma Bocina" **nunca aparece** en Home Assistant (Ajustes → Dispositivos y servicios → Dispositivos). Suscribiéndose al broker con `mosquitto_sub -t 'homeassistant/#' --retained-only`, cada topic de config muestra literalmente el texto `null` en vez de un JSON con los campos del dispositivo.

**Causa raíz**: En `mqtt_discovery.cpp`, `publicarEntidad()` llamaba a `doc.as<JsonObject>()` sobre un `StaticJsonDocument` recién creado (vacío/null):
```cpp
StaticJsonDocument<768> doc;
llenar(doc.as<JsonObject>());        // ❌ BUG
agregarDevice(doc.as<JsonObject>()); // ❌ BUG
```
En ArduinoJson, `.as<T>()` sobre un documento vacío **no lo convierte** a tipo objeto — devuelve una vista inválida, y cualquier escritura sobre ella (`o["name"] = ...`) se descarta silenciosamente sin error de compilación ni de runtime. El documento se queda con valor `null`, y `serializeJson(doc, payload)` termina serializando el texto `"null"` en vez del objeto esperado. La conexión MQTT y el `publish()` funcionan perfectamente — el bug está en la construcción del JSON, no en la red.

**Por qué es tan fácil pasarlo por alto**: no hay ningún error de compilación ni excepción en runtime. El log sigue diciendo "Discovery MQTT publicado" porque `mqtt.publish()` efectivamente envía algo (el string "null"), solo que no es el payload correcto. El bug solo se nota mirando el contenido real del mensaje retenido en el broker.

**Solución**: Usar `.to<JsonObject>()` **una sola vez** para convertir el documento a tipo objeto, y reutilizar esa misma variable:
```cpp
StaticJsonDocument<768> doc;
JsonObject o = doc.to<JsonObject>();  // ✅ .to<>() SÍ convierte el doc a objeto
llenar(o);
agregarDevice(o);
```

**Regla**: En ArduinoJson, para inicializar un `JsonDocument` vacío como objeto (o array) que se va a llenar, usar siempre `.to<JsonObject>()` (o `.to<JsonArray>()`), **nunca** `.as<JsonObject>()`. `.as<T>()` es para *leer/castear* un valor que ya existe con ese tipo; `.to<T>()` es para *inicializar/convertir* el documento a ese tipo. Llamar `.as<JsonObject>()` dos veces sobre el mismo doc (una por cada función que lo llena) es una señal de alerta de este bug — si de verdad fuera un objeto válido tras la primera llamada, no haría falta "re-castear".

**Cómo detectarlo rápido en el futuro**: si un dispositivo MQTT no aparece en Home Assistant pese a que el log confirma conexión y publish exitosos, **verificar el contenido real del mensaje retenido** en el broker (no solo el log del ESP):
```bash
docker run --rm --network host eclipse-mosquitto mosquitto_sub \
  -h <IP_BROKER> -p 1883 -u <user> -P <pass> \
  -t 'homeassistant/#' -v --retained-only
```
Si el payload es `null`, vacío, o no es un JSON válido, el problema está en la construcción del documento (ArduinoJson), no en MQTT/red/Home Assistant.
```

## 2. `docs/CHANGELOG.md` — agregar al final del archivo

Copia y pega esto al final de tu `CHANGELOG.md` (después de `## V4.3`):

```markdown

## V3.5.2 — Fix discovery MQTT (receptor_bocina)
- Fix: `mqtt_discovery.cpp` publicaba literalmente `"null"` en los 7 topics de config de Home Assistant, en vez del JSON de la entidad (ver BUG-011)
- Causa: uso de `doc.as<JsonObject>()` en vez de `doc.to<JsonObject>()` sobre un `StaticJsonDocument` vacío (ArduinoJson descarta silenciosamente las escrituras en ese caso)
- Ahora el dispositivo "Alarma Bocina" y sus 7 entidades aparecen correctamente en Home Assistant tras la reconexión MQTT
```

## 3. `docs/ARCHITECTURE.md` — reemplazar la sección "### Red"

Busca en tu archivo esta sección (cerca del final, dentro de `## Hardware`):

```markdown
### Red
| Dispositivo | IP | Puerto |
|-------------|-------|--------|
| Emisor PIR01 | 192.168.0.200 | UDP 4210 |
| Receptor Central | 192.168.0.201 | UDP 4210 |
| Router/Gateway | 192.168.0.1 | — |
| MQTT Broker | 192.168.0.50 | TCP 1883 |

---
```

Y **reemplázala completa** por esto:

```markdown
### Red
| Dispositivo | IP | Puerto |
|-------------|-------|--------|
| Emisor PIR01 | 192.168.0.200 | UDP 4210 |
| Receptor Central (bocina) | 192.168.0.201 | UDP 4210 |
| Router Principal / Gateway | 192.168.0.1 | — |
| Repetidor 2 (emisor) | 192.168.0.2 | — |
| Repetidor 3 = Huawei B622 (LTE) | 192.168.0.15 (WAN, DHCP) / 192.168.1.1 (LAN interna) | — |
| NAS (Docker: EMQX, Home Assistant) | 192.168.1.200 | — |
| MQTT Broker (EMQX) | **192.168.0.15** ← usar esta IP en `MQTT_SERVER` | TCP 1883 (via DMZ) |

### ⚠️ Topología de red: doble NAT en Repetidor 3 (Huawei B622)

El **Repetidor 3 es un router LTE Huawei B622**, no un repetidor/AP tradicional. A diferencia de los Repetidores 2, que están en modo bridge/AP dentro de la misma subred `192.168.0.0/24`, el B622 **crea su propia subred NAT'eada** (`192.168.1.0/24`) detrás del router Principal — un "doble NAT" clásico. El B622 **no tiene modo bridge/AP real** para su puerto Ethernet (es normal en CPEs LTE: solo ofrecen modos de WAN — LTE / Ethernet WAN / WiFi WAN — pero siempre como router). Intentar forzarlo apagando su DHCP interno NO lo convierte en bridge: solo rompe la asignación de IP a sus clientes WiFi (síntoma: ícono "!" de "sin internet" en los dispositivos conectados a él).

**Consecuencia práctica**: el NAS (con EMQX y Home Assistant) vive en `192.168.1.200`, **inalcanzable directamente** desde la red de sensores `192.168.0.0/24` donde están el emisor y el receptor. El B622 sí puede iniciar conexiones salientes hacia `192.168.0.x` (por eso `ping` funciona en ambos sentidos), pero el tráfico **entrante** hacia `192.168.1.x` iniciado desde `192.168.0.x` requiere reenvío explícito.

**Solución implementada**: DMZ en el B622 (`192.168.1.1` → Seguridad → Configuración de DMZ → host `192.168.1.200`). Esto reenvía todo el tráfico entrante que llega al B622 por su lado `192.168.0.x` hacia el NAS, sin tener que abrir puerto por puerto cada vez que se agregue un nuevo servicio (HA en 8123, dashboard EMQX en 18083, etc.).

**IP del B622 dentro de `192.168.0.x` puede cambiar**: como el B622 obtiene esa IP por DHCP normal del router Principal (no hay forma de fijarla desde el propio B622), un reinicio puede asignarle una IP distinta a `192.168.0.15` si esa dirección ya no está libre en el pool. **Pendiente**: reservar `192.168.0.15` (o la que se use) para la MAC del B622 en el DHCP del router Principal, para que quede estable.

**Regla para futuras migraciones de red**: si se agrega un nuevo repetidor/AP, verificar primero si es un verdadero repetidor WiFi (modo bridge nativo, misma subred) o un router/CPE (crea su propia subred con NAT, requiere DMZ o port forwarding). Confirmarlo revisando el Gateway que reciben los clientes conectados a él: si coincide con el del router Principal → bridge (ok). Si es distinto → double NAT (requiere DMZ/forwarding).

---
```

Cuando termines de aplicarlo, avísame y preparamos el commit para subirlo a GitHub.