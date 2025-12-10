# -*- coding: utf-8 -*-
"""
==========================================
TETRIS GAME - Brick Game Engine
==========================================

Implementación completa del juego Tetris con:
- 7 piezas clásicas + pieza bomba especial
- Sistema de rotación con 4 estados por pieza
- Preview de próximas piezas (next queue)
- Ghost piece (proyección de caída)
- Sistema de combos y multiplicadores
- Aumento progresivo de velocidad
- Line clearing con puntuación avanzada

TABLA DE CONTENIDOS:
    1. Constantes y shapes de piezas (líneas 1-100)
        - TETROMINO_SHAPES: Definiciones de todas las piezas
    2. TetrisGame.__init__ (líneas 100-200)
        - Lectura de configuración del .brik
        - Colores y controles
        - Parámetros del juego
        - Estado inicial
    3. Gestión de piezas (líneas 200-350)
        - load_pieces_from_brik() - Carga desde .brik
        - spawn_piece() - Genera nueva pieza
        - get_next_piece_type() - Sistema de cola
    4. Lógica de movimiento (líneas 350-450)
        - rotate_piece() - Rotación con wall kicks
        - move_piece() - Movimiento lateral
        - apply_gravity() - Caída automática
        - drop_piece() - Drop instantáneo
    5. Colisiones y validación (líneas 450-500)
        - is_valid_position() - Detecta colisiones
        - lock_piece() - Fija pieza al tablero
    6. Sistema de líneas (líneas 500-550)
        - check_and_clear_lines() - Detección
        - calculate_score() - Puntuación con combos
    7. Sistema de bomba (líneas 550-600)
        - explode_bomb() - Explosión y blast radius
        - update_bomb_timers() - Contador de detonación
    8. Input y actualización (líneas 600-650)
        - on_key() - Manejo de teclas
        - update() - Game loop principal
    9. Rendering (líneas 650-668)
        - draw() - Dibuja todo el juego
        - draw_ghost_piece() - Proyección
        - draw_next_queue() - Preview piezas
        - draw_ui() - Panel de información

==========================================
"""
from __future__ import print_function

import random

from games.base_game import BaseGame
from runtime import sym_int, sym_str, sym_bool, sym_float, sym_get


# Definición de tetrominós (Tetris "clásico" simplificado).
# Cada pieza es una lista de 4 orientaciones; cada orientación es
# una lista de (x, y) relativos a un origen.
TETROMINO_SHAPES = {
    "I": [
        [(0, 1), (1, 1), (2, 1), (3, 1)],
        [(2, 0), (2, 1), (2, 2), (2, 3)],
        [(0, 2), (1, 2), (2, 2), (3, 2)],
        [(1, 0), (1, 1), (1, 2), (1, 3)],
    ],
    "O": [
        [(1, 0), (2, 0), (1, 1), (2, 1)],
        [(1, 0), (2, 0), (1, 1), (2, 1)],
        [(1, 0), (2, 0), (1, 1), (2, 1)],
        [(1, 0), (2, 0), (1, 1), (2, 1)],
    ],
    "T": [
        [(1, 0), (0, 1), (1, 1), (2, 1)],
        [(1, 0), (1, 1), (2, 1), (1, 2)],
        [(0, 1), (1, 1), (2, 1), (1, 2)],
        [(1, 0), (0, 1), (1, 1), (1, 2)],
    ],
    "S": [
        [(1, 0), (2, 0), (0, 1), (1, 1)],
        [(1, 0), (1, 1), (2, 1), (2, 2)],
        [(1, 1), (2, 1), (0, 2), (1, 2)],
        [(0, 0), (0, 1), (1, 1), (1, 2)],
    ],
    "Z": [
        [(0, 0), (1, 0), (1, 1), (2, 1)],
        [(2, 0), (1, 1), (2, 1), (1, 2)],
        [(0, 1), (1, 1), (1, 2), (2, 2)],
        [(1, 0), (0, 1), (1, 1), (0, 2)],
    ],
    "J": [
        [(0, 0), (0, 1), (1, 1), (2, 1)],
        [(1, 0), (2, 0), (1, 1), (1, 2)],
        [(0, 1), (1, 1), (2, 1), (2, 2)],
        [(1, 0), (1, 1), (0, 2), (1, 2)],
    ],
    "L": [
        [(2, 0), (0, 1), (1, 1), (2, 1)],
        [(1, 0), (1, 1), (1, 2), (2, 2)],
        [(0, 1), (1, 1), (2, 1), (0, 2)],
        [(0, 0), (1, 0), (1, 1), (1, 2)],
    ],
}

