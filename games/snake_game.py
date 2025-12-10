# -*- coding: utf-8 -*-
"""
==========================================
SNAKE GAME - Brick Game Engine
==========================================

Implementación completa del juego Snake con:
- Portales de teletransporte (aleatorios o fijos)
- Sistema de aceleración progresiva
- Configuración completa vía snake.brik
- Sistema de colisiones configurable
- Spawn de manzanas con respawn automático

TABLA DE CONTENIDOS:
    1. Imports y constantes (líneas 1-10)
    2. SnakeGame.__init__ (líneas 30-150)
        - Lectura de parámetros del tablero
        - Configuración de controles
        - Parámetros del snake
        - Sistema de portales
        - Reglas del juego
        - Estado inicial
    3. Métodos de estado del juego (líneas 150-250)
        - reset_game()
        - is_game_over()
    4. Sistema de portales (líneas 250-300)
        - generate_random_portals()
        - find_portal_exit()
    5. Sistema de spawning (líneas 300-400)
        - spawn_snake()
        - spawn_apple()
    6. Lógica del juego (líneas 400-500)
        - on_key() - Manejo de input
        - update() - Game loop principal
        - move_snake() - Movimiento
        - check_collisions() - Detección de colisiones
    7. Rendering (líneas 500-549)
        - draw() - Dibuja todo el juego
        - draw_board()
        - draw_ui()

==========================================
"""
from __future__ import print_function

import random

from games.base_game import BaseGame
from runtime import sym_int, sym_str, sym_bool, sym_get


