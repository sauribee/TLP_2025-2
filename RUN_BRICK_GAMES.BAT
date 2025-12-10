@echo off
REM -------------------------------------------------------
REM  Lanzador Brick Game (Python 2.7)
REM  Vuelve al menu al cerrar el juego
REM -------------------------------------------------------

REM Ir a la carpeta donde esta este .BAT
cd /d "%~dp0"

REM Ruta de Python 2.7 (ajustar si es necesario)
set PYTHON=C:\Python27\python.exe

REM Verificar que exista python.exe en esa ruta
if not exist "%PYTHON%" (
    echo.
    echo No se encontro Python 2.7 en:
    echo     %PYTHON%
    echo.
    echo Edita RUN_GAME.BAT y cambia la variable PYTHON
    echo por la ruta correcta a python.exe en este PC.
    echo.
    pause
    goto :EOF
)

:menu
cls
echo ==========================================
echo        BRICK GAMES ENGINE - TLP 2025-2
echo ==========================================
echo.
echo  [1] Jugar SNAKE
echo  [2] Jugar TETRIS
echo  [3] Salir
echo.
set "CHOICE="
set /p CHOICE=Selecciona una opcion [1-3]: 

if "%CHOICE%"=="1" goto snake
if "%CHOICE%"=="2" goto tetris
if "%CHOICE%"=="3" goto end
goto menu

:snake
echo.
echo Ejecutando Snake...
"%PYTHON%" main.py snake
REM Cuando cierras la ventana de Snake, Python termina y volvemos al menu
goto menu

:tetris
echo.
echo Ejecutando Tetris...
"%PYTHON%" main.py tetris
REM Cuando cierras la ventana de Tetris, Python termina y volvemos al menu
goto menu

:end
echo.
echo Fin del programa.
pause
