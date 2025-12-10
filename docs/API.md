# 🔧 API del Motor de Juegos

Documentación completa de la API del **Brick Game Engine** para desarrolladores que deseen crear sus propios juegos.

---

## 📖 Índice

1. [GameEngine](#gameengine)
2. [BaseGame](#basegame)
3. [Runtime](#runtime)
4. [Helpers de símbolos](#helpers-de-símbolos)

---

## GameEngine

La clase principal del motor gráfico que maneja el render, input y game loop.

### Constructor

```python
GameEngine(game_width_px=640, height_px=480, cell_size=20, tick_ms=50, info_width_px=360)
```

**Parámetros**:
- `game_width_px` (int): Ancho del área de juego en píxeles
- `height_px` (int): Alto total de la ventana en píxeles
- `cell_size` (int): Tamaño de cada celda en píxeles
- `tick_ms` (int): Milisegundos entre cada frame del game loop
- `info_width_px` (int): Ancho del panel de información lateral

**Atributos públicos**:
- `grid_width` (int): Ancho del tablero en celdas
- `grid_height` (int): Alto del tablero en celdas
- `root` (Tk): Ventana principal de Tkinter
- `game_canvas` (Canvas): Canvas del área de juego
- `info_canvas` (Canvas): Canvas del panel de información
- `font_title`, `font_label`, `font_value`, `font_hint` (Font): Tipografías disponibles

---

### Métodos principales

#### `set_game(game)`

Asigna el juego actual que el motor ejecutará.

**Parámetros**:
- `game` (BaseGame): Instancia del juego (debe implementar `on_key`, `update`, `draw`)

**Ejemplo**:
```python
engine = GameEngine()
game = SnakeGame(engine, symbols)
engine.set_game(game)
```

---

#### `start()`

Inicia el game loop y muestra la ventana. **Bloqueante** (entra al mainloop de Tkinter).

**Ejemplo**:
```python
engine.start()  # La aplicación corre aquí hasta que se cierre
```

---

#### `stop()`

Detiene el motor y cierra la ventana.

**Ejemplo**:
```python
engine.stop()
```

---

### Métodos de dibujo

#### `clear()`

Limpia todo el contenido dibujado en el frame anterior (tanto área de juego como panel de info).

**Ejemplo**:
```python
def draw(self, engine):
    engine.clear()  # Siempre limpiar primero
    # ... dibujar nuevo frame
```

---

#### `draw_brick(grid_x, grid_y, color="#00ff00")`

Dibuja un ladrillo (celda cuadrada) en coordenadas de grilla.

**Parámetros**:
- `grid_x` (int): Coordenada X en la grilla (0 a `grid_width - 1`)
- `grid_y` (int): Coordenada Y en la grilla (0 a `grid_height - 1`)
- `color` (str): Color en formato hexadecimal (`"#RRGGBB"`)

**Ejemplo**:
```python
# Dibuja un ladrillo rojo en la posición (5, 10)
engine.draw_brick(5, 10, "#FF0000")

# Dibuja ladrillo verde en el centro
center_x = engine.grid_width // 2
center_y = engine.grid_height // 2
engine.draw_brick(center_x, center_y, "#00FF00")
```

---

#### `draw_text(x_px, y_px, text, where="game", anchor="nw", font=None)`

Dibuja texto en coordenadas de píxeles.

**Parámetros**:
- `x_px` (int): Coordenada X en píxeles
- `y_px` (int): Coordenada Y en píxeles
- `text` (str): Texto a mostrar
- `where` (str): Dónde dibujar: `"game"` (área de juego) o `"info"` (panel lateral)
- `anchor` (str): Anclaje del texto (Tkinter): `"nw"`, `"n"`, `"center"`, `"e"`, etc.
- `font` (Font): Fuente de Tkinter o `None` para usar la predeterminada

**Ejemplo**:
```python
# Texto en el área de juego
engine.draw_text(10, 10, "Score: 100", where="game", anchor="nw")

# Título centrado en panel de info
center_x = engine.info_width_px // 2
engine.draw_text(
    center_x, 30,
    "Mi Juego",
    where="info",
    anchor="n",
    font=engine.font_title
)
```

---

#### `draw_hline(y_px, where="game")`

Dibuja una línea horizontal decorativa.

**Parámetros**:
- `y_px` (int): Coordenada Y en píxeles donde dibujar la línea
- `where` (str): `"game"` o `"info"`

**Ejemplo**:
```python
# Línea separadora en el panel de info
engine.draw_hline(100, where="info")
```

---

## BaseGame

Clase base abstracta que todo juego debe heredar.

### Estructura mínima

```python
from games.base_game import BaseGame

class MiJuego(BaseGame):
    def __init__(self, engine, symbols):
        super(MiJuego, self).__init__(engine, symbols)
        # Tu inicialización aquí
        self.x = 0
        self.y = 0
    
    def on_key(self, keysym):
        """Maneja eventos de teclado"""
        if keysym == "Left":
            self.x -= 1
        elif keysym == "Right":
            self.x += 1
    
    def update(self, dt_ms):
        """Actualiza lógica del juego cada frame"""
        # dt_ms = milisegundos desde el último frame
        pass
    
    def draw(self, engine):
        """Dibuja el estado actual del juego"""
        engine.draw_brick(self.x, self.y, "#FF0000")
```

---

### Constructor

```python
def __init__(self, engine, symbols):
    super(MiJuego, self).__init__(engine, symbols)
```

**Parámetros**:
- `engine` (GameEngine): Referencia al motor gráfico
- `symbols` (dict): Tabla de símbolos del archivo `.brik`

**Atributos heredados**:
- `self.engine`: Acceso al motor
- `self.symbols`: Acceso a la configuración del `.brik`

---

### Métodos requeridos

#### `on_key(keysym)`

Se llama cuando el usuario presiona una tecla.

**Parámetros**:
- `keysym` (str): Nombre de la tecla presionada

**Teclas comunes**:
- Flechas: `"Left"`, `"Right"`, `"Up"`, `"Down"`
- Letras: `"a"`, `"b"`, ..., `"z"` (minúsculas)
- Especiales: `"space"`, `"Return"`, `"Escape"`
- Números: `"0"`, `"1"`, ..., `"9"`

**Ejemplo**:
```python
def on_key(self, keysym):
    if keysym == "Left":
        self.move_left()
    elif keysym == "Right":
        self.move_right()
    elif keysym == "p":
        self.toggle_pause()
```

---

#### `update(dt_ms)`

Se llama en cada tick del game loop. Actualiza la lógica del juego.

**Parámetros**:
- `dt_ms` (int): Milisegundos transcurridos desde el último frame

**Ejemplo**:
```python
def update(self, dt_ms):
    # Acumular tiempo
    self.elapsed_ms += dt_ms
    
    # Actualizar cada 200ms
    if self.elapsed_ms >= 200:
        self.elapsed_ms -= 200
        self.move_snake()
        self.check_collisions()
```

---

#### `draw(engine)`

Se llama después de `update()` para dibujar el frame actual.

**Parámetros**:
- `engine` (GameEngine): Referencia al motor (para llamar métodos de dibujo)

**Ejemplo**:
```python
def draw(self, engine):
    # Dibujar fondo/grilla
    for x in range(engine.grid_width):
        for y in range(engine.grid_height):
            if self.is_wall(x, y):
                engine.draw_brick(x, y, "#555555")
    
    # Dibujar jugador
    engine.draw_brick(self.player_x, self.player_y, "#00FF00")
    
    # Dibujar UI
    engine.draw_text(20, 20, "Score: %d" % self.score, where="info")
```

---

## Runtime

Módulo que maneja la carga y parseo de archivos `.brik`.

### `load_symbols_from_brik(path)`

Carga un archivo `.brik` y retorna la tabla de símbolos como diccionario plano.

**Parámetros**:
- `path` (str): Ruta al archivo `.brik`

**Retorna**:
- `dict`: Tabla de símbolos con claves en formato `"seccion.subseccion.propiedad"`

**Lanza**:
- `BrikError`: Si hay errores de parseo o el archivo no existe

**Ejemplo**:
```python
from runtime import load_symbols_from_brik

symbols = load_symbols_from_brik("specs/snake.brik")
# symbols = {
#     "board.width": 20,
#     "board.height": 24,
#     "snake.tick_ms": 120,
#     ...
# }
```

---

## Helpers de símbolos

Funciones auxiliares para leer valores tipados desde la tabla de símbolos.

### `sym_get(symbols, key, default=None)`

Obtiene un valor de la tabla de símbolos o retorna el valor por defecto.

**Ejemplo**:
```python
from runtime import sym_get

width = sym_get(symbols, "board.width", 20)
```

---

### `sym_int(symbols, key, default=None)`

Obtiene un valor entero.

**Ejemplo**:
```python
from runtime import sym_int

tick_ms = sym_int(symbols, "snake.tick_ms", 100)
```

---

### `sym_float(symbols, key, default=None)`

Obtiene un valor flotante.

**Ejemplo**:
```python
from runtime import sym_float

speed = sym_float(symbols, "snake.speed_multiplier", 1.5)
```

---

### `sym_str(symbols, key, default=None)`

Obtiene un valor como string.

**Ejemplo**:
```python
from runtime import sym_str

spawn_mode = sym_str(symbols, "snake.spawn", "random")
```

---

### `sym_bool(symbols, key, default=False)`

Obtiene un valor booleano. Interpreta:
- `0`, `"0"`, `"false"`, `"False"`, `""` → `False`
- Cualquier otro valor → `True`

**Ejemplo**:
```python
from runtime import sym_bool

wrap_enabled = sym_bool(symbols, "board.wrap", False)
random_portals = sym_bool(symbols, "portals.random", True)
```

---

## Ejemplo completo: Juego simple

```python
# -*- coding: utf-8 -*-
from games.base_game import BaseGame
from runtime import sym_int, sym_str

class SimpleGame(BaseGame):
    def __init__(self, engine, symbols):
        super(SimpleGame, self).__init__(engine, symbols)
        
        # Leer configuración del .brik
        self.player_color = sym_str(symbols, "player.color", "#00FF00")
        self.speed = sym_int(symbols, "player.speed", 1)
        
        # Estado inicial
        self.x = engine.grid_width // 2
        self.y = engine.grid_height // 2
        self.score = 0
    
    def on_key(self, keysym):
        if keysym == "Left":
            self.x = max(0, self.x - self.speed)
        elif keysym == "Right":
            self.x = min(self.engine.grid_width - 1, self.x + self.speed)
        elif keysym == "Up":
            self.y = max(0, self.y - self.speed)
        elif keysym == "Down":
            self.y = min(self.engine.grid_height - 1, self.y + self.speed)
    
    def update(self, dt_ms):
        # Lógica del juego
        self.score += 1
    
    def draw(self, engine):
        # Dibujar jugador
        engine.draw_brick(self.x, self.y, self.player_color)
        
        # Dibujar UI
        engine.draw_text(20, 20, "Score: %d" % self.score, where="info")
        engine.draw_text(20, 50, "Pos: (%d, %d)" % (self.x, self.y), where="info")
```

**Archivo .brik correspondiente**:

```brik
game "simple_game" {
    board {
        width = 20;
        height = 20;
        cell_size = 20;
    }
    
    player {
        color = "#FF6600";
        speed = 2;
    }
}
```

**main.py**:

```python
from runtime import load_symbols_from_brik
from engine import GameEngine
from games.simple_game import SimpleGame

symbols = load_symbols_from_brik("specs/simple_game.brik")
engine = GameEngine(game_width_px=400, height_px=400, cell_size=20)
game = SimpleGame(engine, symbols)
engine.set_game(game)
engine.start()
```

---

## Mejores prácticas

### 1. Separación de responsabilidades

```python
class MyGame(BaseGame):
    def update(self, dt_ms):
        self._update_physics(dt_ms)
        self._check_collisions()
        self._spawn_items()
    
    def draw(self, engine):
        self._draw_world(engine)
        self._draw_entities(engine)
        self._draw_ui(engine)
```

### 2. Usar constantes del .brik

```python
# Mal: valores hardcodeados
self.speed = 5

# Bien: leer del .brik
self.speed = sym_int(self.symbols, "player.speed", 5)
```

### 3. Validar límites al dibujar

```python
def draw_brick_safe(self, engine, x, y, color):
    if 0 <= x < engine.grid_width and 0 <= y < engine.grid_height:
        engine.draw_brick(x, y, color)
```

### 4. Gestión de tiempo

```python
def __init__(self, engine, symbols):
    super(MyGame, self).__init__(engine, symbols)
    self.accumulator = 0
    self.tick_ms = sym_int(symbols, "game.tick_ms", 100)

def update(self, dt_ms):
    self.accumulator += dt_ms
    
    while self.accumulator >= self.tick_ms:
        self.accumulator -= self.tick_ms
        self.game_tick()  # Lógica que debe correr a velocidad fija
```

---

## Debugging

### Imprimir tabla de símbolos

```python
symbols = load_symbols_from_brik("specs/mi_juego.brik")
import pprint
pprint.pprint(symbols)
```

### Dibujar grilla de debug

```python
def draw(self, engine):
    # Dibujar grilla
    for x in range(engine.grid_width):
        for y in range(engine.grid_height):
            engine.draw_brick(x, y, "#111111")
    
    # Tu juego aquí...
```

### Mostrar FPS

```python
def __init__(self, engine, symbols):
    super(MyGame, self).__init__(engine, symbols)
    self.frame_count = 0
    self.fps_timer = 0

def update(self, dt_ms):
    self.frame_count += 1
    self.fps_timer += dt_ms
    
    if self.fps_timer >= 1000:  # Cada segundo
        print("FPS:", self.frame_count)
        self.frame_count = 0
        self.fps_timer = 0
```

---

## Limitaciones conocidas

1. **Python 2.7**: El código usa `print_function` y sintaxis compatible con Python 2.7
2. **Tkinter**: No soporta transparencias ni rotación de sprites
3. **Performance**: Dibujar >1000 ladrillos por frame puede causar lag
4. **Input**: Solo detecta key press, no key hold (mantener presionada)

---

Para más ejemplos, revisa los archivos `games/snake_game.py` y `games/tetris_game.py`.
