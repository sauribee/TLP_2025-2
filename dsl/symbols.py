# -*- coding: utf-8 -*-
from __future__ import print_function


def build_symbol_table(ast):
    """
    Aplana el diccionario anidado (AST) en una tabla de símbolos:
    - claves tipo "board.width", "controls.right_mov"
    - valores: números, strings, bools, listas, etc.
    """
    table = {}

    def visit(prefix, node):
        # Si es un diccionario, seguimos bajando
        if isinstance(node, dict):
            for key, value in node.items():
                visit(prefix + [key], value)
        else:
            # Hoja: construimos el nombre con puntos
            name = '.'.join(prefix)
            table[name] = node

    visit([], ast)
    return table
