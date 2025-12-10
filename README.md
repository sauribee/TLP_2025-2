# 🎮 Brick Game Engine - TLP 2025-2

Motor de juegos retro estilo **Game Boy** desarrollado en **Python 2.7** con un **DSL (Domain Specific Language)** propio para configurar juegos sin modificar código.

---

## 🌟 Características

- 🐍 **Snake** con portales de teletransporte coloreados y sistema de aceleración progresiva
  - Portales con colores unificados por par (morado, amarillo, cian)
  - Panel de información que muestra los pares de portales activos
  - Restricción de portales: no aparecen en los bordes del tablero
- 🧱 **Tetris** clásico con piezas bomba especiales y sistema de combos
  - Bomba 1x1 (gris claro): explota en área 3×3 centrada en la bomba
  - Bomba 2x2 (gris medio): explota en área 4×4 con la bomba en el centro
  - Probabilidad configurable de aparición de bombas (12.5% por defecto)
  - Sistema de puntuación con bonus por uso de bombas
- 🔧 **DSL personalizado** (`.brik`) para configurar reglas, colores, controles y mecánicas
- 🎨 **Interfaz retro** con panel de información en tiempo real
- ⚡ **Motor gráfico** basado en Tkinter con sistema de grilla
- 🎯 **Arquitectura modular** que facilita agregar nuevos juegos
- 📦 **Compilador** (compiler.py) para generar archivos JSON desde archivos .brik
- 📝 **Gramáticas BNF** formales para Snake y Tetris

---

## 📦 Instalación

### Requisitos

- **Python 2.7** instalado en tu sistema
- **Tkinter** (incluido por defecto en instalaciones estándar de Python 2.7)

### Windows

1. Clona el repositorio:
   ```bash
   git clone https://github.com/sauribee/TLP_2025-2.git
   cd TLP_2025-2
   ```

2. Asegúrate de tener Python 2.7 instalado en `C:\Python27\` o edita `RUN_GAME.BAT` con tu ruta.

3. Ejecuta el launcher:
   ```bash
   RUN_GAME.BAT
   ```

### Linux / Mac

1. Clona el repositorio:
   ```bash
   git clone https://github.com/sauribee/TLP_2025-2.git
   cd TLP_2025-2
   ```

2. Ejecuta el juego directamente:
   ```bash
   python main.py snake   # Para jugar Snake
   python main.py tetris  # Para jugar Tetris
   ```

   O usa el script shell:
   ```bash
   chmod +x run.sh
   ./run.sh
   ```

---

## 🎮 Cómo jugar

### Snake 🐍

- **Flechas**: Mover la serpiente
- **P**: Pausar
- **R**: Reiniciar

**Objetivo**: Come manzanas, crece y evita chocar con paredes o tu propio cuerpo. ¡Usa los portales para teletransportarte!

**Características especiales**:
- 🟣 Portales morados (Par 1): conectan dos posiciones del tablero
- 🟡 Portales amarillos (Par 2): conectan dos posiciones diferentes
- 📊 Panel de información muestra los pares de portales activos

### Tetris 🧱

- **Flecha izquierda/derecha**: Mover pieza
- **Flecha arriba**: Rotar pieza
- **Flecha abajo**: Drop rápido (soft drop)
- **P**: Pausar
- **R**: Reiniciar

**Objetivo**: Completa líneas horizontales para obtener puntos. ¡Usa las bombas especiales estratégicamente!

**Piezas especiales - Bombas**:
- 💣 **Bomba 1x1** (gris claro #808080): Explota en área 3×3, otorga 90 puntos
- 💣💣 **Bomba 2x2** (gris medio #909090): Explota en área 4×4, otorga 160 puntos
- ⚡ Las bombas aparecen aleatoriamente con 12.5% de probabilidad (1 en 8 piezas aprox.)
- 🎯 Al aterrizar, la bomba borra todas las piezas en su área de explosión

---

## 📂 Estructura del Proyecto

```
TLP_2025-2/
├── dsl/                    # Domain Specific Language (.brik)
│   ├── lexer.py           # Tokenizador del lenguaje
│   ├── brik_parser.py     # Parser y generador de AST
│   └── symbols.py         # Generador de tabla de símbolos
├── games/                  # Implementación de juegos
│   ├── base_game.py       # Clase base abstracta
│   ├── snake_game.py      # Lógica completa de Snake (con portales)
│   └── tetris_game.py     # Lógica completa de Tetris (con bombas)
├── specs/                  # Configuraciones .brik y compiladas
│   ├── snake.brik         # Configuración de Snake (con comentarios)
│   ├── snake.json         # Snake compilado a JSON
│   ├── tetris.brik        # Configuración de Tetris (con comentarios)
│   └── tetris.json        # Tetris compilado a JSON
├── bnf/                    # Gramáticas formales BNF
│   ├── snake.bnf          # Gramática BNF de Snake DSL
│   └── tetris.bnf         # Gramática BNF de Tetris DSL
├── docs/                   # Documentación técnica
│   ├── DSL_REFERENCE.md   # Referencia completa del lenguaje .brik
│   └── API.md             # API del motor de juegos
├── screenshots/            # Capturas de pantalla
├── engine.py              # Motor gráfico principal (Tkinter)
├── runtime.py             # Cargador de archivos .brik en tiempo de ejecución
├── compiler.py            # Compilador .brik → .json
├── main.py                # Punto de entrada del programa
├── RUN_GAME.BAT           # Launcher para Windows
├── run.sh                 # Launcher para Linux/Mac
└── README.md              # Este archivo
```

---

## 🔧 El lenguaje .brik

Los archivos `.brik` permiten configurar **completamente** un juego sin tocar código Python. Ejemplo:

```brik
snake version 1.0

