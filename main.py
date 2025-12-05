# -*- coding: utf-8 -*-

from runtime import load_symbols_from_brik, sym_int
from engine import GameEngine
from games.snake_game import SnakeGame


def make_engine_from_symbols(symbols):
    """
    Construye el GameEngine usando los parámetros de board del .brik.
    """
    width  = sym_int(symbols, 'board.width', 20)
    height = sym_int(symbols, 'board.height', 20)
    cell   = sym_int(symbols, 'board.cell_size', 20)

    # game_width_px y height_px = celdas * tamaño de celda
    game_width_px = width * cell
    height_px     = height * cell

    engine = GameEngine(
        game_width_px=game_width_px,
        height_px=height_px,
        cell_size=cell,
        tick_ms=50,        # cada 50 ms hace update/draw
        info_width_px=360  # panel de info a la derecha
    )
    return engine


def main():
    # 1. Cargar símbolos desde specs/snake.brik
    symbols = load_symbols_from_brik('specs/snake.brik')

    # 2. Crear motor a partir de board.width/height/cell_size
    engine = make_engine_from_symbols(symbols)

    # 3. Crear juego Snake parametrizado por symbols
    game = SnakeGame(engine, symbols)

    # 4. Registrar el juego en el motor y arrancar el loop
    engine.set_game(game)
    engine.start()


if __name__ == '__main__':
    main()
