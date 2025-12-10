# -*- coding: utf-8 -*-

from __future__ import print_function

import sys
import lexer
import brik_parser
import symbols


def main(argv):
    if not argv:
        print("Uso: python2 brik.py <archivo.brik> [--debug]")
        sys.exit(1)

    debug = False
    path = None

    # Parseo de argumentos súper simple para no meter más código
    for arg in argv:
        if arg == '--debug':
            debug = True
        elif path is None:
            path = arg
        else:
            print("Argumento no reconocido:", arg)
            print("Uso: python2 brik.py <archivo.brik> [--debug]")
            sys.exit(1)

    if path is None:
        print("Uso: python2 brik.py <archivo.brik> [--debug]")
        sys.exit(1)

    # Leemos el archivo
    try:
        f = open(path, 'r')
        try:
            text = f.read()
        finally:
            f.close()
    except IOError as e:
        print("ERROR: no se pudo abrir el archivo:", e)
        sys.exit(1)

    # Ejecutamos lexer + parser + tabla de símbolos
    try:
        # Si quieres, podrías llamar directamente parser.parse_file(path),
        # pero aquí mostramos explícitamente el flujo:
        tokens = lexer.tokenize(text)
        p = brik_parser.Parser(tokens)
        ast = p.parse_file()
        symtab = symbols.build_symbol_table(ast)
    except (lexer.LexError, brik_parser.ParseError) as e:
        print("ERROR de análisis:", e)
        sys.exit(1)

    # Si llegó hasta aquí, el archivo es válido
    print("OK:", path, "es valido")

    if debug:
        print("\n--- AST (estructura interna) ---")
        import pprint
        pprint.pprint(ast)

        print("\n--- Tabla de símbolos (aplanada) ---")
        for name in sorted(symtab.keys()):
            print("%s = %r" % (name, symtab[name]))


if __name__ == '__main__':
    main(sys.argv[1:])
