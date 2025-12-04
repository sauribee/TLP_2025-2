# -*- coding: utf-8 -*-
import Tkinter as tk
import tkFont
import time


class GameEngine(object):

    def __init__(self,
                    game_width_px=640,
                    height_px=480,
                    cell_size=20,
                    tick_ms=50,
                    info_width_px=360):

        self.game_width_px = game_width_px
        self.height_px = height_px
        self.cell_size = cell_size
        self.tick_ms = tick_ms
        self.info_width_px = info_width_px

        # Tamaño de la grilla SOLO sobre el área de juego
        self.grid_width = int(self.game_width_px / self.cell_size)
        self.grid_height = int(self.height_px / self.cell_size)

        # Referencia al juego actual (objeto con on_key, update, draw)
        self._game = None

        # Estado del loop
        self._running = False
        self._last_time_ms = None

        # ---------- Tkinter ----------
        self.root = tk.Tk()
        self.root.title("Brick Game Engine (Python 2.7)")
        self.root.resizable(0, 0)

        # Frame principal: dentro ponemos los canvas
        self.main_frame = tk.Frame(self.root)
        self.main_frame.pack()

        # Canvas para el área de juego (izquierda)
        self.game_canvas = tk.Canvas(
            self.main_frame,
            width=self.game_width_px,
            height=self.height_px,
            bg="black",
            highlightthickness=0
        )
        self.game_canvas.pack(side="left")

        # Canvas para el panel de info (derecha), opcional
        self.info_canvas = None
        if self.info_width_px > 0:
            self.info_canvas = tk.Canvas(
                self.main_frame,
                width=self.info_width_px,
                height=self.height_px,
                bg="gray12",
                highlightthickness=0
            )
            self.info_canvas.pack(side="left")

        # Tipografías (todas estándar de Tk; si no hay Helvetica, hace fallback)
        self.font_title = tkFont.Font(family="Helvetica", size=16, weight="bold")
        self.font_label = tkFont.Font(family="Helvetica", size=10, weight="bold")
        self.font_value = tkFont.Font(family="Helvetica", size=10)
        self.font_hint = tkFont.Font(family="Helvetica", size=9)

        # Eventos de teclado
        self.root.bind("<KeyPress>", self._on_key_press)

        # Manejo de cierre de ventana
        self.root.protocol("WM_DELETE_WINDOW", self.stop)

        # Asegurar que la ventana reciba el foco de teclado
        self.game_canvas.focus_set()

    # ------------------------------------------------------------------
    # API pública del motor
    # ------------------------------------------------------------------

    def set_game(self, game):
        """
        Asigna el "juego" actual.
        El juego debe implementar:
            - on_key(keysym)
            - update(dt_ms)
            - draw(engine)
        """
        self._game = game

    def start(self):
        """
        Inicia el game loop y entra al mainloop de Tkinter.
        """
        if self._game is None:
            raise RuntimeError("No hay juego asignado. Usa set_game(...) antes de start().")

        self._running = True
        self._last_time_ms = int(round(time.time() * 1000))

        # Arranca el loop del motor
        self.root.after(self.tick_ms, self._loop)
        self.root.mainloop()

    def stop(self):
        """
        Detiene el motor y cierra la ventana.
        """
        self._running = False
        try:
            self.root.destroy()
        except tk.TclError:
            # Si ya está destruida, ignoramos el error.
            pass

    # ------------------------------------------------------------------
    # Funciones gráficas que exponemos a los juegos
    # ------------------------------------------------------------------

    def clear(self):
        """
        Borra todo lo dibujado en el frame actual,
        tanto en el área de juego como en el panel de info (si existe).
        """
        self.game_canvas.delete("all")
        if self.info_canvas is not None:
            self.info_canvas.delete("all")

    def draw_brick(self, grid_x, grid_y, color="#00ff00"):
        """
        Dibuja un ladrillo en coordenadas de grilla (grid_x, grid_y)
        sobre el área de juego.

        grid_x, grid_y: enteros entre 0 y grid_width/grid_height - 1.
        """
        cs = self.cell_size
        x0 = grid_x * cs
        y0 = grid_y * cs
        x1 = x0 + cs
        y1 = y0 + cs

        self.game_canvas.create_rectangle(
            x0, y0, x1, y1,
            fill=color,
            outline="gray20"
        )

    def draw_text(self, x_px, y_px, text,
                  where="game",
                  anchor="nw",
                  font=None):
        """
        Dibuja texto simple en coordenadas de píxeles (x_px, y_px).

        where:
            - "game": dibuja en el área de juego.
            - "info": dibuja en el panel de info (si existe; si no, cae al área de juego).
        anchor:
            - igual que en Tkinter: "nw", "n", "center", etc.
        font:
            - objeto tkFont.Font o None para usar la fuente por defecto.
        """
        if where == "info" and self.info_canvas is not None:
            canvas = self.info_canvas
        else:
            canvas = self.game_canvas

        canvas.create_text(
            x_px, y_px,
            fill="white",
            text=text,
            anchor=anchor,
            font=font
        )

    def draw_hline(self, y_px, where="game"):
        """
        Dibuja una línea horizontal decorativa dentro del canvas indicado.
        """
        if where == "info" and self.info_canvas is not None:
            canvas = self.info_canvas
            width_px = self.info_width_px
        else:
            canvas = self.game_canvas
            width_px = self.game_width_px

        canvas.create_line(
            10, y_px,
            width_px - 10, y_px,
            fill="gray40"
        )

    # ------------------------------------------------------------------
    # Internos del loop y teclado
    # ------------------------------------------------------------------

    def _on_key_press(self, event):
        """
        Callback de Tkinter: se llama cuando se presiona una tecla.
        Pasa event.keysym al juego (por ejemplo 'Left', 'Right', 'Up',
        'Down', 'space', 'p', etc.).
        """
        if self._game is not None and hasattr(self._game, "on_key"):
            self._game.on_key(event.keysym)

    def _loop(self):
        """
        Un paso del game loop:
            - Calcula dt (milisegundos desde el frame anterior)
            - Llama game.update(dt)
            - Limpia pantalla
            - Llama game.draw(engine)
            - Programa el siguiente tick con root.after
        """
        if not self._running:
            return

        now_ms = int(round(time.time() * 1000))
        dt_ms = now_ms - self._last_time_ms
        self._last_time_ms = now_ms

        if self._game is not None:
            # Lógica
            if hasattr(self._game, "update"):
                self._game.update(dt_ms)

            # Render
            self.clear()
            if hasattr(self._game, "draw"):
                self._game.draw(self)

        # Agenda el siguiente frame
        self.root.after(self.tick_ms, self._loop)


