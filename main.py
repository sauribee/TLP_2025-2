# -*- coding: utf-8 -*-
from __future__ import print_function

import sys

from runtime import load_symbols_from_brik, sym_int
from engine import GameEngine
from games.snake_game import SnakeGame
from games.tetris_game import TetrisGame

MAX_WIN_W = 640   # ancho total de la ventana (juego + info)
MAX_WIN_H = 480   # alto total de la ventana


def make_engine_from_symbols(symbols):
    """
    Construye el GameEngine usando los parámetros de 'board' del .brik,
    forzando que la ventana completa sea 640x480.

    El canvas de juego tendrá ancho = board.width * cell_size,
    y el resto hasta 640 px será el panel de información.
    """
    board_w = sym_int(symbols, "board.width", 20)
    board_h = sym_int(symbols, "board.height", 20)
    cell    = sym_int(symbols, "board.cell_size", 20)

    # Cell máximo que permite que el board quepa en 640x480
    max_cell_x = MAX_WIN_W // board_w if board_w > 0 else cell
    max_cell_y = MAX_WIN_H // board_h if board_h > 0 else cell
    max_cell   = min(max_cell_x, max_cell_y)
    if max_cell <= 0:
        max_cell = 1

    # Si el .brik pide un cell muy grande, lo recortamos
    if cell > max_cell:
        cell = max_cell

    # --- Aquí viene la clave ---
    # El canvas de juego ocupa exactamente el ancho del tablero
    game_width_px = board_w * cell

    # El resto hasta 640 px es el panel de info
    info_width_px = max(0, MAX_WIN_W - game_width_px)

    # Alto total fijo 480 (asegúrate de que board_h * cell == 480
    # en tus .brik para no tener franjas negras arriba/abajo).
    height_px = MAX_WIN_H

    engine = GameEngine(
        game_width_px=game_width_px,
        height_px=height_px,
        cell_size=cell,
        tick_ms=50,
        info_width_px=info_width_px,
    )

    return engine



def choose_game():
    """
    Elige juego por argumento de línea de comandos o por input.
    python main.py snake
    python main.py tetris
    """
    if len(sys.argv) >= 2:
        choice = sys.argv[1].strip().lower()
    else:
        print("Selecciona juego: snake / tetris")
        choice = input("> ").strip().lower()

    if choice not in ("snake", "tetris"):
        print("Opción inválida: %s" % choice)
        sys.exit(1)

    return choice


def main():
    choice = choose_game()

    if choice == "snake":
        brik_path = "specs/snake.brik"
        GameClass = SnakeGame
    else:
        brik_path = "specs/tetris.brik"
        GameClass = TetrisGame

    # 1) Cargamos la tabla de símbolos desde el .brik
    symbols = load_symbols_from_brik(brik_path)

    # 2) Creamos el motor a partir de esos símbolos
    engine = make_engine_from_symbols(symbols)

    # 3) Creamos el juego correspondiente
    game = GameClass(engine, symbols)

    # 4) Lo asignamos al engine y arrancamos
    engine.set_game(game)
    engine.start()


if __name__ == "__main__":
    main()