DEFAULT_COLORS = {
    "I": "#00ffff",
    "O": "#ffff00",
    "T": "#aa00ff",
    "S": "#00ff00",
    "Z": "#ff0000",
    "J": "#0000ff",
    "L": "#ff8800",
}

class Piece(object):
    def __init__(self, kind, x, y, rotation):
        self.kind = kind
        self.x = x
        self.y = y
        self.rotation = rotation  # 0..3

class TetrisGame(BaseGame):
    def __init__(self, engine, symbols):
        super(TetrisGame, self).__init__(engine, symbols)

        # Dimensiones de tablero
        self.board_w = sym_int(symbols, "board.width", engine.grid_width)
        self.board_h = sym_int(symbols, "board.height", engine.grid_height)
        # Color de la sombra (global para todas las piezas)
        self.ghost_color = sym_str(symbols, "board.colors.ghost","#666666")
        
        # Colores del tablero
        self.color_bg   = sym_str(symbols, "board.colors.background", "#000000")
        self.color_grid = sym_str(symbols, "board.colors.grid",       "#000000")

        # Color de las paredes (borde). Si no hay walls en el .brik, usamos grid.
        # self.wall_color = sym_str(symbols, "board.colors.walls", self.color_grid)
        self.wall_color = sym_str(symbols, "board.colors.walls", "#self.color_grid")

        # ---------------- Cola de próximas piezas ----------------
        self.next_queue_length = sym_int(symbols, "ui.next_queue_length", 3)
        if self.next_queue_length < 1:
            self.next_queue_length = 1

        # se llenará en _init_pieces()
        self.next_queue = []

        # Controles
        self.key_left    = sym_str(symbols, "controls.left_mov",   "Left").lower()
        self.key_right   = sym_str(symbols, "controls.right_mov",  "Right").lower()
        self.key_down    = sym_str(symbols, "controls.down_mov",   "Down").lower()
        self.key_rotate  = sym_str(symbols, "controls.rotate",     "Up").lower()
        self.key_drop    = sym_str(symbols, "controls.drop",       "space").lower()
        self.key_pause   = sym_str(symbols, "controls.pause",      "p").lower()
        self.key_restart = sym_str(symbols, "controls.restart",    "r").lower()

        # Tiempo de caída y progresión de velocidad
        self.base_tick_ms = sym_int(symbols, "tetris.tick_ms", 500)
        self.min_tick_ms  = sym_int(symbols, "tetris.min_tick_ms", 80)
        self.tick_ms      = self.base_tick_ms

        # Scoring y progreso de nivel
        self.score_per_line   = sym_int(symbols, "rules_scoring.per_line",   100)
        self.score_per_tetris = sym_int(symbols, "rules_scoring.per_tetris", 800)
        self.lines_per_level  = sym_int(symbols, "rules_speed_progression.lines_per_level", 10)
        self.tick_delta_per_level = sym_int(symbols, "rules_speed_progression.delta_ms", 50)

        # Cargar formas y colores de pieces.piece_X.* en el .brik
        self.piece_shapes, self.piece_colors = self._load_pieces_from_symbols(symbols)
        
        # Separar piezas normales de bombas
        self.normal_pieces = []
        self.bomb_pieces = []
        for kind in sorted(self.piece_shapes.keys()):
            if 'bomb' in kind.lower():
                self.bomb_pieces.append(kind)
            else:
                self.normal_pieces.append(kind)
        
        self.piece_kinds = sorted(self.piece_shapes.keys())
        
        # Configuración de bombas aleatorias
        self.random_bombs = sym_bool(symbols, "rules_random_pieces.random_bombs", False)
        self.bomb_chance = sym_float(symbols, "rules_random_pieces.bomb_chance", 0.05)

        # Estado del juego
        self.board = self._make_empty_board()
        self.current_piece      = None
        # self.next_piece_kind    = None
        self.score              = 0
        self.level              = 1
        self.total_lines_cleared = 0
        self.game_over          = False
        self.paused             = False

        self.accum_ms = 0

        self._rng = random.Random()
        self._init_pieces()   # <- aquí ya base_tick_ms está definida

    # ------------------------------------------------------------------
    # Helpers de inicialización
    # ------------------------------------------------------------------

    def _make_empty_board(self):
        # Empezamos todo vacío
        board = [[None for _ in range(self.board_w)] for _ in range(self.board_h)]

        # Paredes verticales: primera (x=0) y última columna (x=board_w-1)
        for y in range(self.board_h):
            board[y][0] = self.wall_color
            board[y][self.board_w - 1] = self.wall_color

        # Paredes horizontales: primera fila (y=0) y última fila (y=board_h-1)
        for x in range(self.board_w):
            board[0][x] = self.wall_color
            board[self.board_h - 1][x] = self.wall_color
        return board

    def _init_pieces(self):
        self.board = self._make_empty_board()
        self.current_piece       = None
        self.score               = 0
        self.level               = 1
        self.total_lines_cleared = 0
        self.game_over           = False
        self.paused              = False
        self.accum_ms            = 0

        self.tick_ms = self.base_tick_ms

        # rellenar cola y spawnear
        self._refill_next_queue(full=True)
        self._spawn_new_piece()
    
    def _load_pieces_from_symbols(self, symbols):
        """
        Construye:
          - self.piece_shapes[kind] = [rot0_cells, rot1_cells, ...]
          - self.piece_colors[kind] = "#rrggbb"
        tomando la info de pieces.piece_X en la tabla de símbolos.

        Si alguna pieza no está definida en el .brik, usa las formas
        por defecto (TETROMINO_SHAPES / DEFAULT_COLORS).
        """
        piece_shapes = {}
        piece_colors = {}

        # Sufijos estándar: I, O, T, S, Z, J, L
        standard_pieces = ["I", "O", "T", "S", "Z", "J", "L"]
        
        for suffix in standard_pieces:
            base = "pieces.piece_%s" % suffix

            # Color: pieces.piece_I.color
            color_key = base + ".color"
            color = sym_str(symbols, color_key, None)
            if color is None:
                # fallback si el .brik no define color
                if 'DEFAULT_COLORS' in globals() and suffix in DEFAULT_COLORS:
                    color = DEFAULT_COLORS[suffix]
                else:
                    color = "#ffffff"

            # Rotations: lista de matrices 4x4, si están en el .brik
            rot_key = base + ".rotations"
            mats = sym_get(symbols, rot_key, None)

            rotations_cells = []

            if mats is not None:
                # mats es algo como: [mat0, mat1, mat2, mat3]
                for mat in mats:
                    cells = []
                    for y, row in enumerate(mat):
                        for x, val in enumerate(row):
                            if val:
                                # cualquier valor distinto de 0 lo consideramos bloque
                                cells.append((x, y))
                    rotations_cells.append(cells)

            # Si no hay mats en el .brik, hacemos fallback a TETROMINO_SHAPES
            if not rotations_cells:
                if 'TETROMINO_SHAPES' in globals() and suffix in TETROMINO_SHAPES:
                    rotations_cells = TETROMINO_SHAPES[suffix]
                else:
                    # como último recurso, una pieza de 1 bloque
                    rotations_cells = [[(0, 0)]]

            piece_shapes[suffix] = rotations_cells
            piece_colors[suffix] = color
        
        # Cargar piezas de bomba (bomb_1x1, bomb_2x2, etc.)
        bomb_suffixes = ["bomb_1x1", "bomb_2x2"]
        for suffix in bomb_suffixes:
            base = "pieces.piece_%s" % suffix
            
            # Verificar si existe en símbolos
            color_key = base + ".color"
            color = sym_str(symbols, color_key, None)
            
            if color is not None:  # Solo cargar si está definida
                rot_key = base + ".rotations"
                mats = sym_get(symbols, rot_key, None)
                
                rotations_cells = []
                if mats is not None:
                    for mat in mats:
                        cells = []
                        for y, row in enumerate(mat):
                            for x, val in enumerate(row):
                                if val:
                                    cells.append((x, y))
                        rotations_cells.append(cells)
                
                if rotations_cells:
                    piece_shapes[suffix] = rotations_cells
                    piece_colors[suffix] = color

        return piece_shapes, piece_colors


    # ------------------------------------------------------------------
    # Helpers de inicialización
    # ------------------------------------------------------------------
    
    def _refill_next_queue(self, full=False):
        """
        Rellena self.next_queue con piezas aleatorias hasta tener
        self.next_queue_length elementos.

        Si full=True, vacía primero la cola.
        """
        if full:
            self.next_queue = []

        while len(self.next_queue) < self.next_queue_length:
            self.next_queue.append(self._random_piece_kind())

    def _init_pieces(self):
        # Tablero nuevo con paredes (según tu _make_empty_board actual)
        self.board = self._make_empty_board()

        self.current_piece       = None
        self.score               = 0
        self.level               = 1
        self.total_lines_cleared = 0
        self.game_over           = False
        self.paused              = False
        self.accum_ms            = 0

        # Velocidad base desde el .brik
        self.tick_ms = self.base_tick_ms

        # Inicializar la cola de próximas piezas y spawnear la primera
        self._refill_next_queue(full=True)
        self._spawn_new_piece()


        # self.next_piece_kind = self._random_piece_kind()
        # self._spawn_new_piece()

    def _random_piece_kind(self):
        """
        Selecciona una pieza aleatoria.
        Si random_bombs está activo y hay bombas disponibles,
        usa bomb_chance para decidir si devolver una bomba.
        """
        # Si no hay bombas configuradas o random_bombs está desactivado
        if not self.random_bombs or not self.bomb_pieces:
            # Solo piezas normales
            if self.normal_pieces:
                return self._rng.choice(self.normal_pieces)
            else:
                return self._rng.choice(self.piece_kinds)
        
        # Decidir si spawnear bomba o pieza normal
        if self._rng.random() < self.bomb_chance:
            # Spawnear bomba
            return self._rng.choice(self.bomb_pieces)
        else:
            # Spawnear pieza normal
            if self.normal_pieces:
                return self._rng.choice(self.normal_pieces)
            else:
                return self._rng.choice(self.piece_kinds)
    
    def _draw_preview_piece(self, engine, kind, top_y_px):
        """
        Dibuja una preview de la pieza 'kind' en el panel de info,
        centrada horizontalmente y comenzando en la coordenada Y dada
        (en píxeles dentro del panel de info).
        """
        canvas = engine.info_canvas
        if canvas is None:
            return

        # Usamos la rotación 0 para la preview
        shape = self.piece_shapes[kind][0]
        color = self.piece_colors.get(kind, "#ffffff")

        # Tamaño de celda para la preview (más pequeño que el de juego)
        cell = max(4, engine.cell_size // 2)

        min_x = min(px for (px, py) in shape)
        max_x = max(px for (px, py) in shape)
        min_y = min(py for (px, py) in shape)
        max_y = max(py for (px, py) in shape)

        width_cells  = max_x - min_x + 1
        height_cells = max_y - min_y + 1

        total_w_px = width_cells * cell
        total_h_px = height_cells * cell

        center_x = engine.info_width_px // 2
        start_x = center_x - total_w_px // 2
        start_y = top_y_px

        for (gx, gy) in shape:
            x0 = start_x + (gx - min_x) * cell
            y0 = start_y + (gy - min_y) * cell
            x1 = x0 + cell
            y1 = y0 + cell
            canvas.create_rectangle(
                x0, y0, x1, y1,
                fill=color,
                outline="gray30"
            )
            
    def _spawn_new_piece(self):
        # Si la cola está vacía por alguna razón, la rellenamos
        if not self.next_queue:
            self._refill_next_queue(full=True)

        # Tomamos la primera pieza de la cola
        kind = self.next_queue.pop(0)
        rotation = 0

        shape = self.piece_shapes[kind][rotation]
        min_x = min(px for (px, py) in shape)
        max_x = max(px for (px, py) in shape)
        width = max_x - min_x + 1

        # --- IMPORTANTE: tenemos paredes en x=0 y x=board_w-1 ---
        inner_w = self.board_w - 2  # columnas 1..board_w-2

        # Centramos dentro del interior y desplazamos +1 por la pared izquierda
        x = 1 + (inner_w - width) // 2 - min_x

        # Aparece justo debajo de la pared superior (y=0 es pared)
        y = 1

        piece = Piece(kind, x, y, rotation)

        if not self._can_place(piece, x, y, rotation):
            # No hay espacio para nueva pieza -> game over
            self.game_over = True
            return

        self.current_piece = piece

        # Aseguramos que la cola siga llena para las siguientes previews
        self._refill_next_queue(full=False)

    # ------------------------------------------------------------------
    # Utilidades sobre piezas/tablero
    # ------------------------------------------------------------------

    def _piece_cells(self, piece, x=None, y=None, rotation=None):
        if piece is None:
            return []
        if x is None:
            x = piece.x
        if y is None:
            y = piece.y
        if rotation is None:
            rotation = piece.rotation

        shape = self.piece_shapes[piece.kind][rotation % len(self.piece_shapes[piece.kind])]
        cells = []
        for dx, dy in shape:
            cells.append((x + dx, y + dy))
        return cells

    def _can_place(self, piece, x, y, rotation):
        for cx, cy in self._piece_cells(piece, x, y, rotation):
            if cx < 0 or cx >= self.board_w:
                return False
            if cy < 0 or cy >= self.board_h:
                return False
            if self.board[cy][cx] is not None:
                return False
        return True

    def _lock_piece(self):
        if self.current_piece is None:
            return
        
        # Verificar si la pieza es una bomba
        is_bomb = "bomb" in self.current_piece.kind.lower()
        
        if is_bomb:
            # Obtener las celdas que ocupa la bomba
            cells = self._piece_cells(self.current_piece)
            if len(cells) > 0:
                # Encontrar la esquina superior izquierda de la bomba
                min_x = min(cx for cx, cy in cells)
                min_y = min(cy for cx, cy in cells)
                
                # Determinar el tamaño de la bomba y la explosión
                if "1x1" in self.current_piece.kind:
                    # Bomba 1x1: explota 3x3 centrada en la bomba
                    # min_x, min_y es la posición de la bomba
                    # Explosión: desde (min_x-1, min_y-1) hasta (min_x+1, min_y+1)
                    self._explode_bomb_area(min_x - 1, min_y - 1, 3, 3)
                elif "2x2" in self.current_piece.kind:
                    # Bomba 2x2: explota 4x4 con la bomba 2x2 en el centro
                    # Bomba ocupa: (min_x, min_y) a (min_x+1, min_y+1)
                    # Explosión: desde (min_x-1, min_y-1) hasta (min_x+2, min_y+2)
                    self._explode_bomb_area(min_x - 1, min_y - 1, 4, 4)
                else:
                    # Por defecto: 3x3
                    self._explode_bomb_area(min_x - 1, min_y - 1, 3, 3)
            
            self.current_piece = None
            self._spawn_new_piece()
        else:
            # Pieza normal: fijar al tablero
            color = self.piece_colors.get(self.current_piece.kind, "#ffffff")
            for cx, cy in self._piece_cells(self.current_piece):
                if 0 <= cy < self.board_h and 0 <= cx < self.board_w:
                    self.board[cy][cx] = color
            self.current_piece = None

            lines = self._clear_full_lines()
            if lines > 0:
                self._apply_scoring(lines)
                self._update_level(lines)

            self._spawn_new_piece()
    
    def _explode_bomb_area(self, start_x, start_y, width, height):
        """
        Explota una bomba borrando todas las celdas en un área rectangular.
        
        Args:
            start_x: coordenada X de la esquina superior izquierda del área
            start_y: coordenada Y de la esquina superior izquierda del área
            width: ancho del área de explosión
            height: alto del área de explosión
        
        Ejemplo:
            - Bomba 1x1 en (5, 10): _explode_bomb_area(4, 9, 3, 3)
              Borra desde (4,9) hasta (6,11) = 3x3
            
            - Bomba 2x2 en (5, 10): _explode_bomb_area(4, 9, 4, 4)
              Borra desde (4,9) hasta (7,12) = 4x4
        """
        # Borrar todas las celdas en el área de explosión
        for dy in range(height):
            for dx in range(width):
                ex = start_x + dx
                ey = start_y + dy
                
                # Verificar que esté dentro del tablero (sin tocar paredes)
                if 0 < ex < self.board_w - 1 and 0 < ey < self.board_h - 1:
                    # Borrar la celda
                    self.board[ey][ex] = None
        
        # Bonus de puntos por usar bomba (basado en área)
        self.score += (width * height) * 10

    def _clear_full_lines(self):
        new_board = []
        cleared = 0

        for y in range(self.board_h):
            row = self.board[y]

            # Fila superior e inferior: son paredes, siempre se conservan
            if y == 0 or y == self.board_h - 1:
                new_board.append(list(row))
                continue

            # Revisar SOLO las columnas interiores (1..board_w-2)
            if all(row[x] is not None for x in range(1, self.board_w - 1)):
                # Linea completa -> se elimina
                cleared += 1
            else:
                new_board.append(list(row))

        # Rellenar filas nuevas por arriba del pozo, manteniendo paredes laterales
        while len(new_board) < self.board_h:
            row = [None] * self.board_w
            row[0] = self.wall_color
            row[self.board_w - 1] = self.wall_color
            # Insertamos en y=1 (justo debajo de la pared superior)
            new_board.insert(1, row)

        self.board = new_board
        self.total_lines_cleared += cleared
        return cleared


    def _apply_scoring(self, lines):
        if lines <= 0:
            return
        if lines == 4:
            self.score += self.score_per_tetris
        else:
            self.score += self.score_per_line * lines

    def _update_level(self, lines_cleared_now):
        if self.lines_per_level <= 0:
            return
        # recomputar nivel desde cero
        self.level = 1 + (self.total_lines_cleared // self.lines_per_level)
        # ajustar velocidad
        new_tick = self.base_tick_ms - (self.level - 1) * self.tick_delta_per_level
        if new_tick < self.min_tick_ms:
            new_tick = self.min_tick_ms
        self.tick_ms = new_tick

    # ------------------------------------------------------------------
    # API esperada por el motor
    # ------------------------------------------------------------------

    def reset(self):
        self._init_pieces()

    def on_key(self, keysym):
        k = keysym.lower()

        if k == self.key_pause:
            if not self.game_over:
                self.paused = not self.paused
            return

        if k == self.key_restart:
            self.reset()
            return

        if self.game_over or self.paused:
            return

        if self.current_piece is None:
            return

        if k == self.key_left:
            self._move_piece(-1, 0)
        elif k == self.key_right:
            self._move_piece(1, 0)
        elif k == self.key_down:
            # soft drop
            moved = self._move_piece(0, 1)
            if moved:
                # pequeño bonus por caída suave, opcional
                self.score += 1
        elif k == self.key_rotate:
            self._rotate_piece()
        elif k == self.key_drop:
            # hard drop
            moved = True
            while moved:
                moved = self._move_piece(0, 1)
                if moved:
                    self.score += 2
            # no se pudo mover más -> bloquear pieza
            self._lock_piece()

    def _move_piece(self, dx, dy):
        if self.current_piece is None:
            return False
        new_x = self.current_piece.x + dx
        new_y = self.current_piece.y + dy
        if self._can_place(self.current_piece, new_x, new_y, self.current_piece.rotation):
            self.current_piece.x = new_x
            self.current_piece.y = new_y
            return True
        else:
            # si estamos intentando mover hacia abajo y no se puede,
            # significa que la pieza se posa y se bloquea.
            if dy > 0:
                self._lock_piece()
            return False

    def _rotate_piece(self):
        if self.current_piece is None:
            return
        new_rot = (self.current_piece.rotation + 1) % 4
        # intento de rotación simple; si choca, se cancela
        if self._can_place(self.current_piece, self.current_piece.x, self.current_piece.y, new_rot):
            self.current_piece.rotation = new_rot

    def update(self, dt_ms):
        if self.game_over or self.paused:
            return

        self.accum_ms += dt_ms
        while self.accum_ms >= self.tick_ms:
            self.accum_ms -= self.tick_ms
            if self.current_piece is None:
                self._spawn_new_piece()
                if self.game_over:
                    return
            else:
                self._move_piece(0, 1)

    def _compute_ghost_cells(self):
        """
        Calcula la posición de la 'ghost piece' (sombra) para la pieza actual.
        Devuelve una lista de (x, y) en tablero, o None si no aplica.
        No modifica self.current_piece.
        """
        if self.current_piece is None:
            return None
        if self.game_over:
            return None

        piece = self.current_piece

        # Usamos x, y, rotation actuales, pero sin modificar la pieza real.
        x = piece.x
        y = piece.y
        rotation = piece.rotation

        # Bajamos la pieza virtualmente hasta que ya no pueda bajar más.
        while self._can_place(piece, x, y + 1, rotation):
            y += 1

        # Si la sombra está exactamente en la misma fila que la pieza real,
        # igual la dibujamos (la pieza real luego la tapa), no hace daño.
        # Calculamos las celdas finales en (x, y) encontrado.
        return self._piece_cells(piece, x=x, y=y, rotation=rotation)

    def draw(self, engine):

        # Fondo
        for y in range(self.board_h):
            for x in range(self.board_w):
                engine.draw_brick(x, y, color=self.color_bg)

        # Tablero fijo
        for y in range(self.board_h):
            for x in range(self.board_w):
                color = self.board[y][x]
                if color is not None:
                    engine.draw_brick(x, y, color=color)
        
        # ---------- GHOST PIECE (sombra) ----------
        ghost_cells = self._compute_ghost_cells()
        if ghost_cells is not None:
            for (gx, gy) in ghost_cells:
                # Solo dibujamos en celdas vacías para no tapar bloques fijos
                if 0 <= gx < self.board_w and 0 <= gy < self.board_h:
                    if self.board[gy][gx] is None:
                        engine.draw_brick(gx, gy, color=self.ghost_color)

        # Pieza actual
        if self.current_piece is not None:
            color = self.piece_colors.get(self.current_piece.kind, "#ffffff")
            for cx, cy in self._piece_cells(self.current_piece):
                if 0 <= cx < self.board_w and 0 <= cy < self.board_h:
                    engine.draw_brick(cx, cy, color=color)

        # ---------- Panel de info ----------
        if engine.info_canvas is not None:
            center_x = engine.info_width_px // 2

            # Título
            engine.draw_text(
                center_x, 30,
                "Tetris",
                where="info",
                anchor="n",
                font=engine.font_title
            )

            engine.draw_hline(70, where="info")

            # Score / nivel / líneas, etc. (lo que ya tenías)
            engine.draw_text(
                20, 90,
                "Score:",
                where="info",
                anchor="nw",
                font=engine.font_label
            )
            engine.draw_text(
                20, 110,
                str(self.score),
                where="info",
                anchor="nw",
                font=engine.font_value
            )

            # ... más stats si quieres ...

            engine.draw_hline(180, where="info")

            # Cola de próximas piezas
            engine.draw_text(
                20, 190,
                "Next pieces:",
                where="info",
                anchor="nw",
                font=engine.font_label
            )

            # dibujamos hasta next_queue_length previews
            y0 = 215
            spacing = 60  # separación vertical entre previews
            for i, kind in enumerate(self.next_queue[:self.next_queue_length]):
                top_y = y0 + i * spacing
                self._draw_preview_piece(engine, kind, top_y_px=top_y)


            if self.game_over:
                engine.draw_text(
                    center_x, 420,
                    "GAME OVER",
                    where="info",
                    anchor="n",
                    font=engine.font_title
                )
                engine.draw_text(
                    center_x, 450,
                    "Pulsa %s para reiniciar" % self.key_restart.upper(),
                    where="info",
                    anchor="n",
                    font=engine.font_hint
                )
            elif self.paused:
                engine.draw_text(
                    center_x, 420,
                    "PAUSA",
                    where="info",
                    anchor="n",
                    font=engine.font_title
                )
