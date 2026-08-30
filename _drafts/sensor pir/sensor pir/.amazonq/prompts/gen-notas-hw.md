# Genera notas de hardware (docs/notas.md)

## Objetivo
Crear un archivo `docs/notas.md` con la documentación técnica de hardware del proyecto actual.

## Instrucciones

1. **Analiza el proyecto**: Lee los archivos fuente del workspace para extraer:
   - Placa/microcontrolador usado
   - Sensores, módulos y actuadores
   - Pines y protocolos de comunicación
   - Voltajes y consideraciones eléctricas

2. **Genera el markdown** con esta estructura exacta:

```markdown
# [Componente principal] — [Placa]

## Hardware
- Lista de componentes con modelo exacto
- Pin asignado → función para cada uno

## Conexión
| Componente | Placa |
|------------|-------|
| Pin X      | Pin Y |
(una fila por cada cable/conexión física)

## Notas
- Tiempos de calibración o inicialización
- Configuraciones físicas (jumpers, switches, dip)
- Conflictos de pines conocidos y alternativas
- Limitaciones de voltaje o corriente
- Cualquier gotcha relevante para el montaje
```

3. **Guarda** en `docs/notas.md`. Crea la carpeta `docs/` si no existe.

## Criterios de calidad
- Solo incluir información verificable desde el código o datasheets conocidos
- Ser conciso: máximo 30 líneas
- Usar terminología estándar del fabricante para nombres de pines
- Si hay múltiples componentes, crear una sección ## Conexión por cada uno
- Incluir en Notas solo información que NO sea obvia del datasheet (gotchas, tips prácticos)
