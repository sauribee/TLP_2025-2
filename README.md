# Brick Game Engine — Deliverable 1 (Lexer + Parser + AST + Symbol Table)

**Authors:** Juan Miguel Páez Tatis, Santiago Uribe Echavarría  
**Course:** Programming Language Theory (Teoría de lenguajes de programación)  
**Instructor:** Fernán Alonso Villa Garzón
**Date**  October 2025

This repository contains a minimal C++11 toolchain for the `.brik` language used to describe brick-style games for Tetris and Snake games.
Deliverable 1 focuses on **lexing**, **parsing to an AST**, **symbol table construction**, and **basic semantic checks**.

---

## Platform constraints

- Target environment: **Windows XP** on **AMD Athlon XP**
- Small footprint: executable + game files should fit on a **1.44 MB** floppy
- Toolchain: C++11 (MinGW/GCC recommended)

---

## Repository layout

```├─ src/ # sources
│ ├─ main.cpp # main entry with flags (--dump-ast, --dump-symbols, --json)
│ ├─ parser_main.cpp # optional secondary entry (AST/syntax-only)
│ ├─ lexer.c++ # provided lexer
│ ├─ ast_parser.hpp # AST and recursive-descent parser
│ └─ semantics.hpp # symbol table + semantic checks
├─ games/ # language samples
│ ├─ tetris.brik
│ └─ snake.brik
├─ docs/ # technical documentation
│ ├─ LANGUAGE_SPEC.md
│ ├─ GRAMMAR.ebnf
│ └─ tokens_list.txt # lexer token order (reference)
├─ out/ # test outputs (AST, symbols, diagnostics, JSON)
└─ README.md
```

> If you only use this archive of docs, place the files alongside your sources as shown above.

## Build

```powershell
g++ -std=c++11 -O2 -Wall -Wextra -s -o .\brik_parser.exe .\src\main.cpp
```

- `-s` strips symbols to reduce size.
- If you see codepage issues in PowerShell, you can switch to UTF‑8:

  ```powershell
  chcp 65001
  $OutputEncoding = [Console]::OutputEncoding = [Text.UTF8Encoding]::UTF8
  ```

## Usage

```brik_parser.exe [--dump-ast] [--dump-symbols] [--json <file>] <file.brik>
```

Examples:

```powershell
.brik_parser.exe --dump-ast . etris.brik

.brik_parser.exe --dump-symbols .\snake.brik

.brik_parser.exe --json . etris.symbols.json . etris.brik

.brik_parser.exe --dump-ast --dump-symbols --json .\snake.symbols.json .\snake.brik `
  > snake.ast.txt 2> snake.diag.txt
```

- **stdout**: AST / Symbol Table (with `--dump-*`).
- **stderr**: Diagnostics (semantic warnings/errors and syntax errors).

## Outputs

- **AST (text)**: human-readable, via `dumpAST`.
- **Symbol Table (text)**: per-scope key/value view for the engine.
- **Symbol Table (JSON)**: dependency-free C++11 emitter. Example:

  ```json
  {
    "scopes": {
      "game":    { "title": "Tetris", "version": 1.0, "id": "tetris", "available_pieces": ["I","O"] },
      "board":   { "width": 10, "height": 20 },
      "piece:I": { "color": "cyan", "rotations": [[[0,1,1,0],[0,0,1,1],[0,0,0,0],[0,0,0,0]]] }
    }
  }
  ```

## Exit codes

- `0`: no semantic errors.
- `2`: syntax error **or** semantic errors.
- `1/3`: I/O or unexpected exceptions (see stderr).

## Notes

- Syntax errors: precise locations (line/col).
- Semantic diagnostics: clear warnings/errors (see `docs/LANGUAGE_SPEC.md`).

## Next steps (Deliverable 2)

- Interpreter/evaluator consuming JSON/symbols.
- Map `controls` and implement `rules_*` behavior.
- Add more semantic rules (ranges, allowed keys, etc.).
