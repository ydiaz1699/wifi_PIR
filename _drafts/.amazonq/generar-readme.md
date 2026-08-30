# Prompt: Generar README.md

## Objetivo
Analiza el proyecto proporcionado y genera un `README.md` completo, profesional y orientado al usuario final que quiere usar/instalar/contribuir al proyecto.

## Qué debe analizar
1. **Propósito del proyecto**: Qué hace, para quién es, qué problema resuelve
2. **Requisitos**: Hardware, software, dependencias, versiones mínimas
3. **Instalación**: Pasos exactos desde clonar hasta ejecutar
4. **Arquitectura**: Visión general (no exhaustiva, solo lo necesario para entender)
5. **Configuración**: Qué se puede personalizar y cómo
6. **Problemas comunes**: Errores frecuentes y sus soluciones

## Estructura obligatoria del README

```markdown
# [Emoji] Nombre del Proyecto

[Descripción en 1-2 líneas: qué hace + tecnología principal + plataforma target]

## 🎯 Características
- ✅ [Feature 1 con detalle técnico breve]
- ✅ [Feature 2]
- ✅ [Feature N]

## 📋 Requisitos

### Hardware (solo si aplica)
| Componente | Especificación |
|-----------|---|
| [nombre] | [modelo/spec] |

### Software
- [Herramienta de build] (versión mínima)
- [Framework/Runtime]
- [Dependencias principales con versión]

## 🚀 Instalación Rápida

### 1. Clonar repositorio
### 2. Configurar [credenciales/entorno/dependencias]
### 3. Compilar/Instalar
### 4. Ejecutar

[Cada paso con comando exacto copiable en bloque de código]

## 📊 Arquitectura
[Diagrama ASCII o descripción breve de componentes principales y su interacción]
[Flujo principal simplificado]

## 📁 Estructura del Proyecto
[Árbol con descripción de cada archivo/carpeta importante]

## ⚙️ Configuración Avanzada
[Opciones configurables: flags, variables, archivos de config]
[Ejemplos de personalización comunes]

## 🐛 Troubleshooting
| Síntoma | Causa | Solución |
|---------|-------|----------|
| [error visible] | [causa raíz] | [comando o acción exacta] |

## 📈 Monitoreo y Diagnóstico (si aplica)
[Cómo ver logs, métricas, estado del sistema]

## 🔧 Problemas Conocidos y Soluciones
[Issues abiertos con workarounds disponibles]

## 💡 Tips de Desarrollo
[Reglas no obvias que evitan bugs: convenciones, gotchas, mejores prácticas]

## 📚 Referencias
[Links a documentación de dependencias, hardware, APIs usadas]

## 📝 Historial de Cambios
### v[X.Y.Z] (YYYY-MM-DD)
- [Cambios principales]

## 📄 Licencia
[Tipo de licencia]

## ✉️ Contacto / Issues
[Cómo reportar bugs con pasos claros]
```

## Reglas de generación

1. **Orientado a la acción**: El usuario debe poder ir de "cloné el repo" a "funciona" siguiendo solo el README
2. **Comandos copiables**: Todo comando en bloques de código con el shell correcto (bash, cmd, powershell)
3. **Tabla de troubleshooting**: Mínimo 5 problemas comunes con solución directa
4. **No redundar con archivo-mapa.yml**: El README es para USAR el proyecto, el mapa es para ENTENDERLO internamente
5. **Emojis en headers**: Mejoran la navegabilidad visual (usar con moderación, solo en H2)
6. **Adaptarse al tipo de proyecto**:
   - Embedded/IoT → incluir diagrama de conexiones, pinout, consumo
   - Web app → incluir screenshots, URLs de demo, variables de entorno
   - CLI → incluir ejemplos de uso con output esperado
   - Librería → incluir API reference resumida con ejemplos de código
   - API → incluir endpoints, autenticación, ejemplos curl
7. **Longitud apropiada**: Suficiente para ser autónomo, no tan largo que nadie lo lea
8. **Secciones opcionales**: Omitir secciones que no apliquen al tipo de proyecto

## Criterios de calidad
- Un usuario nuevo puede instalar y ejecutar el proyecto en menos de 5 minutos leyendo solo el README
- Cada error común tiene una solución concreta (no "revisa la documentación")
- La arquitectura se entiende en 30 segundos con el diagrama
- Los tips de desarrollo previenen los bugs más frecuentes del proyecto
