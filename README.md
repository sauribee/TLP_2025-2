# Brick Game Engine — Deliverable 1 (Lexer · Parser · AST · Symbol Table)

**Authors:** Juan Miguel Páez Tatis, Santiago Uribe Echavarría  
**Course:** Theory of Programming Languages  
**Instructor:** Fernán Alonso Villa Garzón  
**Date:** October 2025

This repository contains a minimal C++11 toolchain for the `.brik` language used to describe brick-style games for Tetris and Snake games.
Deliverable 1 focuses on **lexing**, **parsing to an AST**, **symbol table construction**, and **basic semantic checks**.

---

## Repository layout

```TLP_2025-2/
├─ src/                  # Source code
│  ├─ main.cpp           # CLI entry point (--dump-ast, --dump-symbols, --json)
│  ├─ lexer.c++          # Lexer
│  ├─ ast_parser.hpp     # AST types + (recursive-descent / LL) parser
│  ├─ semantics.hpp      # Symbol table + basic semantic checks
│  └─ ...                # Other headers/implementations
├─ games/                # Language examples
│  ├─ tetris.brik
│  └─ snake.brik
├─ docs/                 # Technical documentation
│  ├─ LANGUAGE_SPEC.md   # Language spec (this deliverable)
│  ├─ GRAMMAR.ebnf       # Approx. grammar in EBNF (this deliverable)
│  └─ tokens_list.txt    # Token reference (optional)
├─ out/                  # Outputs (AST, symbols, diagnostics, JSON)
├─ .gitignore
└─ README.md
```

> If you only use this archive of docs, place the files alongside your sources as shown above.

---

## Build

Compile **all** sources under `src/`:

```bash
g++ -std=c++11 -O2 -Wall -Wextra -s src/*.c++ -o brik_parser.exe
```

- `-s` strips symbols to reduce size.
- If you see codepage issues in PowerShell, you can switch to UTF‑8:

---

## Usage

```bash
brik_parser.exe [--dump-ast] [--dump-symbols] [--json <file>] <file.brik>
```

**Examples:**

```powershell
.brik_parser.exe --dump-ast . etris.brik

.brik_parser.exe --dump-symbols .\snake.brik

.brik_parser.exe --json . etris.symbols.json . etris.brik

.brik_parser.exe --dump-ast --dump-symbols --json .\snake.symbols.json .\snake.brik `
  > snake.ast.txt 2> snake.diag.txt
```

---

## Outputs

**Output conventions:**

- **stdout**: AST / Symbol Table (with `--dump-*`).
- **stderr**: Diagnostics (semantic warnings/errors and syntax errors).

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

---

## Exit codes

- `0`: no semantic errors.
- `2`: syntax error **or** semantic errors.
- `1/3`: I/O or unexpected exceptions (see stderr).

---

## Language at a glance (this deliverable)

- **Lexical elements:** see `docs/tokens_list.txt` and the **Lexicon** section in `docs/LANGUAGE_SPEC.md`.
- **Grammar:** `docs/GRAMMAR.ebnf` describes `game`, `board`, `controls`, `pieces`, `piece_*`, and optional `rules_*` blocks.
- **Semantics (D1):**
  - Required keys in `game`, `board`, `controls`.
  - Basic types and ranges (e.g., positive integers for `width`, `height`; 4×4 matrices for piece rotations).
  - Piece identifiers consistent across declarations and references.
  - Optional rules blocks use the **`rules_` prefix**; canonical regex in the DSL context is `rules_\w+` (in languages that require escaping, use `rules_\\w+`).

---

## Development notes

- **Standard:** C++11 with warnings enabled (`-Wall -Wextra`), no external dependencies.
- **Organization:** small helpers (e.g., `baseName(...)`) live as free functions/utilities.
- **Versioning:** keep binaries under `out/` and **exclude** them via `.gitignore`.  
  Consider attaching compiled artifacts to a GitHub Release if required by your course.

---

## Documentation set

- `docs/LANGUAGE_SPEC.md` — authoritative language specification (lexicon, sections, types, semantics, diagnostics format).
- `docs/GRAMMAR.ebnf` — grammar snapshot aligned with the current parser.
- `docs/tokens_list.txt` — optional lexer token table for quick reference.

---