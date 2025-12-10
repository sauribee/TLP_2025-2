=============================================================
    PROYECTO: Brick Game Engine - TLP_2025-2
=============================================================

Brick Game Engine es un motor de juegos retro estilo Game Boy desarrollado
en Python 2.7 con un DSL (Lenguaje de Dominio Especifico) propio llamado
".brik" disenado para configurar juegos clasicos como Snake y Tetris sin
modificar codigo Python.

Este proyecto incluye:
- Un compilador que traduce archivos .brik a formato JSON
- Un motor grafico basado en Tkinter
- Implementaciones completas de Snake y Tetris con mecanicas especiales

-------------------------------------------------------------
                      COMO JUGAR
-------------------------------------------------------------

Para ejecutar los juegos, hemos creado un script RUN_BRICK_GAMES.bat, para facilitar la ejecución.

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
  P : Pausar el juego
  R : Reiniciar el juego
  
  OBJETIVO: Come manzanas para crecer y obtener puntos. Evita chocar con
  las paredes o tu propio cuerpo. Usa los portales de colores para
  teletransportarte a traves del tablero.

  CARACTERISTICAS ESPECIALES:
  - Portales de teletransporte por pares (morado y amarillo)
  - Sistema de aceleracion progresiva
  - Panel de informacion muestra los portales activos


TETRIS
------
  Flecha izquierda/derecha : Mover la pieza
  Flecha arriba : Rotar la pieza
  Flecha abajo : Caida rapida (soft drop)
  P : Pausar el juego
  R : Reiniciar el juego
  
  OBJETIVO: Completa lineas horizontales para obtener puntos. Cuantas mas
  lineas completes simultaneamente, mas puntos obtendras.

  CARACTERISTICAS ESPECIALES:
  - Piezas bomba 1x1 (gris claro): Explotan en area 3x3, otorgan 90 puntos
  - Piezas bomba 2x2 (gris medio): Explotan en area 4x4, otorgan 160 puntos
  - Las bombas aparecen aleatoriamente con 12.5% de probabilidad
  - Sistema de combos y puntuacion con bonus


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
