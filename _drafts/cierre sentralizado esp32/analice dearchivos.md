Estos dos archivos son guías para asistentes de código (como yo), no forman parte del firmware del ESP32. Aquí te explico cada uno:



copilot-instructions.md



Es un archivo de instrucciones para que cualquier asistente de código (Copilot, Amazon Q, etc.) entienda el contexto del proyecto.



Define: el objetivo del proyecto (PKE con ESP32 y NimBLE), los datos del dispositivo BLE autorizado (MAC, nombre, UUID, RSSI), las reglas de implementación (librerías, pines, estilo), y el formato de respuesta esperado (en español, conciso).



SKILL.md



Es una definición más detallada del "skill" o capacidad que el asistente debe tener para este proyecto.



Describe paso a paso el flujo de trabajo: cómo detectar la intención del usuario, configurar el proyecto, construir la lógica de escaneo BLE, controlar las salidas (relé y LED), y los criterios de calidad.



Incluye decisiones técnicas clave (cómo comparar direcciones BLE, manejo de UUID, etc.) y ejemplos de prompts que el usuario podría hacer.



En resumen: ambos archivos sirven para que los asistentes de IA generen código correcto y consistente para tu proyecto de cierre centralizado PKE con ESP32, sin necesidad de que repitas la configuración cada vez.