class SnakeGame(BaseGame):
    """
    Implementación del juego Snake que lee TODA la configuración
    desde la tabla de símbolos generada a partir de snake.brik.
    """

    def __init__(self, engine, symbols):
        super(SnakeGame, self).__init__(engine, symbols)

        # ------------------------------------------------------------------
        # Lectura de parámetros del tablero
        # ------------------------------------------------------------------
        self.board_w = sym_int(symbols, 'board.width', engine.grid_width)
        self.board_h = sym_int(symbols, 'board.height', engine.grid_height)
        self.wrap = sym_bool(symbols, 'board.wrap', False)
        self.cell_size = sym_int(symbols, 'board.cell_size', 20)

        # Colores (los usaremos al dibujar)
        self.color_bg = sym_str(symbols, 'board.colors.background', '#000000')
        self.color_grid = sym_str(symbols, 'board.colors.grid', '#333333')
        self.color_snake = sym_str(symbols, 'board.colors.snake_body', '#00FF00')
        self.color_walls = sym_str(symbols, 'board.colors.walls', '#888888')
        self.color_apple = sym_str(symbols, 'board.colors.apple', '#FF0000')
        self.color_portal = sym_str(symbols, "board.colors.portal", "#FFD700")


        # ------------------------------------------------------------------
        # Controles – vienen como strings tipo "right", "left", "p", "r"
        # ------------------------------------------------------------------
        self.key_right = sym_str(symbols, 'controls.right_mov', 'right').lower()
        self.key_left  = sym_str(symbols, 'controls.left_mov',  'left').lower()
        self.key_up    = sym_str(symbols, 'controls.up_mov',    'up').lower()
        self.key_down  = sym_str(symbols, 'controls.down_mov',  'down').lower()
        self.key_pause   = sym_str(symbols, 'controls.pause',   'p').lower()
        self.key_restart = sym_str(symbols, 'controls.restart', 'r').lower()

        # ------------------------------------------------------------------
        # Parámetros propios del Snake
        # ------------------------------------------------------------------
        self.tick_ms_cfg = sym_int(symbols, 'snake.tick_ms', 150)
        self.initial_length = sym_int(symbols, 'snake.initial_length', 4)
        self.growth_per_apple = sym_int(symbols, 'snake.growth_per_apple', 1)
        self.spawn_mode = sym_str(symbols, 'snake.spawn', 'center').lower()

        # Reglas de fin de juego
        self.rule_out_of_bounds = sym_str(
            symbols, 'rules_end_game.on_out_of_bounds', 'end'
        ).lower()
        self.rule_self_collision = sym_str(
            symbols, 'rules_end_game.on_self_collision', 'end'
        ).lower()
        self.rule_wall_collision = sym_str(
            symbols, 'rules_end_game.on_wall_collision', 'end'
        ).lower()

        # Reglas de puntuación y velocidad
        self.apple_points = sym_int(symbols, 'rules_scoring.apple_points', 1)
        self.speedup_after_apple = sym_int(
            symbols, 'rules_speed_progression.speedup_after_apple', 0
        )
        self.min_tick_ms = sym_int(
            symbols, 'rules_speed_progression.min_tick_ms', self.tick_ms_cfg
        )

        # Reglas de aparición de manzanas (por ahora, muy sencillas)
        self.apple_respawn_ticks = sym_int(
            symbols, 'rules_spawning.apple_respawn_ticks', 0
        )
        self.max_apples = sym_int(symbols, 'rules_spawning.max_apples', 1)

        # ------------------------------------------------------------------
        # Paredes a partir de level.grid
        # ------------------------------------------------------------------
        self.walls = set()
        level_grid = sym_get(symbols, 'level.grid', None)
        if level_grid is not None:
            self._build_walls_from_grid(level_grid)

        # Zona jugable interior (dentro del rectángulo de paredes)
        if self.walls:
            xs = [x for (x, y) in self.walls]
            ys = [y for (x, y) in self.walls]
            self.wall_min_x = min(xs)
            self.wall_max_x = max(xs)
            self.wall_min_y = min(ys)
            self.wall_max_y = max(ys)
        else:
            # Si no hay paredes, usamos todo el board
            self.wall_min_x = 0
            self.wall_max_x = self.board_w - 1
            self.wall_min_y = 0
            self.wall_max_y = self.board_h - 1


        # ------------------------------------------------------------------
        # Estado del juego
        # ------------------------------------------------------------------
        self.score = 0
        self.game_over = False
        self.paused = False

        # Dirección actual y pendiente (dx, dy)
        self.current_dir = (1, 0)  # empieza hacia la derecha
        self.pending_dir = self.current_dir

        # Manejo de tiempo
        self.accum_ms = 0
        self.tick_ms = self.tick_ms_cfg

        # Crecimiento y velocidad
        self.apples_eaten = 0
        self._growth_pending = 0

        # Snake y comida
        self.snake = []
        self.snake_set = set()
        self.food = None
        
        # ------------------ Portales ------------------
        self.portals = {}
        self.portal_cells = set()
        self.portal_colors = {}  # Mapea cada celda de portal a su color
        self.portal_pairs = []   # Lista de pares [(a, b, color_a, color_b), ...]
        
        # Colores predefinidos para pares de portales
        self.pair_colors = [
            ("#9932CC", "#FF69B4"),  # Morado y rosado (par 1)
            ("#00CED1", "#FFD700"),  # Cian y dorado (par 2)
            ("#FF4500", "#32CD32"),  # Naranja y verde lima (par 3)
        ]

        # ¿Portales aleatorios o fijos del .brik?
        self.portal_random = sym_bool(symbols, "portals.random", False)
        self.portal_num_pairs = sym_int(symbols, "portals.num_pairs", 2)

        if self.portal_random:
            # Creamos portales aleatorios
            self._spawn_random_portals()
        else:
            # Usamos los fijos del .brik (tu estructura original)
            pair_idx = 0
            p1_from = sym_get(symbols, "portals.p1_from", None)
            p1_to   = sym_get(symbols, "portals.p1_to",   None)
            if p1_from and p1_to:
                a = self._clamp_to_play_area(p1_from[0], p1_from[1])
                b = self._clamp_to_play_area(p1_to[0],   p1_to[1])
                color_a, color_b = self.pair_colors[pair_idx % len(self.pair_colors)]
                
                self.portals[a] = b
                self.portals[b] = a
                self.portal_cells.add(a)
                self.portal_cells.add(b)
                self.portal_colors[a] = color_a
                self.portal_colors[b] = color_b
                self.portal_pairs.append((a, b, color_a, color_b))
                pair_idx += 1

            p2_from = sym_get(symbols, "portals.p2_from", None)
            p2_to   = sym_get(symbols, "portals.p2_to",   None)
            if p2_from and p2_to:
                a = self._clamp_to_play_area(p2_from[0], p2_from[1])
                b = self._clamp_to_play_area(p2_to[0],   p2_to[1])
                color_a, color_b = self.pair_colors[pair_idx % len(self.pair_colors)]
                
                self.portals[a] = b
                self.portals[b] = a
                self.portal_cells.add(a)
                self.portal_cells.add(b)
                self.portal_colors[a] = color_a
                self.portal_colors[b] = color_b
                self.portal_pairs.append((a, b, color_a, color_b))
                pair_idx += 1

        self._init_snake_and_food()

    def _clamp_to_play_area(self, x, y):
        x = max(self.wall_min_x + 1, min(self.wall_max_x - 1, x))
        y = max(self.wall_min_y + 1, min(self.wall_max_y - 1, y))
        return (x, y)

    def _spawn_random_portals(self):
        """
        Crea pares de portales aleatorios, respetando walls.
        Rellena self.portals, self.portal_cells, self.portal_colors y self.portal_pairs.
        Los portales no aparecerán en los bordes (al menos 2 celdas de margen).
        """

        self.portals.clear()
        self.portal_cells.clear()
        self.portal_colors.clear()
        self.portal_pairs = []

        valid = []
        # Restringir para que no queden en los bordes (dejar al menos 2 celdas de margen)
        for y in range(self.wall_min_y + 2, self.wall_max_y - 1):
            for x in range(self.wall_min_x + 2, self.wall_max_x - 1):
                pos = (x, y)
                if pos in self.walls:
                    continue
                valid.append(pos)

        if not valid:
            return

        # Número de pares que realmente podemos crear
        max_pairs = len(valid) // 2
        num_pairs = min(self.portal_num_pairs, max_pairs)
        if num_pairs <= 0:
            return

        # Elegimos 2 * num_pairs celdas distintas
        chosen = random.sample(valid, 2 * num_pairs)

        for i in range(num_pairs):
            a = chosen[2 * i]
            b = chosen[2 * i + 1]
            
            # Obtener colores para este par
            color_a, color_b = self.pair_colors[i % len(self.pair_colors)]

            # mapa bidireccional
            self.portals[a] = b
            self.portals[b] = a

            self.portal_cells.add(a)
            self.portal_cells.add(b)
            
            self.portal_colors[a] = color_a
            self.portal_colors[b] = color_b
            
            self.portal_pairs.append((a, b, color_a, color_b))


    # ======================================================================
    #  Construcción de paredes a partir de level.grid
    # ======================================================================

    def _build_walls_from_grid(self, grid):
        """
        level.grid es una matriz de 0/1. La usamos para obst&aacute;culos internos.
        La orilla (primera/última fila/columna) la ignoramos porque el marco
        externo lo construimos aparte para que sea único y cubra todo el board.
        """
        h = len(grid)
        if h == 0:
            return

        w = len(grid[0])
        if w == 0:
            return

        # Escala para encajar level.grid en el board
        sx = max(1, self.board_w // w)
        sy = max(1, self.board_h // h)

        # Obstáculos internos (ignoramos la orilla del grid)
        for gy in range(h):
            row = grid[gy]
            for gx in range(len(row)):
                if not row[gx]:
                    continue

                # Saltamos borde del grid: primera/última fila/columna
                if gy == 0 or gy == h - 1 or gx == 0 or gx == w - 1:
                    continue

                # Solo celdas internas llegan aquí
                for y in range(gy * sy, (gy + 1) * sy):
                    for x in range(gx * sx, (gx + 1) * sx):
                        if 0 <= x < self.board_w and 0 <= y < self.board_h:
                            self.walls.add((x, y))

        # --- ÚNICO marco externo en TODO el borde del tablero ---
        # Lados superior e inferior
        for x in range(self.board_w):
            self.walls.add((x, 0))
            self.walls.add((x, self.board_h - 1))

        # Lados izquierdo y derecho
        for y in range(self.board_h):
            self.walls.add((0, y))
            self.walls.add((self.board_w - 1, y))

    # ======================================================================
    #  Inicialización de snake + comida
    # ======================================================================

    def _init_snake_and_food(self):
        # Posición de la cabeza
        if self.spawn_mode == 'random':
            head = self._random_empty_cell()
        else:
            head = (self.board_w // 2, self.board_h // 2)
            if head in self.walls:
                head = self._random_empty_cell()

        self.snake = [head]
        self.snake_set = set(self.snake)

        # Cuerpo inicial extendido en dirección opuesta al movimiento
        dx, dy = self.current_dir
        for i in range(1, self.initial_length):
            x = head[0] - dx * i
            y = head[1] - dy * i
            if x < 0 or x >= self.board_w or y < 0 or y >= self.board_h:
                break
            if (x, y) in self.walls:
                break
            self.snake.append((x, y))
            self.snake_set.add((x, y))

        # Colocamos comida
        self._spawn_food()

    def _random_empty_cell(self):
        """
        Retorna una celda vacía DENTRO del rectángulo de paredes,
        sin paredes, snake ni portales.
        """
        while True:
            x = random.randint(self.wall_min_x + 1, self.wall_max_x - 1)
            y = random.randint(self.wall_min_y + 1, self.wall_max_y - 1)
            pos = (x, y)

            if pos in self.snake_set:
                continue
            if pos in self.walls:
                continue
            if pos in self.portal_cells:
                continue
            return pos

    def _spawn_food(self):
        self.food = self._random_empty_cell()

    # ======================================================================
    #  API esperada por el GameEngine
    # ======================================================================

    def reset(self):
        """Reinicia el juego usando de nuevo la tabla de símbolos."""
        self.score = 0
        self.game_over = False
        self.paused = False

        self.current_dir = (1, 0)
        self.pending_dir = self.current_dir

        self.accum_ms = 0
        self.tick_ms = self.tick_ms_cfg

        self.apples_eaten = 0
        self._growth_pending = 0

        self.snake = []
        self.snake_set = set()
        self.food = None
        self._init_snake_and_food()

        if self.portal_random:
            self._spawn_random_portals()


    def on_key(self, keysym):
        k = keysym.lower()

        # Pausa
        if k == self.key_pause:
            if not self.game_over:
                self.paused = not self.paused
            return

        # Reinicio
        if k == self.key_restart:
            self.reset()
            return

        # Cambio de dirección
        if k == self.key_right:
            new_dir = (1, 0)
        elif k == self.key_left:
            new_dir = (-1, 0)
        elif k == self.key_up:
            new_dir = (0, -1)
        elif k == self.key_down:
            new_dir = (0, 1)
        else:
            return

        # Evitar giro de 180° si la snake tiene longitud > 1
        if len(self.snake) > 1:
            if (new_dir[0] == -self.current_dir[0] and
                    new_dir[1] == -self.current_dir[1]):
                return

        self.pending_dir = new_dir

    def update(self, dt_ms):
        if self.game_over or self.paused:
            return

        self.accum_ms += dt_ms
        # Puede dar más de un tick si dt_ms es muy grande
        while self.accum_ms >= self.tick_ms:
            self.accum_ms -= self.tick_ms
            self._step()

    def _step(self):
        # Aplicar dirección pendiente
        self.current_dir = self.pending_dir

        head_x, head_y = self.snake[0]
        dx, dy = self.current_dir
        nx = head_x + dx
        ny = head_y + dy

        # Wrap-around opcional
        if self.wrap:
            nx %= self.board_w
            ny %= self.board_h

        out_of_bounds = not (0 <= nx < self.board_w and 0 <= ny < self.board_h)
        new_head = (nx, ny)

        new_head = (nx, ny)

        # Portal
        if new_head in self.portals:
            new_head = self.portals[new_head]


        # Fuera de los límites (sin wrap)
        if out_of_bounds:
            if self.rule_out_of_bounds == 'end':
                self.game_over = True
            return

        # Contra pared
        if new_head in self.walls:
            if self.rule_wall_collision == 'end':
                self.game_over = True
            return

        # Contra sí misma
        if new_head in self.snake_set:
            if self.rule_self_collision == 'end':
                self.game_over = True
            return

        # Avanzar snake
        self.snake.insert(0, new_head)
        self.snake_set.add(new_head)

        # Comer manzana
        if self.food is not None and new_head == self.food:
            self.score += self.apple_points
            self.apples_eaten += 1
            self._growth_pending += self.growth_per_apple
            self._spawn_food()

            # Progresión de velocidad
            if (self.speedup_after_apple > 0 and
                    (self.apples_eaten % self.speedup_after_apple) == 0):
                new_tick = int(self.tick_ms * 0.9)
                if new_tick < self.min_tick_ms:
                    new_tick = self.min_tick_ms
                self.tick_ms = new_tick
        else:
            # Sin comer: crece solo si hay crecimiento pendiente
            if self._growth_pending > 0:
                self._growth_pending -= 1
            else:
                tail = self.snake.pop()
                self.snake_set.remove(tail)
                
                
    def draw(self, engine):
        """
        Dibuja paredes, snake y manzana usando el API de dibujo del engine.
        """

        # Paredes
        for (x, y) in self.walls:
            engine.draw_brick(x, y, color=self.color_walls)

        # Portales con colores individuales
        for (x, y) in self.portal_cells:
            color = self.portal_colors.get((x, y), "#FFD700")  # Fallback a dorado
            engine.draw_brick(x, y, color=color)

        # Snake
        for (x, y) in self.snake:
            engine.draw_brick(x, y, color=self.color_snake)

        # Comida
        if self.food is not None:
            fx, fy = self.food
            engine.draw_brick(fx, fy, color=self.color_apple)

        # ---------------- Panel de info (derecha) ----------------
        if engine.info_canvas is not None:
            center_x = engine.info_width_px // 2

            engine.draw_text(
                center_x, 30,
                "Snake",
                where="info",
                anchor="n",
                font=engine.font_title
            )

            engine.draw_hline(60, where="info")

            engine.draw_text(
                20, 80,
                "Score:",
                where="info",
                anchor="nw",
                font=engine.font_label
            )
            engine.draw_text(
                20, 100,
                str(self.score),
                where="info",
                anchor="nw",
                font=engine.font_value
            )

            engine.draw_text(
                20, 130,
                "Tick actual: %d ms" % self.tick_ms,
                where="info",
                anchor="nw",
                font=engine.font_hint
            )

            engine.draw_text(
                20, 160,
                "Manzanas comidas: %d" % self.apples_eaten,
                where="info",
                anchor="nw",
                font=engine.font_hint
            )

            engine.draw_hline(190, where="info")
            
            # Mostrar información de portales
            if len(self.portal_pairs) > 0:
                engine.draw_text(
                    20, 210,
                    "Portales:",
                    where="info",
                    anchor="nw",
                    font=engine.font_label
                )
                
                y_offset = 230
                for idx, (a, b, color_a, color_b) in enumerate(self.portal_pairs):
                    # Dibujar indicador de color para portal A
                    if hasattr(engine.info_canvas, 'create_rectangle'):
                        x_start = 20
                        engine.info_canvas.create_rectangle(
                            x_start, y_offset, x_start + 15, y_offset + 15,
                            fill=color_a, outline="white"
                        )
                        engine.info_canvas.create_text(
                            x_start + 20, y_offset + 7,
                            text="<->",
                            fill="white",
                            anchor="w",
                            font=engine.font_hint
                        )
                        engine.info_canvas.create_rectangle(
                            x_start + 45, y_offset, x_start + 60, y_offset + 15,
                            fill=color_b, outline="white"
                        )
                    y_offset += 20
                
                engine.draw_hline(y_offset + 10, where="info")

            if self.game_over:
                engine.draw_text(
                    center_x, y_offset + 30,
                    "GAME OVER",
                    where="info",
                    anchor="n",
                    font=engine.font_title
                )
                engine.draw_text(
                    center_x, 260,
                    "Pulsa %s para reiniciar" % self.key_restart.upper(),
                    where="info",
                    anchor="n",
                    font=engine.font_hint
                )
            elif self.paused:
                engine.draw_text(
                    center_x, 230,
                    "PAUSA",
                    where="info",
                    anchor="n",
                    font=engine.font_title
                )
