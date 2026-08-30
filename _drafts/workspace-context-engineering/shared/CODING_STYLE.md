# CODING_STYLE.md (Compartido)

## Convención de Tags (Todo Tree)

Todo el firmware del workspace usa estos tags de comentario, reconocidos por la
extensión Todo Tree (ver `shared/SOFTWARE.md` para la config del `settings.json`):

```cpp
#include <Arduino.h>

// TODO: Tarea pendiente de implementar
// FIXME: Algo que funciona mal y necesita corrección
// HACK: Solución temporal — refactorizar antes de producción
// BUG: Error conocido, aún sin solución
// NOTE: Información importante sobre el comportamiento del código
// WARN: Advertencia crítica de hardware o lógica

void setup() { Serial.begin(115200); }
void loop() { }
```

## Reglas generales

- **`WARN:`** se reserva para advertencias de hardware (voltajes, pines de boot,
  corriente máxima) — no para lógica de software. Eso es `NOTE:` o `FIXME:`.
- Antes de mergear, no debe quedar ningún `BUG:` sin ticket asociado en `TASKS.md`
  del proyecto correspondiente.
- Strings largos en proyectos AVR (Arduino Uno) **deben** usar la macro `F()` para
  no consumir SRAM — ver `HARDWARE.md` del proyecto Uno para el porqué.
