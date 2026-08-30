# Cómo usar esta plantilla

Cuando empieces un proyecto nuevo (misma placa que ya existe, o placa nueva):

```bash
cp -r _template-proyecto proyecto-<nombre-descriptivo>
cd proyecto-<nombre-descriptivo>
```

1. Si la placa **no existe** todavía en `../boards/`, créala primero
   (ver `../boards/_template-board.md`).
2. Reemplaza `<nombre-proyecto>` y `<placa>` en todos los archivos de `.ai/`.
   (búsqueda y reemplazo global funciona bien acá, son placeholders literales)
3. Llena `.ai/HARDWARE.md` con el wiring — **no copies pines genéricos**, esos
   ya están en `../boards/<placa>.md`.
4. Agrega la ruta de este proyecto en la sección "Proyectos que usan esta
   placa" del archivo correspondiente en `../boards/`.
5. Crea `src/`, `platformio.ini` real (basado en el bloque `[env:]` de la
   placa) y `.vscode/` (copia de `../shared/SOFTWARE.md`).

## Por qué esto reduce trabajo repetido

`TASKS.md`, `CHANGELOG.md`, `DECISIONS.md`, `ROADMAP.md` y la estructura de
`SKILL.md`/`SOFTWARE.md` son siempre iguales en forma — solo cambia el
contenido. Copiar la plantilla te da la forma gratis; tú solo llenas lo que
es genuinamente distinto de este proyecto.

`HARDWARE.md` deja de ser el archivo más largo del proyecto (antes tenía toda
la ficha técnica de la placa) y pasa a ser corto: dos tablas — pines que usa
este proyecto y periféricos conectados. La ficha técnica completa vive una
sola vez en `boards/`.
