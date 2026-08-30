Este proyecto trae una tercera categoría distinta a las dos anteriores: el archivo **`archivo-mapa.yml`**.

## Cómo se llama esto

Por el propio encabezado del archivo (`Generado por: Archivo-Mapa (Claude)`), parece que ya tenés un flujo/skill propio llamado **"Archivo-Mapa"**. En términos generales, esto se conoce como:

- **"Repo map"** o **"codebase map"** — término que usan herramientas como Aider para describir un resumen estructurado de todo el repositorio pensado para dárselo a un LLM como contexto.
- **"Project context manifest"** o **"codebase digest"** — cuando, como en tu caso, no es solo un mapa de archivos sino que incluye arquitectura, flujos, problemas conocidos, métricas y guía de setup todo en un solo YAML.

La diferencia con lo que ya tenés catalogado:

| Archivo | Categoría | Alcance |
|---|---|---|
| `notas.md` + `conexiones.drawio.svg` | Hardware docs | Conexionado físico/pinout |
| `copilot-instructions.md` + `SKILL.md` | AI agent context files | Reglas y procedimientos para que el agente trabaje bien |
| **`archivo-mapa.yml`** | **Repo map / Codebase manifest** | **Panorama completo del proyecto**: arquitectura, flujos de estado, limitaciones críticas, problemas conocidos, métricas, guía de setup y changelog — todo en un único documento estructurado |

Es, en esencia, el más "denso" de los tres: combina lo que normalmente estaría repartido entre un README técnico, un ADR (architecture decision record) y un changelog, pero en formato YAML pensado para que un LLM lo parsee fácilmente en una sola pasada.

`README.md` no es nuevo — es documentación estándar para humanos, no es específico de "contexto para IA".

## El meta-prompt para esta categoríaCon esto ya tenés la colección de tres meta-prompts complementarios:

1. **`prompt-docs-hardware.md`** → `notas.md` + `conexiones.drawio.svg` (pinout/wiring, para humanos que cablean)
2. **`prompt-contexto-agentes-ia.md`** → `copilot-instructions.md` + `SKILL.md` (reglas y procedimientos para que el agente responda bien)
3. **`prompt-repo-map.md`** → `archivo-mapa.yml` (panorama completo del proyecto: arquitectura, flujos, problemas conocidos, setup)

Cada uno cubre una capa distinta de documentación orientada a IA, del más específico (pines) al más general (todo el proyecto).