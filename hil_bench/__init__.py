"""Banco de pruebas HIL genérico: las "manos y oídos" del LLM para cualquier DUT.

Esta herramienta le da al LLM (en Kiro) "manos" para accionar las entradas
físicas de un dispositivo bajo prueba (DUT) a través de optoacopladores
controlados por una Raspberry Pi Pico (MicroPython), y "oídos" para leer el
monitor serial del DUT, todo vía un MCP server en Python (el "sistema
nervioso").

El core es GENÉRICO: razona en primitivas de actuador (``boton``,
``interruptor``), no en conceptos de dominio. Cada banco concreto se describe de
forma declarativa en un ``bench.yaml`` (canales con etiqueta, tipo, GPIO,
polaridad...). wifi_PIR es solo un ejemplo: ver ``examples/wifi_pir/bench.yaml``.

Consulta ``docs/DISENO_BANCO_HIL.md`` para la arquitectura, el cableado y el
protocolo serial completos, y ``README.md`` para el uso.
"""
