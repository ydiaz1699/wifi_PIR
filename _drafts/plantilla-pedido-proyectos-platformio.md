# Plantilla para pedir proyectos de firmware completos

Copiar y adaptar esto al inicio de un chat nuevo con Claude para obtener
código completo, descargable y con la estructura de carpetas clara.

---

## Plantilla de pedido

```
Quiero un proyecto de PlatformIO completo y listo para copiar/pegar.

Entorno: VS Code + PlatformIO, [placa(s) que uses]
Objetivo: [qué debe hacer el firmware]

Necesito:
- Estructura de carpetas con platformio.ini + src/main.cpp por cada subproyecto
- Código completo y funcional (no fragmentos)
- Comprimido en .zip para descargar
- Instrucciones breves de cómo abrirlo y subirlo (build/upload)

Mostrame la estructura así, indicando en qué carpeta hay que parase
para que VS Code/PlatformIO la reconozca:

<nombre_proyecto>/
├── <subproyecto_1>/          ← abrir ESTA carpeta en VS Code
│   ├── platformio.ini
│   └── src/main.cpp
└── <subproyecto_2>/          ← abrir ESTA carpeta en VS Code
    ├── platformio.ini
    └── src/main.cpp
```

---

## Por qué funciona esta fórmula

- **"Código completo, no fragmentos"** → evita que te devuelva solo trozos sueltos de código.
- **"Comprimido en .zip"** → obliga a que se generen los archivos reales y aparezca el botón de descarga, en vez de solo pegar el código en el chat.
- **El diagrama de árbol con la flecha "← abrir ESTA"** → deja explícito en qué carpeta exacta tenés que pararte para que PlatformIO reconozca el `platformio.ini`. Sin esto, es fácil confundirse y abrir la carpeta padre por error.

---

## Contexto de entorno (pegar también al inicio, si aplica)

Como el LLM no tiene memoria entre chats, conviene guardar y reusar tu
"plantilla de hardware" (placas, pines, extensiones de VS Code, convenciones)
como un bloque de texto o archivo aparte, y pegarlo junto con el pedido
de arriba cuando empieces un proyecto nuevo. Esto evita que Claude tenga
que adivinar tu setup o vuelvas a explicarlo desde cero cada vez.

---

## Tips extra

- Si es continuación de un proyecto anterior: subí el archivo de código actual
  o pegalo en el chat — así se parte de lo que ya existe, no de cero.
- Si el proyecto tiene varias placas (ej. emisor + receptor), pedí explícitamente
  "un subproyecto por placa, cada uno con su propio platformio.ini" — así evitás
  que todo quede mezclado en una sola carpeta que PlatformIO no puede procesar.
- Pedí instrucciones de "cómo abrir y subir" solo si las necesitás — si ya sabés
  el flujo, podés omitir esa parte del pedido para una respuesta más corta.
