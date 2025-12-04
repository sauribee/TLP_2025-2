# -*- coding: utf-8 -*-
from __future__ import print_function

class Token(object):
    __slots__ = ('type', 'value', 'line', 'col')

    def __init__(self, t, v, line, col):
        self.type = t      # 'IDENT', 'NUMBER', 'STRING', 'COLOR', '{', '}', '[', ']', '=', ';', ',', 'EOF'
        self.value = v     # texto original (sin comillas para STRING)
        self.line = line   # número de línea (1-based)
        self.col = col     # número de columna (1-based)

    def __repr__(self):
        return "Token(%r, %r, %d, %d)" % (self.type, self.value, self.line, self.col)


class LexError(Exception):
    pass


def tokenize(text):
    """
    Convierte el contenido de un archivo .brik en una lista de tokens.

    No usa regex ni librerías externas para mantener el código pequeño
    y fácilmente portable a Python 2.7.
    """
    tokens = []
    i = 0
    line = 1
    col = 1
    n = len(text)

    while i < n:
        ch = text[i]

        # Espacios en blanco (excepto newline)
        if ch in ' \t\r':
            i += 1
            col += 1
            continue

        # Nueva línea
        if ch == '\n':
            i += 1
            line += 1
            col = 1
            continue

        # Comentarios de línea: // ... (opcional, por si los quieres usar)
        if ch == '/' and i + 1 < n and text[i + 1] == '/':
            i += 2
            col += 2
            while i < n and text[i] != '\n':
                i += 1
                col += 1
            continue

        # Puntuación simple
        if ch in '{}[]=;,':
            tokens.append(Token(ch, ch, line, col))
            i += 1
            col += 1
            continue

        # String literal: "..."
        if ch == '"':
            start_col = col
            i += 1
            col += 1
            start = i
            # Leemos hasta la siguiente comilla
            while i < n and text[i] != '"':
                if text[i] == '\n':
                    raise LexError("Unterminated string at line %d" % line)
                i += 1
                col += 1
            if i >= n:
                raise LexError("Unterminated string at line %d" % line)
            value = text[start:i]   # sin las comillas
            i += 1
            col += 1
            tokens.append(Token('STRING', value, line, start_col))
            continue

        # Color hexadecimal: #RRGGBB
        if ch == '#':
            start_col = col
            j = i + 1
            hex_digits = '0123456789abcdefABCDEF'
            while j < n and text[j] in hex_digits:
                j += 1
            if j == i + 1:
                raise LexError("Invalid color literal at line %d, col %d" % (line, col))
            value = text[i:j]  # incluye el '#'
            tokens.append(Token('COLOR', value, line, start_col))
            col += (j - i)
            i = j
            continue

        # Número (int o float)
        if ch.isdigit():
            start_col = col
            j = i
            has_dot = False
            while j < n and (text[j].isdigit() or (text[j] == '.' and not has_dot)):
                if text[j] == '.':
                    has_dot = True
                j += 1
            value = text[i:j]
            tokens.append(Token('NUMBER', value, line, start_col))
            col += (j - i)
            i = j
            continue

        # Identificador (letra o '_' seguido de letras/dígitos/_)
        if ch.isalpha() or ch == '_':
            start_col = col
            j = i + 1
            while j < n and (text[j].isalnum() or text[j] == '_'):
                j += 1
            value = text[i:j]
            tokens.append(Token('IDENT', value, line, start_col))
            col += (j - i)
            i = j
            continue

        # Cualquier otro carácter se considera error
        raise LexError("Unexpected character %r at line %d, col %d"
                       % (ch, line, col))

    # Token de fin de archivo
    tokens.append(Token('EOF', '', line, col))
    return tokens


if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
        print("Usage: python lexer.py <file.brik>")
        sys.exit(1)
    with open(sys.argv[1], 'r') as f:
        text = f.read()
    try:
        for tok in tokenize(text):
            print(tok)
    except LexError as e:
        print("Lex error:", e)
        sys.exit(1)
