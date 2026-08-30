# Prompt reutilizable: generar archivos de contexto para agentes de IA
# (copilot-instructions.md + SKILL.md) a partir del código actual

Usar este prompt junto con el código fuente del proyecto (main.cpp / .ino / .py, etc.)
cuando se quiera que cualquier asistente de IA (Copilot, Cursor, Claude Code, Claude.ai)
entienda automáticamente las reglas y el dominio del proyecto sin tener que explicarlas
en cada conversación nueva.

---

## Prompt

Analiza el código fuente que te adjunto de este proyecto. A partir de él —y de
lo que yo te indique sobre el propósito general— genera dos archivos de contexto
para agentes de IA:

### 1. `copilot-instructions.md`
Instrucciones generales de repositorio, con estas secciones:
- **Objetivo del proyecto**: qué hace el sistema, en 2-3 líneas, inferido del código.
- **Datos/parámetros de configuración actuales**: constantes, pines, umbrales,
  identificadores (MAC/UUID/SSID/etc.) que ya existen en el código — con sus
  valores reales, no placeholders.
- **Reglas de implementación**: patrones que el código ya sigue y que deben
  mantenerse (ej. librerías usadas, forma de manejar interrupciones, callbacks,
  estructura de pines) para que futuras sugerencias sean consistentes.
- **Convenciones de estilo**: idioma de comentarios/variables, nivel de verbosidad,
  framework/board objetivo, requisito de que compile.
- **Estilo de respuesta esperado**: idioma, nivel de detalle, qué NO incluir.

### 2. `SKILL.md`
Procedimiento accionable para la tarea recurrente principal de este proyecto:
- **Propósito**: una frase de cuándo se debe activar esta skill.
- **Flujo de trabajo**: pasos concretos y en orden que el agente debe seguir
  para generar o corregir código de este tipo (inicialización, lógica principal,
  manejo de estado, control de salidas), basados en lo que el código real hace.
- **Decisiones clave**: elecciones de diseño ya tomadas en el código que deben
  respetarse (ej. método de comparación usado, forma de resolver ambigüedades,
  por qué se eligió tal valor de umbral/tiempo).
- **Criterios de salida**: qué debe incluir siempre la respuesta del agente
  (código completo, explicación breve, comentarios en tal idioma, etc.).
- **Ejemplos de prompts**: 2-3 ejemplos realistas de cómo un usuario pediría
  variaciones de esta misma tarea (cambiar el dispositivo objetivo, el umbral, etc.).
- **Notas adicionales**: aclarar que esta skill es específica de este repo y que
  debe mantenerse sincronizada con `copilot-instructions.md` si hay valores compartidos.

### Reglas estrictas
- Usar **solo** valores, pines, librerías y lógica que existan realmente en el
  código adjunto. No inventar parámetros ni asumir hardware no mencionado.
- Si el propósito general del proyecto no es evidente solo con el código,
  preguntar antes de redactar la sección de "Objetivo".
- Si ya existen versiones previas de estos archivos en el proyecto, comparar
  contra el código actual y señalar explícitamente qué quedó desactualizado
  (por ejemplo, un valor de umbral que cambió en el código pero no en las
  instrucciones) en vez de sobrescribir en silencio.

---

## Notas de uso
- Adjuntar el código fuente relevante junto con este prompt; no hace falta
  compartir el repo completo.
- Si el proyecto tiene varias tareas recurrentes distintas (no solo una), pedir
  un `SKILL.md` por cada una, con nombre descriptivo (ej. `skill-generar-pke.md`,
  `skill-agregar-sensor.md`) en vez de mezclar todo en un solo archivo.
- Guardar este prompt junto al de documentación de hardware (`prompt-docs-hardware.md`)
  en la misma carpeta de plantillas, ya que suelen pedirse juntos al arrancar
  o retomar un proyecto.
