"""MCP server del banco HIL genérico.

Expone como herramientas MCP (stdio) las acciones de estímulo genéricas sobre
el dispositivo bajo prueba (DUT) —pulsar botones, fijar interruptores— y la
lectura de su monitor serial, de modo que el LLM pueda cerrar el lazo acción ->
reacción sin intervención manual. El mapa etiqueta -> GPIO y la semántica de
cada canal se leen de un ``bench.yaml`` (ver ``config.py``).
"""
