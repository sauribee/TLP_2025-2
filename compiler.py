# -*- coding: utf-8 -*-
"""
compiler.py

Compilador de archivos .brik a formato .json

Uso:
    python compiler.py specs/snake.brik
    python compiler.py specs/tetris.brik
    python compiler.py --all

Genera archivos .json en la misma carpeta que el .brik
"""
from __future__ import print_function

import sys
import os
import json

# Agregar el directorio actual al path para imports correctos
if os.path.dirname(__file__):
    sys.path.insert(0, os.path.dirname(__file__))

from dsl.brik_parser import parse_file
from dsl.symbols import build_symbol_table


def compile_brik_to_json(brik_path):
    """
    Compila un archivo .brik a .json
    
    Args:
        brik_path: Ruta al archivo .brik
    
    Returns:
        Ruta del archivo .json generado
    """
    if not os.path.exists(brik_path):
        print("ERROR: Archivo no encontrado: %s" % brik_path)
        return None
    
    print("Compilando: %s" % brik_path)
    
    try:
        # Parsear el archivo .brik
        ast = parse_file(brik_path)
        
        # Construir tabla de símbolos
        symbols = build_symbol_table(ast)
        
        # Crear estructura completa para JSON (incluye AST y símbolos)
        compiled_data = {
            "source_file": os.path.basename(brik_path),
            "ast": ast,
            "symbols": symbols
        }
        
        # Determinar nombre del archivo de salida
        base_name = os.path.splitext(brik_path)[0]
        json_path = base_name + ".json"
        
        # Escribir JSON
        with open(json_path, 'w') as f:
            json.dump(compiled_data, f, indent=2, ensure_ascii=False)
        
        print("  -> Generado: %s" % json_path)
        print("  -> Tamaño: %d bytes" % os.path.getsize(json_path))
        
        return json_path
        
    except Exception as e:
        print("ERROR al compilar %s: %s" % (brik_path, str(e)))
        return None


def compile_all():
    """
    Compila todos los archivos .brik en la carpeta specs/
    """
    specs_dir = "specs"
    
    if not os.path.exists(specs_dir):
        print("ERROR: No se encuentra la carpeta %s" % specs_dir)
        return
    
    brik_files = [f for f in os.listdir(specs_dir) if f.endswith('.brik')]
    
    if not brik_files:
        print("No se encontraron archivos .brik en %s" % specs_dir)
        return
    
    print("=" * 60)
    print("Compilando archivos .brik")
    print("=" * 60)
    
    compiled_count = 0
    for brik_file in brik_files:
        brik_path = os.path.join(specs_dir, brik_file)
        json_path = compile_brik_to_json(brik_path)
        if json_path:
            compiled_count += 1
        print()
    
    print("=" * 60)
    print("Compilación completada: %d/%d archivos" % (compiled_count, len(brik_files)))
    print("=" * 60)


def show_usage():
    """
    Muestra información de uso del compilador
    """
    print("Uso:")
    print("  python compiler.py <archivo.brik>     - Compila un archivo específico")
    print("  python compiler.py --all              - Compila todos los .brik en specs/")
    print("  python compiler.py --help             - Muestra esta ayuda")
    print()
    print("Ejemplos:")
    print("  python compiler.py specs/snake.brik")
    print("  python compiler.py specs/tetris.brik")
    print("  python compiler.py --all")


def main():
    """
    Punto de entrada principal del compilador
    """
    if len(sys.argv) < 2:
        show_usage()
        sys.exit(1)
    
    arg = sys.argv[1]
    
    if arg in ("--help", "-h"):
        show_usage()
        sys.exit(0)
    
    if arg == "--all":
        compile_all()
    else:
        # Compilar archivo específico
        brik_path = arg
        compile_brik_to_json(brik_path)


if __name__ == "__main__":
    main()
