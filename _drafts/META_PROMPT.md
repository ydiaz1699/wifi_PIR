# Meta-Prompt: Documentación Completa de Proyecto

## Instrucciones

Copiar el siguiente prompt y pegarlo en un chat nuevo con cualquier LLM cuando quieras documentar un proyecto después de trabajar en él. Reemplazar `[PROYECTO]` con el nombre/descripción de tu proyecto.

---

## El Prompt (copiar desde aquí)

```
Analiza TODO este chat completo y genera la documentación del proyecto [PROYECTO]. 

Necesito que crees 4 archivos en la carpeta `docs/` del proyecto:

---

### 1. `docs/ARCHITECTURE.md` — Cómo funciona el sistema

Incluir:
- Diagrama general del sistema (ASCII art)
- Descripción de cada componente (qué hace, cómo se comunica)
- Flujos de datos completos (paso a paso con flechas)
- Hardware utilizado (pines, boards, conexiones)
- Protocolos de comunicación (formato de mensajes, puertos, etc.)
- Diseño del software (loop principal, prioridades, módulos)
- Principios de diseño (reglas que el sistema sigue)
- Estructura de archivos del proyecto

Formato: técnico pero legible. Que alguien que nunca vio el proyecto pueda entender todo leyendo solo este archivo.

---

### 2. `docs/CHANGELOG.md` — Historial de versiones

Incluir:
- Cada versión que se trabajó en este chat
- Para cada versión: qué se agregó, qué se cambió, qué se arregló
- Orden cronológico (la primera versión arriba, la última abajo)
- Nombres concretos de archivos/funciones que cambiaron

Formato: conciso, tipo bullet points. Sin explicaciones largas.

---

### 3. `docs/BUGS_FIXED.md` — Errores resueltos

Para CADA bug que se resolvió en este chat, documentar:
- **Síntoma**: qué se observaba (el error visible)
- **Causa raíz**: por qué pasaba realmente (el problema de fondo)
- **Solución**: qué se hizo para arreglarlo
- **Regla**: una frase corta que resuma cómo evitar este error en el futuro

Propósito: que en el futuro, ni yo ni ningún LLM vuelva a introducir estos mismos errores.

---

### 4. `docs/ROADMAP.md` — Mejoras futuras

Este archivo es CRÍTICO. Debe estar escrito para que un LLM SIN CONTEXTO ni conocimiento del proyecto pueda implementar cada mejora. Para cada mejora pendiente incluir:

- **Qué hacer**: descripción clara del objetivo
- **Por qué**: qué problema resuelve o qué valor agrega
- **Implementación**: pasos concretos numerados
  - Archivos a crear o modificar (paths exactos)
  - Código ejemplo si aplica
  - Dependencias necesarias
- **Cómo verificar**: qué test/prueba confirma que funciona
- **Prioridad**: alta/media/baja

Además, incluir una sección al inicio:

```
## Instrucciones para LLM

CONTEXTO: [descripción de 2-3 líneas de qué es el proyecto]

REGLAS CRÍTICAS:
1. [regla 1 — lo más importante que NO debe hacer]
2. [regla 2]
3. [etc.]

ARCHIVOS CLAVE:
- [path] — [qué hace]
- [path] — [qué hace]

ERRORES COMUNES A EVITAR:
- [error 1 y por qué pasa]
- [error 2]
```

---

### 5. Actualizar `README.md`

- Agregar links a los 4 documentos
- Quick start (clonar, configurar, compilar, probar)
- Tabla de hardware
- Diagrama de arquitectura simplificado

---

### Reglas generales para toda la documentación:

1. Escribir como si el lector NO tiene acceso a este chat
2. No asumir conocimiento previo del proyecto
3. Ser específico: paths de archivos, nombres de funciones, valores concretos
4. Usar diagramas ASCII cuando ayuden a entender
5. Preferir ejemplos concretos sobre descripciones abstractas
6. Las instrucciones del ROADMAP deben ser tan detalladas que un LLM pueda implementarlas SIN preguntar
7. Los bugs deben incluir la REGLA para evitarlos (no solo la solución)

---

### Después de crear los archivos:

- Hacer commit con mensaje descriptivo
- Push a GitHub
- Mostrarme los links a los archivos creados
```

---

## Variante corta (si el chat fue breve)

```
Analiza este chat y documenta el proyecto en `docs/`:
1. ARCHITECTURE.md — cómo funciona el sistema (diagrama + flujos + archivos)
2. CHANGELOG.md — qué versiones se hicieron y qué cambió en cada una  
3. BUGS_FIXED.md — cada bug: síntoma, causa, solución, regla para no repetir
4. ROADMAP.md — mejoras pendientes con instrucciones paso a paso para un LLM sin contexto

Reglas: escribir para alguien que nunca vio el proyecto. Ser específico (paths, funciones, valores). El ROADMAP debe ser ejecutable sin preguntar.

Commit + push a GitHub.
```

---

## Variante para proyecto existente que ya tiene docs

```
Lee los archivos en `docs/` (ARCHITECTURE.md, CHANGELOG.md, BUGS_FIXED.md, ROADMAP.md) y ACTUALÍZALOS con todo lo que hicimos en este chat:
- Agregar las nuevas versiones al CHANGELOG
- Agregar los bugs nuevos al BUGS_FIXED
- Actualizar ARCHITECTURE si cambió el diseño
- Mover items completados del ROADMAP a CHANGELOG, agregar nuevos pendientes

Commit + push.
```

---

## Tips

- Usar este prompt **al final** de una sesión productiva (no al principio)
- Si el chat fue muy largo, el LLM puede perder detalle → pedirle que revise sección por sección
- Si un LLM dice "no tengo acceso al chat anterior" → darle el ROADMAP.md y decirle "implementá la tarea N"
- El ROADMAP es el documento más valioso — es la "memoria" del proyecto entre sesiones
