# Prompt: Generar archivo-mapa.yml

## Objetivo
Analiza el proyecto proporcionado y genera un archivo `archivo-mapa.yml` completo que sirva como mapa técnico exhaustivo del proyecto.

## Qué debe analizar
1. **Estructura de archivos**: Recorrer todos los archivos del proyecto para entender la organización
2. **Código fuente**: Leer los archivos principales para extraer arquitectura, flujos, dependencias
3. **Configuración**: Revisar archivos de build (platformio.ini, package.json, pom.xml, Makefile, etc.)
4. **Dependencias**: Identificar librerías externas y sus versiones
5. **Hardware** (si aplica): Pines, periféricos, protocolos de comunicación

## Estructura obligatoria del YAML generado

```yaml
# Generado por: [herramienta/autor]
# Fecha: [ISO 8601]
# Fuente: análisis del repositorio
---
nombre: [nombre-proyecto]
descripcion: >
  [Descripción concisa de qué hace el proyecto, en 2-3 líneas máximo]
version: "[semver]"
lenguaje_principal: "[lenguaje (framework/plataforma)]"
licencia: "[licencia]"
fecha_ultima_actualizacion: "[YYYY-MM-DD]"

objetivo_principal: >
  [Qué problema resuelve y cuál es el resultado esperado al ejecutarlo]

# Solo incluir si el proyecto involucra hardware
requisitos_hardware:
  microcontrolador_principal: "[modelo]"
  pines_utilizados: [mapeo pin → función]
  perifericos: [lista con modelo, protocolo, dirección]
  fuente_alimentacion: [voltajes y consumos]

# Siempre incluir
dependencias:
  runtime: [lista con nombre, versión, propósito]
  desarrollo: [lista con nombre, versión, propósito]

# Siempre incluir
arquitectura:
  patron: "[MVC, FSM, Event-driven, Monolítico, Microservicios, etc.]"
  componentes:
    - nombre: "[ComponenteX]"
      responsabilidad: "[qué hace]"
      estados: [si aplica FSM]
      archivo_header: "[path]"
      archivo_implementacion: "[path]"

# Siempre incluir
flujos_principales:
  diagrama_conceptual: |
    [Diagrama ASCII del flujo principal: setup/init → loop/main → componentes]
  flujo_detallado_[nombre]:
    inicio: "[estado inicial]"
    pasos: [secuencia numerada]
    duracion_tipica: "[tiempo]"

# Siempre incluir
estructura_archivos:
  [árbol del proyecto con descripción de cada archivo/carpeta relevante]

# Siempre incluir
limitaciones_criticas:
  - id: "[identificador_corto]"
    descripcion: "[qué NO se puede hacer o qué rompe el proyecto]"
    impacto: "[consecuencia si se ignora]"
    solucion: "[cómo evitarlo o resolverlo]"

# Siempre incluir
configuracion:
  archivos_config: [lista de archivos de configuración y qué controlan]
  variables_entorno: [si aplica]
  secretos: [qué archivos contienen credenciales y cómo crearlos]

# Siempre incluir
comandos_utiles:
  compilar: "[comando]"
  ejecutar: "[comando]"
  test: "[comando]"
  debug: "[comando]"
  deploy: "[comando]"
```

## Reglas de generación

1. **Ser exhaustivo pero preciso**: Incluir todo lo que un desarrollador necesita para entender el proyecto sin leer el código
2. **No inventar**: Solo documentar lo que existe en el código. Si algo no aplica, omitir esa sección
3. **Priorizar lo crítico**: Las limitaciones y restricciones que pueden romper el proyecto van primero
4. **Usar valores reales**: Extraer constantes, timeouts, intervalos directamente del código fuente
5. **Formato consistente**: Usar YAML válido, con comentarios separadores entre secciones
6. **Adaptarse al tipo de proyecto**:
   - Embedded/IoT → incluir hardware, pines, consumo, protocolos
   - Web backend → incluir endpoints, DB schema, autenticación
   - Web frontend → incluir rutas, componentes, estado global
   - CLI tool → incluir argumentos, flags, formatos de salida
   - Librería → incluir API pública, tipos exportados, ejemplos de uso

## Criterios de calidad
- Un desarrollador nuevo puede entender el proyecto completo leyendo solo este archivo
- Todas las "trampas" y gotchas están documentadas en limitaciones_criticas
- Los flujos permiten trazar el camino de ejecución sin abrir el código
- Las dependencias incluyen versión exacta y propósito
