# -*- coding: utf-8 -*-
from __future__ import print_function

import sys

from runtime import load_symbols_from_brik, sym_int
from engine import GameEngine
from games.snake_game import SnakeGame
from games.tetris_game import TetrisGame

MAX_GAME_W = 640
MAX_GAME_H = 480


def make_engine_from_symbols(symbols):
    """
    Construye el GameEngine usando los parámetros de 'board' del .brik,
    pero forzando que el área de juego (canvas izquierdo) sea 640x480.
    """
    board_w = sym_int(symbols, "board.width", 20)
    board_h = sym_int(symbols, "board.height", 20)
    cell    = sym_int(symbols, "board.cell_size", 20)

    # Tamaño ideal según el .brik
    ideal_w = board_w * cell
    ideal_h = board_h * cell

    # Cell máximo que permite que el board quepa en 640x480
    max_cell_x = MAX_GAME_W // board_w if board_w > 0 else 1
    max_cell_y = MAX_GAME_H // board_h if board_h > 0 else 1

    max_cell = min(max_cell_x, max_cell_y)
    if max_cell <= 0:
        max_cell = 1  # por seguridad

    # Si el tamaño ideal se pasa de 640x480, recortamos cell
    if ideal_w > MAX_GAME_W or ideal_h > MAX_GAME_H:
        cell = max_cell

    # Si el .brik pedía algo aún mayor, también lo recortamos
    if cell > max_cell:
        cell = max_cell

    # El área de juego ES siempre 640x480
    game_width_px = MAX_GAME_W
    height_px     = MAX_GAME_H

    engine = GameEngine(
        game_width_px=game_width_px,
        height_px=height_px,
        cell_size=cell,
        tick_ms=50,
        info_width_px=360,
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
