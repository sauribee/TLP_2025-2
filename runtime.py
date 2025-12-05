# runtime.py
# -*- coding: utf-8 -*-

import os

from dsl.brik_parser import parse_file
from dsl.symbols import build_symbol_table


class BrikError(Exception):
    """Errores relacionados con la carga/analizador de archivos .brik."""
    pass


def load_symbols_from_brik(path):
    """
    Carga un archivo .brik, lo analiza y devuelve la tabla de símbolos
    como un diccionario plano: { "board.width": 10, "snake.tick_ms": 200, ... }.
    Lanza BrikError si hay problemas.
    """
    if not os.path.exists(path):
        raise BrikError("Archivo .brik no encontrado: %s" % path)

    # Ya NO leemos el archivo aquí; eso lo hace parse_file internamente.
    try:
        ast = parse_file(path)   # ✅ aquí va la RUTA, no el texto
    except Exception as e:
        raise BrikError("Error de parseo en %s: %s" % (path, e))

    try:
        symbols = build_symbol_table(ast)
    except Exception as e:
        raise BrikError("Error construyendo tabla de símbolos: %s" % e)

    return symbols

# ----------------------------------------------------------------------
# Helpers para leer valores tipados desde la tabla de símbolos
# ----------------------------------------------------------------------

def sym_get(symbols, key, default=None):
    """Obtiene un símbolo o devuelve un valor por defecto."""
    if key in symbols:
        return symbols[key]
    return default


def sym_int(symbols, key, default=None):
    v = sym_get(symbols, key, default)
    if v is None:
        return None
    return int(v)


def sym_float(symbols, key, default=None):
    v = sym_get(symbols, key, default)
    if v is None:
        return None
    return float(v)


def sym_str(symbols, key, default=None):
    v = sym_get(symbols, key, default)
    if v is None:
        return None
    return str(v)


def sym_bool(symbols, key, default=False):
    """
    Interpreta un símbolo como booleano:
    - 0, "0", "", "false", "False" -> False
    - cualquier otra cosa -> True
    """
    v = sym_get(symbols, key, None)
    if v is None:
        return bool(default)

    if isinstance(v, int):
        return v != 0

    s = str(v).strip().lower()
    if s in ("0", "false", "no", ""):
        return False
    return True
