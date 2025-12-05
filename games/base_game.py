# -*- coding: utf-8 -*-

class BaseGame(object):
    """
    Interfaz común para los juegos que corre el GameEngine.

    El motor asume que cualquier juego tiene estos tres métodos:
      - on_key(keysym): manejar teclas (event.keysym de Tk)
      - update(dt_ms): actualizar lógica en función del tiempo (milisegundos)
      - draw(engine): dibujar en el engine (celdas, texto, etc.)
    """

    def __init__(self, engine, symbols):
        self.engine = engine      # referencia al GameEngine
        self.symbols = symbols    # dict de la tabla de símbolos .brik

    def on_key(self, keysym):
        """Se llama cuando el usuario presiona una tecla."""
        pass

    def update(self, dt_ms):
        """Se llama en cada tick del bucle principal."""
        pass

    def draw(self, engine):
        """Se llama en cada frame para dibujar el juego."""
        pass
