# Prompt para generar una referencia condensada de un componente

> Plantilla reutilizable para pasar el contrato y comportamiento verificable de un componente a otra LLM sin pegar todo el código fuente.

## Instrucciones para la LLM

Actúa como analista de software y genera un **DOCUMENTO DE REFERENCIA CONDENSADO** del componente indicado abajo. El documento debe permitir que otra persona o LLM razone sobre su contrato, comportamiento observable e interacción con dependencias **sin inventar datos ni depender del código fuente completo**.

La fuente de verdad del comportamiento implementado es el código realmente inspeccionado. La documentación, los tests, los logs y la conversación sirven como evidencia complementaria, pero no convierten por sí solos una propuesta en una funcionalidad implementada.

### Componente a condensar

- **Nombre exacto:** `[nombre del componente, módulo, archivo o sistema]`
- **Tipo:** `[librería | servicio | módulo UI | script | API | clase | sistema | otro]`
- **Alcance explícito:** `[qué debe analizarse y qué queda fuera]`
- **Fuentes proporcionadas:** `[rutas, fragmentos, URLs, conversación o artefactos]`
- **Versión/commit/fecha, si existe:** `[dato o "no confirmado"]`

## Reglas de investigación y evidencia

1. **Inventario antes de resumir.** Identifica los archivos, símbolos, interfaces y dependencias que forman parte del alcance. No afirmes que analizaste un componente completo si solo recibiste un fragmento.
2. **Lectura completa de las fuentes relevantes.** Si un archivo está truncado, falta una dependencia que cambia el contrato o no puede comprobarse una referencia, marca el resultado como `LECTURA INCOMPLETA` y enumera lo que no pudo verificarse. No rellenes el hueco con una suposición.
3. **Cierre de dependencias limitado.** Sigue una dependencia solo si puede cambiar el contrato o el comportamiento observable del componente: tipos públicos, constantes, validadores, configuración, persistencia, transporte, errores, callbacks o formatos. No copies dependencias irrelevantes ni conviertas el documento en una descripción de todo el repositorio.
4. **Precedencia de fuentes:**
   - comportamiento observado en el código inspeccionado;
   - comportamiento confirmado por tests, compilación, logs o ejecución;
   - decisiones explícitas de la conversación o documentación vigente;
   - documentación histórica, planes y propuestas.
   Si dos fuentes contradicen al código, describe la contradicción y marca el dato como `NO CONFIRMADO` o `PROPUESTO`; no lo presentes como hecho implementado.
5. **Trazabilidad compacta.** Para cada dato que pueda cambiar cómo se usa o modifica el componente, conserva una referencia de procedencia como `archivo::símbolo`, ruta de configuración, test o conversación. Usa líneas solo si están disponibles y pueden mantenerse útiles.
6. **Separación de estados.** No mezcles estas categorías:
   - `CONFIRMADO`: observado directamente o verificado por una prueba identificable;
   - `NO CONFIRMADO`: mencionado, inferido parcialmente o imposible de verificar;
   - `PROPUESTO/PENDIENTE`: decisión futura, mejora o cambio aún no implementado;
   - `FUERA DE ALCANCE`: no se analizó deliberadamente.
7. **No inventar.** Si no se conoce un nombre, valor, error, orden, dependencia, garantía o formato, escribe `no confirmado`. No normalices nombres reales a nombres más cómodos.
8. **No confundir ausencia con falsedad.** Si algo no aparece en las fuentes, indica `no observado en las fuentes inspeccionadas`; no afirmes que definitivamente no existe fuera del alcance.
9. **No copiar implementación.** No incluyas cuerpos completos de funciones, clases o scripts. Puedes incluir pseudocódigo mínimo o una regla de orden únicamente cuando sea necesario para expresar un comportamiento observable y no pueda describirse con precisión de otra forma.
10. **No introducir dominio.** Solo crea secciones o datos propios de red, hardware, UI, base de datos, seguridad, etc. cuando existan realmente en el componente o en sus dependencias relevantes.
11. **Prioriza los datos que cambian decisiones.** Conserva nombres exactos, firmas, constantes, límites, valores por defecto, estados, sentinelas, formatos, errores, persistencia, idempotencia, orden de operaciones y condiciones de éxito/fallo. Elimina tutoriales, contexto ornamental y explicaciones repetidas.
12. **Revisión de completitud.** Antes de entregar el documento, comprueba que cada elemento del alcance tenga estado: `CONFIRMADO`, `NO CONFIRMADO`, `PROPUESTO/PENDIENTE` o `FUERA DE ALCANCE`.

## Formato de salida obligatorio

Entrega únicamente el documento Markdown final, usando exactamente esta estructura. No agregues una introducción fuera del documento.

# [Nombre exacto del componente] — Documento de referencia condensado

