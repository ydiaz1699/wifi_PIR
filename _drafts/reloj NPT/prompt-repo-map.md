# Prompt reutilizable: generar un "repo map" / codebase manifest (archivo-mapa.yml)
# a partir del código actual

Usar este prompt junto con el código fuente completo del proyecto (o el repo
empaquetado, ej. vía Repomix) cuando se quiera un documento único, estructurado
y denso que le dé a cualquier LLM el panorama completo del proyecto en una sola
lectura, sin tener que explorar archivo por archivo.

A diferencia de `copilot-instructions.md` (reglas de comportamiento) y `SKILL.md`
(procedimiento para una tarea puntual), este archivo describe **el proyecto en sí**:
qué es, cómo está armado, qué falla y qué se sabe sobre su estado actual.

---

## Prompt

Antes de escribir una sola línea del archivo de salida, leé **el código fuente
completo** que te adjunto — todos los archivos del repositorio, o el equivalente
empaquetado (ej. un dump de Repomix) — de principio a fin, incluyendo headers,
implementaciones, archivos de configuración y comentarios. No te bases en los
nombres de archivo ni en suposiciones sobre "lo típico" para este tipo de proyecto.

Reglas de lectura obligatorias:
- Recorré **cada archivo** del repositorio, no solo `main.cpp`/`main.ino` o el
  punto de entrada. Un dato relevante (un pin, una constante, un timeout) puede
  estar en un header que nunca se abre a simple vista.
- No te detengas en el primer archivo que "parece" tener toda la info; los
  detalles suelen estar repartidos entre varios archivos (constantes en un
  `.h`, lógica en un `.cpp`, configuración en `platformio.ini`/`package.json`/`.env`).
- Si el código está dividido en módulos (headers + implementaciones separadas,
  como `Clase.h` + `Clase.cpp`), leé ambos antes de describir esa clase/módulo:
  el header solo te da la interfaz, no el comportamiento real.
- Prestá atención a comentarios de advertencia, `static_assert`, `TODO`, `FIXME`,
  y a la ausencia de patrones esperados (ej. si no hay watchdog, si no hay manejo
  de un caso de error) — eso también es información válida para el mapa.
- Si el repositorio incluye un README, changelog, o un `archivo-mapa.yml` previo,
  leerlos también, pero usarlos solo como referencia a contrastar contra el
  código real — nunca como sustituto de leer el código.

Solo después de haber leído todo el código —y **basándote exclusivamente en
lo que encontraste ahí**—, genera un archivo `archivo-mapa.yml` (o `repo-map.yml`)
con esta estructura:

### Metadata general
- nombre, descripción breve, versión (si existe), lenguaje principal,
  fecha de última actualización (si es inferible de comentarios/changelog).

### Objetivo principal
- 2-4 líneas describiendo qué hace el sistema, inferido del código real
  (no del nombre de la carpeta ni de suposiciones).

### Requisitos de hardware/entorno (si aplica)
- Microcontrolador/plataforma objetivo, pines usados, periféricos,
  requisitos de alimentación — todo extraído de constantes y `#include`/`platformio.ini`.
- Si el proyecto no es de hardware (ej. backend, script), omitir esta sección
  o adaptarla a requisitos de entorno de ejecución (runtime, SO, dependencias del sistema).

### Flujos principales
- Diagrama conceptual en texto de `setup()`/inicialización y del bucle o
  flujo principal, paso a paso, basado en el orden real de llamadas del código.
- Máquinas de estado presentes en el código (nombre del componente, estados
  posibles, transiciones, timeouts/intervalos con sus valores reales).

### Limitaciones y restricciones críticas
- Cosas que, si se rompen, generan fallos graves (ej. constantes que no deben
  cambiarse sin ajustar otra parte, límites de memoria/flash, requisitos de
  compilador/estándar de lenguaje) — identificadas por comentarios, asserts,
  o restricciones evidentes en el propio código.

### Problemas conocidos / áreas frágiles
- Extraídos de TODOs, comentarios de advertencia, ausencia de manejo de errores
  evidente (ej. falta de timeout, falta de validación) — con severidad estimada
  y solución sugerida si es clara. No inventar problemas que no se puedan
  justificar con algo presente en el código.

### Estructura del proyecto
- Listado de carpetas/archivos relevantes con una descripción de una línea
  de la responsabilidad de cada uno.

### Dependencias clave
- Librerías externas con su versión (de `platformio.ini`, `package.json`,
  `requirements.txt`, etc., según el ecosistema).

### Guía rápida de setup
- Pasos mínimos para clonar, configurar credenciales/config si aplica,
  compilar/instalar y ejecutar — basados en archivos de config reales
  (ej. plantillas de secrets, `.env.example`).

### Comandos importantes
- Build, upload/deploy, test, monitor/logs — los que correspondan al
  ecosistema detectado (PlatformIO, npm, make, etc.).

### Changelog (si hay evidencia)
- Solo si hay control de versiones, comentarios de versión, o un
  changelog previo del que partir; si no hay evidencia, omitir en vez de inventar.

### Referencias
- Enlaces a librerías/frameworks usados, si son identificables por nombre exacto.

### Notas finales
- Recordatorio de que este archivo es contexto para LLMs y debe actualizarse
  ante cambios grandes de arquitectura.

### Reglas estrictas
- Usar **solo** información verificable en el código/config adjuntos.
  No inventar métricas de rendimiento, changelog, ni problemas sin evidencia.
- Cada afirmación del mapa debe poder rastrearse a un archivo y una sección
  concreta del código leído. Si no podés señalar de dónde sale un dato, no lo incluyas.
- No resumir "por conocimiento general del framework" lo que el código hace distinto
  de lo típico — si el código se aparta del patrón esperado, documentar lo que
  realmente hace, no lo que "debería" hacer.
- Cuando algo sea una estimación (ej. "tiempo típico de conexión"), marcarlo
  explícitamente como estimado, no como medición real.
- Si ya existe un `archivo-mapa.yml` previo, comparar contra el código actual
  y señalar qué secciones quedaron desactualizadas en vez de regenerar todo
  desde cero sin contraste.
- Si el propósito general no es evidente solo con el código, preguntar antes
  de redactar "Objetivo principal".
- Si el repositorio es grande, no reducir la lectura para ahorrar tiempo:
  es preferible tardar más y cubrir todos los archivos que entregar un mapa
  incompleto basado en una lectura parcial.

---

## Notas de uso
- Este archivo conviene generarlo con el **código completo** del repo (a diferencia
  de los otros dos prompts, que pueden funcionar con solo el/los archivo(s) principal(es)),
  porque su valor está en dar panorama completo, no en un componente puntual.
- Es el más costoso de generar bien — conviene regenerarlo solo tras cambios
  grandes de arquitectura, no en cada sesión.
- Guardar junto con `prompt-docs-hardware.md` y `prompt-contexto-agentes-ia.md`
  en la misma carpeta de plantillas.