# ----------------------------------------------------------------------
# Demo mínima de Entrega 2:
#   - Un ladrillo que se mueve con las flechas
#   - Panel de info a la derecha con título, puntaje, fecha/hora, etc.
# ----------------------------------------------------------------------

class MovingBrickDemo(object):
    """
    Demo mínima:
     - Un solo ladrillo
     - Se mueve con las flechas
     - No se sale de la grilla
     - Muestra información en el panel derecho (sin fecha/hora)
    """

    def __init__(self, engine):
        self.engine = engine

        # Lo ponemos en el centro de la grilla.
        self.x = engine.grid_width // 2
        self.y = engine.grid_height // 2

        self.color = "#00ff00"  # Verde
        self.move_count = 0     # Para el "puntaje" y stats

    def on_key(self, keysym):
        """
        keysym: string como 'Left', 'Right', 'Up', 'Down', 'space', etc.
        """
        moved = False

        if keysym == "Left":
            self.x -= 1
            moved = True
        elif keysym == "Right":
            self.x += 1
            moved = True
        elif keysym == "Up":
            self.y -= 1
            moved = True
        elif keysym == "Down":
            self.y += 1
            moved = True

        # Mantener el ladrillo dentro de la grilla
        if self.x < 0:
            self.x = 0
        if self.x >= self.engine.grid_width:
            self.x = self.engine.grid_width - 1
        if self.y < 0:
            self.y = 0
        if self.y >= self.engine.grid_height:
            self.y = self.engine.grid_height - 1

        if moved:
            self.move_count += 1

    def update(self, dt_ms):
        """
        dt_ms: milisegundos desde el último frame.
        En esta demo no usamos dt, pero en Snake/Tetris sí será útil.
        """
        pass

    def draw(self, engine):
        """
        Dibuja el ladrillo en el área de juego y
        un panel de información a la derecha.
        """
        # ---------------- Área de juego ----------------
        engine.draw_brick(self.x, self.y, self.color)
        engine.draw_text(
            10, 10,
            "Usa las flechas para mover el ladrillo",
            where="game",
            anchor="nw",
            font=engine.font_hint
        )

        # ---------------- Panel de info ----------------
        # Título centrado
        if engine.info_width_px > 0 and engine.info_canvas is not None:
            center_x = engine.info_width_px // 2

            engine.draw_text(
                center_x, 30,
                "Brick Game Engine",
                where="info",
                anchor="n",
                font=engine.font_title
            )

            engine.draw_hline(70, where="info")

            # Puntaje (por ejemplo, 10 puntos por movimiento)
            score = self.move_count * 10

            engine.draw_text(
                20, 90,
                "Puntaje:",
                where="info",
                anchor="nw",
                font=engine.font_label
            )
            engine.draw_text(
                20, 110,
                str(score),
                where="info",
                anchor="nw",
                font=engine.font_value
            )

            # Número de movimientos
            engine.draw_text(
                20, 140,
                "Movimientos:",
                where="info",
                anchor="nw",
                font=engine.font_label
            )
            engine.draw_text(
                20, 160,
                str(self.move_count),
                where="info",
                anchor="nw",
                font=engine.font_value
            )

            engine.draw_hline(200, where="info")

            # Info técnica del motor (queda muy "docente" pero útil)
            engine.draw_text(
                20, 220,
                "Tick: %d ms" % engine.tick_ms,
                where="info",
                anchor="nw",
                font=engine.font_hint
            )
            engine.draw_text(
                20, 240,
                "Grilla: %dx%d celdas" % (engine.grid_width, engine.grid_height),
                where="info",
                anchor="nw",
                font=engine.font_hint
            )

# ----------------------------------------------------------------------
# Punto de entrada: corre la demo si ejecutas este archivo directamente
# ----------------------------------------------------------------------

if __name__ == "__main__":
    engine = GameEngine(
        game_width_px=640,
        height_px=480,
        cell_size=20,   # 32x24 celdas en el área de juego
        tick_ms=50,     # ~20 FPS
        info_width_px=360   # Panel de info a la derecha
    )

    demo = MovingBrickDemo(engine)
    engine.set_game(demo)
    engine.start()
