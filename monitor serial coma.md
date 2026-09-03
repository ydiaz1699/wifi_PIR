Sí. Puedes hacer que todo lo que imprime el monitor de PlatformIO se guarde en un archivo de texto usando la redirección de PowerShell o CMD.

En PowerShell
python -m platformio device monitor --port COM14 --baud 115200 | Tee-Object -FilePath salida.txt


Esto tiene una ventaja: ves los datos en la terminal y simultáneamente se guardan en salida.txt.

Si quieres que se vaya agregando al archivo sin borrar lo anterior:

python -m platformio device monitor --port COM14 --baud 115200 | Tee-Object -FilePath salida.txt -Append

En CMD

Si no necesitas verlo en pantalla y solo quieres guardarlo:

python -m platformio device monitor --port COM14 --baud 115200 > salida.txt


Para verlo y guardarlo simultáneamente desde CMD puedes usar:

python -m platformio device monitor --port COM14 --baud 115200 | powershell -Command "$input | Tee-Object -FilePath salida.txt"

⚠️ Un detalle importante

El monitor serie normalmente está ejecutándose continuamente, así que salida.txt se irá llenando mientras el dispositivo imprime. Para terminar el monitor puedes usar:

Ctrl + C


Yo usaría Tee-Object, porque así no pierdes la visualización de los mensajes mientras haces el registro:

python -m platformio device monitor --port COM14 --baud 115200 | Tee-Object -FilePath salida.txt


Si quieres, también puedo mostrarte cómo hacer que PlatformIO guarde automáticamente cada sesión con un nombre que incluya fecha y hora, por ejemplo log_2026-09-03_14-15.txt.