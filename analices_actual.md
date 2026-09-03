## Ya está incorporado

La versión unificada ya incluye:

- Emisor PIR + timbre en un único firmware.
- Central unificada V3+V4.
- `BOOT_ID` persistente mediante LittleFS.
- Secuencias y ACK.
- Cola de eventos y reintentos `reliable`.
- Deduplicación y protección contra replay.
- Registro de dispositivos y estados `ONLINE/STALE/OFFLINE`.
- HMAC opcional.
- MQTT V4 y compatibilidad con topics V3.
- MQTT Discovery para Home Assistant.
- Modo de alarma `armado/desarmado`.
- Activación temporizada de la bocina.
- OTA base.
- Simulador y pruebas existentes de codec/protocolo.
- V3 conservada como respaldo.

## Lo que falta antes de declararla versión final

### 1. Completar el gate físico

Todavía falta dejar evidencia de:

- Una pulsación de timbre produce un único evento.
- Una activación PIR controlada produce un único evento.
- El PIR vuelve a detectar después de regresar a `LOW`.
- La central recibe los eventos.
- La central activa la bocina correctamente.
- `MOTION` activa la bocina solo cuando el modo está `armado`.
- `TIMBRE` funciona según la regla definida también en modo `desarmado`.
- Se prueban eventos PIR y timbre cercanos o simultáneos.
- Se reinician ambos dispositivos y se observa que el `BOOT_ID` aumenta.
- No se generan efectos duplicados cuando se repite un paquete.

Actualmente sabemos que los dispositivos arrancan y funcionan, pero todavía no está completo el gate de producción.

### 2. Validar MQTT y Home Assistant

Falta comprobar en la instalación real:

- Que MQTT Discovery crea correctamente las entidades.
- Que los mensajes `retained` son correctos.
- Que el estado `online/offline` funciona después de reiniciar la central.
- Que la central sigue funcionando localmente si MQTT no está disponible.
- Que al recuperar MQTT no aparecen eventos duplicados.
- Que no hay otra central V3 conectada al mismo tiempo.
- Que no existen configuraciones Discovery antiguas duplicadas.

La confusión de `Alarma - Forzar Bocina` no impide la migración, pero conviene corregirla después separando:

```text
Alarma - Bocina activa
Alarma - Activar bocina
```

El primer elemento mostraría el estado físico y el segundo sería el control manual.

### 3. Decidir la seguridad de producción

Ahora ambos dispositivos están funcionando con autenticación deshabilitada. Para cerrar la versión hay que elegir explícitamente una de estas opciones:

**Opción recomendada para producción:**

- Activar `AUTH REQUIRED` en emisor y central.
- Verificar una comunicación válida.
- Verificar que un paquete sin HMAC es rechazado.
- Verificar que un paquete con clave incorrecta es rechazado.
- Verificar que un replay no vuelve a activar la bocina.

Si todavía no se quiere activar HMAC, debe quedar documentado como:

```text
Candidata funcional, seguridad HMAC pendiente
```

No debe declararse como versión final segura.

### 4. Revisar OTA

El emisor tiene contraseña OTA configurada, pero la central todavía no tiene una protección equivalente.

Antes de incluir OTA como parte de la versión final hay que:

- Proteger OTA en la central.
- Probar contraseña correcta e incorrecta.
- Verificar que la OTA no interrumpe el control normal.
- Documentar recuperación si una carga falla.

Si no se va a usar OTA todavía, puede quedar fuera del gate de esta migración, pero debe documentarse claramente.

### 5. Corregir y publicar la documentación

Las actas:

```text
docs/PRUEBA_HARDWARE_NODEMCU.md
docs/PRUEBA_HARDWARE_EMISOR.md
```

todavía aparecen sin versionar en el checkout actual. Deben actualizarse con los resultados finales y publicarse.

También falta corregir algunas contradicciones documentales:

- Diferenciar “implementado en código” de “verificado en hardware”.
- Aclarar el formato real de almacenamiento de configuración.
- No afirmar que la central envía configuración por MQTT si ese flujo todavía no está completo.
- Actualizar README, arquitectura, changelog y plan con la ruta final elegida.

### 6. Escoger una única ruta canónica

Actualmente existen varias variantes:

```text
legacy/emisor_pir/
emisor_pir_unificado/

legacy/receptor_bocina/
receptor_central_unificado/
```

La ruta operativa actual es:

```text
emisor_pir_unificado/       ← versión unificada actual
receptor_central_unificado/ ← versión unificada actual

legacy/emisor_pir/          ← respaldo V3
legacy/receptor_bocina/     ← respaldo V3
```

Cuando el gate esté terminado, se puede decidir si se renombran las unificadas a:

```text
emisor_pir/
receptor_central/
```

y se mueve V3 a una carpeta claramente marcada como respaldo, por ejemplo:

```text
legacy/v3/emisor_pir/
legacy/v3/receptor_bocina/
```

La regla importante es:

> En el hardware debe operar únicamente la versión unificada. V3 debe permanecer apagada y solo como recuperación.

## Ideas que no hacen falta para cerrar esta migración

Estas funcionalidades pueden dejarse para una versión posterior:

- Capability Discovery.
- Event Log.
- COMMAND/RESPONSE para relés.
- Zonas avanzadas de alarma.
- Sirena con patrones complejos.
- DHCP o descubrimiento automático.
- AES-GCM.
- Persistencia avanzada de la central.
- Configuración remota MQTT completa.

No incorporarlas ahora no significa que la migración esté incompleta. Son extensiones posteriores.

## Orden recomendado para terminar

1. Completar pruebas controladas de PIR y timbre.
2. Confirmar recepción en la central, bocina y modo armado/desarmado.
3. Verificar MQTT Discovery, retained, LWT y ausencia de una central V3 simultánea.
4. Probar reinicio, `BOOT_ID`, duplicados y reintentos.
5. Decidir si `AUTH REQUIRED` entra en producción.
6. Proteger OTA de la central o declararla fuera del alcance.
7. Actualizar y publicar las actas hardware.
8. Corregir README, arquitectura, changelog y matriz.
9. Marcar V3 como `legacy/respaldo`.
10. Promover `emisor_pir_unificado` y `receptor_central_unificado` como única versión operativa.

**Estado real recomendado ahora:**

```text
V3: respaldo validado
V4 unificada: candidata operativa funcionando
Migración final: pendiente de cerrar pruebas, seguridad, documentación y promoción
```

No hace falta integrar más funcionalidades grandes antes de continuar con el hardware. Primero conviene cerrar este gate; después se puede hacer la limpieza de nombres y dejar una sola versión oficial.
