# Checklist: agregar una placa nueva al catálogo

Sigue esto cuando tu biblioteca de placas crezca (ej. agregas un ESP32, una
Raspberry Pi Pico, un STM32, etc.)

## 1. ¿Es realmente una placa nueva?

- [ ] Confirma que **no es la misma placa física** que ya está en `boards/`.
      Mismo MCU con distinto breakout (ej. "ESP32 DevKit" vs "ESP32-WROOM barebone")
      sí cuenta como placa nueva — el pinout físico cambia.
- [ ] Si es la misma placa que ya tienes pero para *otro proyecto*, **no crees
      un archivo nuevo** — el proyecto nuevo solo referencia el existente.

## 2. Crea `boards/<nombre-placa>.md`

Copia esta estructura mínima:

```markdown
# Board: <Nombre completo>

## Identidad
- MCU:
- Clock:
- RAM:
- Flash:
- Alimentación:

## Conectividad
- (WiFi/BT/ninguna, USB-UART chip)

## Niveles de Voltaje
- Lógico:
- ADC:
- Regla de oro para interfaces con otros voltajes:

## Mapeo de Pines (genérico de la placa)
| Función | Pin/GPIO | Notas |

## Consideraciones críticas de esta placa
- (lo que romper aquí es irreversible o difícil de debuggear)

## `platformio.ini` — bloque `[env:]` de referencia
```ini
[env:...]
```

## Proyectos que usan esta placa
- (se completa cuando exista al menos un proyecto)
```

## 3. Registra la placa en `boards/README.md`

Agrega una fila a la tabla con el link al archivo nuevo.

## 4. Cuando crees el primer proyecto con esta placa

- [ ] Usa `_template-proyecto/` como base (ver ese folder)
- [ ] En `.ai/HARDWARE.md` del proyecto, **enlaza** a `../../boards/<placa>.md`
      en vez de copiar la tabla de pines
- [ ] Agrega la ruta del proyecto en la sección "Proyectos que usan esta placa"
      del archivo de catálogo