- **Tipo:** [tipo real]
- **Generado a partir de:** [código fuente, tests, documentación, conversación u otras fuentes realmente utilizadas]
- **Estado de evidencia:** `COMPLETO` o `LECTURA INCOMPLETA`
- **Referencia de versión:** [commit, versión, fecha o `no confirmado`]

## Propósito

Explica en una o dos frases qué es el componente y qué problema resuelve. No describas intenciones no verificadas como si fueran comportamiento implementado.

## Contrato público / interfaz

Incluye solo interfaces utilizables desde fuera del componente, con nombres exactos y sin cuerpos de implementación. Elige lo que corresponda al tipo de componente:

```text
clases, tipos, constantes y enums públicos
funciones y métodos públicos con firma
endpoints, métodos HTTP, parámetros y respuestas
eventos, mensajes, callbacks y formatos
props, slots, hooks o eventos de UI
comandos CLI, argumentos, códigos de salida
esquema de datos, columnas o campos públicos
```

Para cada elemento indica su propósito y, cuando esté confirmado, las condiciones de éxito, error, opcionalidad y compatibilidad.

## Datos concretos

Lista únicamente valores verificables y relevantes para usar correctamente el componente:

- nombres exactos de identificadores, rutas, topics, endpoints, archivos o campos;
- límites de tamaño, rangos, timeouts, versiones y formatos;
- valores por defecto y valores reservados o sentinela;
- estados, códigos, prioridades y flags;
- configuración externa requerida y variables de entorno;
- persistencia, retención, serialización o unidades;
- procedencia compacta de cada dato, cuando sea útil: `(fuente: archivo::símbolo)`.

Si un dato fue propuesto pero no implementado, márcalo explícitamente como `PROPUESTO/PENDIENTE`.

## Comportamiento no obvio / invariantes

Describe solo reglas que cambien el resultado observable o la forma correcta de integración. Incluye, si aplica:

- orden obligatorio de inicialización, validación, persistencia, envío o cierre;
- qué se rechaza, descarta, ignora, registra, retorna o lanza;
- qué es idempotente y qué puede repetirse;
- estados transitorios, degradados, fallos silenciosos y reintentos;
- diferencias entre ausencia, valor cero, valor vacío y valor reservado;
- reglas de sesión, deduplicación, concurrencia, caché o expiración;
- compatibilidad hacia atrás y comportamiento de entradas antiguas;
- garantías que **no** existen.

No describas algoritmos internos salvo que conocerlos sea necesario para predecir una de esas reglas.

## Dependencias

Separa:

- **Dependencias directas:** componentes, librerías, servicios, archivos de configuración o recursos llamados/importados directamente.
- **Supuestos externos relevantes:** qué debe existir o estar configurado fuera del componente.
- **Dependencias no confirmadas:** referencias mencionadas pero no inspeccionadas.

No declares una librería, servicio o recurso como dependencia si solo aparece en documentación no verificada y no afecta al contrato observado.

## Estado y decisiones (si aplica)

- **Implementado y confirmado:** [cambios o decisiones que el código y/o una prueba confirma]
- **Implementado pero no verificado completamente:** [por ejemplo, no compilado en el entorno real]
- **Propuesto/pendiente:** [mejoras, bugs o decisiones futuras discutidas pero no implementadas]
- **Descartado o fuera de alcance:** [solo si existe una decisión explícita]

Si no existe conversación previa relevante, escribe `No se proporcionó historial de decisiones aplicable.` No inventes un roadmap.

## Matriz mínima de trazabilidad

Incluye una tabla breve para los datos o decisiones que podrían inducir a error en una sesión nueva:

| ID | Afirmación condensada | Fuente/evidencia | Estado |
|---|---|---|---|
| REF-001 | [dato verificable] | `[archivo::símbolo]`, test o conversación | `CONFIRMADO` / `NO CONFIRMADO` / `PROPUESTO/PENDIENTE` |

No repitas en la tabla todo el documento; úsala para cubrir decisiones críticas, límites, invariantes y contradicciones.

## Fuera de alcance de este documento

Enumera explícitamente:

- archivos, dependencias o rutas no inspeccionados;
- comportamiento que requiere ejecución real, hardware, red, broker, navegador o despliegue y no fue verificado;
- aspectos que el componente no expone o que no pueden deducirse de las fuentes;
- cualquier parte omitida para mantener el documento condensado.

## Control final antes de entregar

Verifica internamente que:

- no haya identificadores inventados ni nombres normalizados;
- todas las afirmaciones críticas tengan estado y, cuando sea posible, procedencia;
- la interfaz no contenga cuerpos de implementación;
- los valores literales estén copiados exactamente y con unidades;
- las propuestas no aparezcan mezcladas con funcionalidades implementadas;
- las limitaciones y la lectura incompleta estén declaradas;
- el resultado sea autosuficiente para razonar, pero no pretenda sustituir al código para cambios de implementación.
