=============================================================
     PROYECTO: Brick Game Engine - Motor de Juegos Retro
=============================================================

Brick Game Engine es un motor de juegos retro estilo Game Boy desarrollado
en Python 2.7 con un DSL (Lenguaje de Dominio Especifico) propio llamado
".brik" disenado para configurar juegos clasicos como Snake y Tetris sin
modificar codigo Python.

Este proyecto incluye:
- Un compilador que traduce archivos .brik a formato JSON
- Un motor grafico basado en Tkinter
- Implementaciones completas de Snake y Tetris con mecanicas especiales
- Documentacion tecnica completa del lenguaje .brik

-------------------------------------------------------------
                      COMO JUGAR
-------------------------------------------------------------

Para ejecutar los juegos, hemos creado un script que hace todo el trabajo
por ti.

METODO 1: USANDO EL LAUNCHER (RECOMENDADO)
-------------------------------------------

1. Haz doble clic en el archivo "RUN_BRICK_GAMES.BAT"

2. Aparecera un menu con las opciones:
   [1] Jugar SNAKE
   [2] Jugar TETRIS
   [3] Salir

3. Selecciona el numero del juego que deseas jugar y presiona Enter.

4. El juego se iniciara automaticamente.


METODO 2: DESDE LA LINEA DE COMANDOS
-------------------------------------

1. Abre una terminal de comandos (cmd.exe) en la carpeta principal del
   proyecto.

2. Ejecuta el juego directamente con Python:

   PARA JUGAR SNAKE:
   C:\Python27\python.exe main.py snake

   PARA JUGAR TETRIS:
   C:\Python27\python.exe main.py tetris


-------------------------------------------------------------
                    CONTROLES DE LOS JUEGOS
-------------------------------------------------------------

SNAKE
-----
  Flechas direccionales : Mover la serpiente
  P                     : Pausar el juego
  R                     : Reiniciar el juego
  
  OBJETIVO: Come manzanas para crecer y obtener puntos. Evita chocar con
  las paredes o tu propio cuerpo. Usa los portales de colores para
  teletransportarte a traves del tablero.

  CARACTERISTICAS ESPECIALES:
  - Portales de teletransporte por pares (morado, amarillo, cian)
  - Sistema de aceleracion progresiva
  - Panel de informacion muestra los portales activos


TETRIS
------
  Flecha izquierda/derecha : Mover la pieza
  Flecha arriba            : Rotar la pieza
  Flecha abajo             : Caida rapida (soft drop)
  P                        : Pausar el juego
  R                        : Reiniciar el juego
  
  OBJETIVO: Completa lineas horizontales para obtener puntos. Cuantas mas
  lineas completes simultaneamente, mas puntos obtendras.

  CARACTERISTICAS ESPECIALES:
  - Piezas bomba 1x1 (gris claro): Explotan en area 3x3, otorgan 90 puntos
  - Piezas bomba 2x2 (gris medio): Explotan en area 4x4, otorgan 160 puntos
  - Las bombas aparecen aleatoriamente con 12.5% de probabilidad
  - Sistema de combos y puntuacion con bonus


-------------------------------------------------------------
                 SINTAXIS DEL LENGUAJE .BRIK
-------------------------------------------------------------

El lenguaje .brik permite configurar completamente un juego sin modificar
codigo Python. Los archivos .brik se encuentran en la carpeta "specs/".

--- ESTRUCTURA BASICA ---

[tipo_juego] version [numero]

game "nombre_del_juego" {
    board {
        // Configuracion del tablero
    }
    controls {
        // Mapeo de teclas
    }
    // Secciones especificas del juego
}

--- TIPOS DE DATOS SOPORTADOS ---

