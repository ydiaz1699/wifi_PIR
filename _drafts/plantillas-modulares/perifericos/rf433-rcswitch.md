# 🔌 Periférico: RF 433MHz (RCSwitch)

| Atributo | Valor |
|----------|-------|
| **Categoría** | Comunicación inalámbrica |
| **Voltaje** | 5V (para módulos típicos) |
| **Protocolo** | Digital / INT (interrupción) |
| **Pines** | D2 (INT0) en Arduino Uno para receptor |
| **Librería** | `RCSwitch` |
| **Notas** | Transmisor: ~3V-12V (mayor voltaje = mayor alcance). Receptor: superregenerativo (ruidoso) o superheterodino (mejor). No encriptado — solo para proyectos domésticos. |

> Nota: si tu proyecto usa `RadioHead` + `RH_ASK` en vez de `RCSwitch`
> (por ejemplo el proyecto del emisor/receptor con FS1000A/RXB12),
> el pin de RX no puede ser D0/D1 (UART) — usar un pin dummy libre
> si el driver no recibe realmente.