game "snake" {
    board {
        width = 20;
        height = 24;
        cell_size = 20;
        colors {
            background = #FFFFFF;
            snake_body = #2C8D3E;
            apple      = #F74848;
        }
    }
    
    snake {
        tick_ms = 120;
        initial_length = 1;
        growth_per_apple = 3;
    }
    
    controls {
        right_mov = "right";
        left_mov  = "left";
        pause     = "p";
    }
}
```

Para más detalles, consulta la [Referencia del DSL](docs/DSL_REFERENCE.md).

---

## 🚀 Agregar tu propio juego

1. Crea una clase que herede de `BaseGame` en `games/`:

```python
from games.base_game import BaseGame

class MiJuego(BaseGame):
    def __init__(self, engine, symbols):
        super(MiJuego, self).__init__(engine, symbols)
        # Tu inicialización aquí
    
    def on_key(self, keysym):
        # Manejar teclas
        pass
    
    def update(self, dt_ms):
        # Actualizar lógica del juego
        pass
    
    def draw(self, engine):
        # Dibujar en pantalla
        engine.draw_brick(5, 5, "#FF0000")
```

2. Crea un archivo `.brik` en `specs/` con tu configuración.

3. Agrega tu juego en `main.py`:

```python
from games.mi_juego import MiJuego

# En la función choose_game()
elif choice == "mijuego":
    brik_path = "specs/mijuego.brik"
    GameClass = MiJuego
```

---

## 🧪 Testing

Para probar el parser del DSL:

```bash
python dsl/brik_parser.py specs/snake.brik
```

Para probar el motor gráfico:

```bash
python engine.py
```

---

## 📚 Documentación adicional

- [**Referencia del DSL**](docs/DSL_REFERENCE.md): Sintaxis completa del lenguaje `.brik`
- [**API del Motor**](docs/API.md): Métodos disponibles para desarrolladores
- [**Changelog**](CHANGELOG.md): Historial de versiones

---

## 🤝 Contribuciones

Este proyecto fue desarrollado como parte del curso **Teoría de Lenguajes de Programación (TLP)** 2025-2.

### Equipo

- **Desarrolladores**: [Nombres del equipo]
- **Profesor**: [Nombre del profesor]
- **Universidad**: [Nombre de la universidad]

---

## 📄 Licencia

Este proyecto está bajo la licencia MIT. Ver [LICENSE](LICENSE) para más detalles.

---

## 🐛 Problemas conocidos

- Python 2.7 es legacy (EOL 2020). Se recomienda migrar a Python 3.x en futuras versiones.
- Tkinter puede tener issues de performance en tableros muy grandes (>50x50).

---

## 🎯 Roadmap

- [ ] Migración a Python 3.x
- [ ] Soporte para más juegos (Pong, Space Invaders)
- [ ] Editor visual de archivos `.brik`
- [ ] Sistema de high scores persistente
- [ ] Sonido y efectos de audio

---

## 📧 Contacto

Para preguntas o sugerencias, abre un issue en GitHub o contacta al equipo del proyecto.

**¡Disfruta jugando!** 🎮