* Numeros enteros y decimales: 42, 3.14, -10
* Cadenas de texto: "snake", "left", "top_center"
* Colores hexadecimales: #FF0000, #2C8D3E, #FFFFFF
* Booleanos: true, false
* Listas: [1, 2, 3], [#FF0000, #00FF00]
* Matrices: [[1,0,0],[0,1,0],[0,0,1]]
* Identificadores: piece_I, piece_O, piece_T

--- COMENTARIOS ---

// Comentario estilo C++
# Comentario estilo Python

--- SECCIONES COMUNES ---

board {
    width = 20;              // Ancho en celdas
    height = 24;             // Alto en celdas
    cell_size = 20;          // Tamano de cada celda en pixeles
    
    colors {
        background = #FFFFFF;
        grid = #CCCCCC;
    }
}

controls {
    right_mov = "right";     // Tecla flecha derecha
    left_mov = "left";       // Tecla flecha izquierda
    pause = "p";             // Tecla P para pausar
    restart = "r";           // Tecla R para reiniciar
}

--- CONFIGURACION DE SNAKE ---

snake {
    tick_ms = 120;           // Velocidad del juego (ms por frame)
    initial_length = 1;      // Longitud inicial de la serpiente
    growth_per_apple = 3;    // Cuanto crece al comer una manzana
}

portals {
    random = true;           // Portales aleatorios
    num_pairs = 2;           // Numero de pares de portales
}

rules_end_game {
    on_out_of_bounds = "end";    // Terminar al salir del tablero
    on_self_collision = "end";   // Terminar al chocar consigo mismo
}

--- CONFIGURACION DE TETRIS ---

tetris {
    tick_ms = 650;           // Velocidad de caida
    gravity = 1;             // Gravedad en celdas/tick
    spawn = "top_center";    // Posicion de aparicion
}

pieces {
    piece_I {
        color = "#75E689";
        rotations = [
            [[0,0,0,0],[1,1,1,1],[0,0,0,0],[0,0,0,0]],
            [[0,1,0,0],[0,1,0,0],[0,1,0,0],[0,1,0,0]]
        ];
    }
}

available_pieces = [piece_I, piece_O, piece_T, piece_J, piece_L, piece_S, piece_Z];

rules_random_pieces {
    random_bombs = true;
    bomb_chance = 0.125;     // 12.5% de probabilidad
}

Para documentacion completa del lenguaje, consulta: docs/DSL_REFERENCE.md


-------------------------------------------------------------
                    COMPILAR ARCHIVOS .BRIK
-------------------------------------------------------------

Los archivos .brik deben ser compilados a JSON antes de ejecutarse.
El sistema hace esto automaticamente al usar RUN_BRICK_GAMES.BAT, pero
tambien puedes compilar manualmente.

COMPILAR UN ARCHIVO ESPECIFICO:
C:\Python27\python.exe compiler.py specs/snake.brik

COMPILAR TODOS LOS ARCHIVOS EN specs/:
C:\Python27\python.exe compiler.py --all

El compilador genera archivos .json en la misma carpeta que los .brik.


-------------------------------------------------------------
                 ESTRUCTURA DEL PROYECTO
-------------------------------------------------------------

TLP_2025-2/
|
+--- main.py                (Punto de entrada del programa)
+--- compiler.py            (Compilador .brik a .json)
+--- runtime.py             (Cargador de archivos .brik)
+--- engine.py              (Motor grafico basado en Tkinter)
+--- RUN_BRICK_GAMES.BAT    (Launcher para Windows)
+--- README.txt             (Este archivo)
+--- INSTALL.txt            (Guia de instalacion)
+--- requirements.txt       (Dependencias del proyecto)
|
+--- dsl/                   (Implementacion del lenguaje .brik)
|    +--- lexer.py          (Tokenizador)
|    +--- brik_parser.py    (Parser y generador de AST)
|    +--- symbols.py        (Tabla de simbolos)
|    \--- brik.py           (CLI del analizador)
|
+--- games/                 (Implementacion de los juegos)
|    +--- base_game.py      (Clase base abstracta)
|    +--- snake_game.py     (Logica completa de Snake)
|    \--- tetris_game.py    (Logica completa de Tetris)
|
+--- specs/                 (Configuraciones .brik y compiladas)
|    +--- snake.brik        (Configuracion de Snake)
|    +--- snake.json        (Snake compilado)
|    +--- tetris.brik       (Configuracion de Tetris)
|    \--- tetris.json       (Tetris compilado)
|
+--- bnf/                   (Gramaticas formales BNF)
|    +--- snake.bnf         (Gramatica BNF de Snake)
|    \--- tetris.bnf        (Gramatica BNF de Tetris)
|
+--- docs/                  (Documentacion tecnica)
|    +--- DSL_REFERENCE.md  (Referencia completa del lenguaje)
|    \--- API.md            (API del motor de juegos)
|
\--- screenshots/           (Capturas de pantalla)


-------------------------------------------------------------
              COMO CREAR TU PROPIO JUEGO
-------------------------------------------------------------

1. Crea un archivo .brik en la carpeta specs/ con tu configuracion.

2. Crea una clase de juego en games/ que herede de BaseGame:

   from games.base_game import BaseGame
   
   class MiJuego(BaseGame):
       def __init__(self, engine, symbols):
           super(MiJuego, self).__init__(engine, symbols)
       
       def on_key(self, keysym):
           # Manejar entrada de teclado
           pass
       
       def update(self, dt_ms):
           # Actualizar logica del juego
           pass
       
       def draw(self, engine):
           # Dibujar en pantalla
           engine.draw_brick(x, y, color)

3. Registra tu juego en main.py:

   from games.mi_juego import MiJuego
   
   # En la funcion choose_game()
   elif choice == "mijuego":
       brik_path = "specs/mijuego.brik"
       GameClass = MiJuego

4. Compila tu archivo .brik:
   C:\Python27\python.exe compiler.py specs/mijuego.brik

5. Ejecuta tu juego:
   C:\Python27\python.exe main.py mijuego


-------------------------------------------------------------
                         TESTING
-------------------------------------------------------------

PROBAR EL PARSER DEL DSL:
C:\Python27\python.exe dsl/brik.py specs/snake.brik

PROBAR EL PARSER CON DEBUG:
C:\Python27\python.exe dsl/brik.py specs/snake.brik --debug

PROBAR EL MOTOR GRAFICO:
C:\Python27\python.exe engine.py


-------------------------------------------------------------
                      DOCUMENTACION
-------------------------------------------------------------

- docs/DSL_REFERENCE.md : Sintaxis completa del lenguaje .brik
- docs/API.md          : Metodos disponibles para desarrolladores
- bnf/snake.bnf        : Gramatica formal BNF de Snake
- bnf/tetris.bnf       : Gramatica formal BNF de Tetris
- INSTALL.txt          : Guia detallada de instalacion


-------------------------------------------------------------
                    CREDITOS
-------------------------------------------------------------

Este proyecto fue desarrollado como parte del curso:
  Teoria de Lenguajes de Programacion (TLP) 2025-2

Equipo de desarrollo: [Nombres del equipo]
Profesor: [Nombre del profesor]
Universidad: [Nombre de la universidad]


-------------------------------------------------------------
                  PROBLEMAS CONOCIDOS
-------------------------------------------------------------

- Python 2.7 es legacy (EOL 2020). Se recomienda migracion a Python 3.x
  en futuras versiones.
- Tkinter puede tener problemas de rendimiento en tableros muy grandes
  (mayores a 50x50 celdas).


-------------------------------------------------------------
                       LICENCIA
-------------------------------------------------------------

Este proyecto esta bajo la licencia MIT.
Ver archivo LICENSE para mas detalles.


=============================================================
                    ¡Disfruta jugando!
=============================================================
