# Brick Game Engine — Assignment 2 - Practice project 

**Authors:** Juan Miguel Páez Tatis, Santiago Uribe Echavarría  
**Course:** Theory of Programming Languages  
**Instructor:** Fernán Alonso Villa Garzón  
**Date:** November 2025

This repository contains a minimal C++ toolchain for the `.brik` language used to describe brick-style games for Tetris and Snake games.

- Assigment 1 focuses on **lexing**, **parsing to an AST**, **symbol table construction**, and **basic semantic checks**.

- Assignment 2 implements a minimal game engine that satisfies the rubric:
  - Window **640×480** (SDL2).
  - Main loop: **events → update(dt) → render → present**.
  - Basic drawing functions: `dibujar_ladrillo(...)` (brick), `dibujar_texto(...)` (text).
  - Keyboard input mapping (actions: left/right/up/down/escape/restart).
  - Demos: `--demo brick` (rubric minimum), `--demo snake`, `--demo tetris`

---

## Repository layout

```
TLP_2025-2/
├─ docs/                 # Technical documentation
│  ├─ GRAMMAR.ebnf       # Approx. grammar in EBNF 
│  ├─ LANGUAGE_SPEC.md   # Language spec 
│  └─ tokens_list.txt    # Token reference 
├─ engine/
│  ├─ assets/
│  │  └─ DejaVuSans.ttf                    # Font used for text (TTF)
│  └─ src/
│     ├─ Engine.hpp / Engine.cpp           # Engine (SDL2 + SDL_ttf)
│     ├─ GameFactory.hpp / GameFactory.cpp # Demo selection via CLI
│     ├─ IGame.hpp                         # Game interface (init/update/render)
│     ├─ Input.hpp / Input.cpp             # Action-based input map
│     ├─ RulesBrick.cpp                    # Minimal demo (move a single brick)
│     ├─ RulesSnake.cpp                    # Snake demo
│     └─ RulesTetris.cpp                   # Tetris demo
├─ games/                # Language examples
│  ├─ snake.brik
│  └─ tetris.brik
├─ out/                  # Outputs examples (AST, symbols, diagnostics, JSON)
├─ src/                  # Source code
│  ├─ ast_parser.hpp     # AST types + (recursive-descent / LL) parser
│  ├─ lexer.c++          # Lexer
│  ├─ main.cpp           # CLI entry point (--dump-ast, --dump-symbols, --json)
│  └─ semantics.hpp      # Symbol table + basic semantic checks
├─ .gitignore
└─ README.md
```

> If you only use this archive of docs, place the files alongside your sources as shown above.

---

## Requirements (Windows)

> Always use the **MSYS2 MinGW 64-bit** shell (the prompt must show `MINGW64`).

Install the toolchain and dependencies:
```bash
pacman -Syu
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-pkgconf \
               mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf
```

Font (once):
```bash
mkdir -p assets
cp /c/Windows/Fonts/arial.ttf assets/DejaVuSans.ttf
```

## Build

For assignment 1:

Compile **all** sources under `src/`:

```
bash
g++ -std=c++11 -O2 -Wall -Wextra -s src/*.c++ -o brik_parser.exe
```

- `-s` strips symbols to reduce size.
- If you see codepage issues in PowerShell, you can switch to UTF‑8:


For assginment 2:

From `engine/`:
```bash
g++ -std=c++17 src/*.cpp -I src \
  $(pkg-config --cflags sdl2 SDL2_ttf) \
  $(pkg-config --libs sdl2 SDL2_ttf) \
  -O2 -o brik_engine.exe
```
---

## Usage

```
bash
brik_parser.exe [--dump-ast] [--dump-symbols] [--json <file>] <file.brik>
```

**Examples:**

```
powershell
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

```
json
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

## Language at a glance 

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

## Run (demos)

```bash
# Rubric-minimal demo (move one brick with arrow keys)
./brik_engine.exe --demo brick

# Extra demos
./brik_engine.exe --demo snake
./brik_engine.exe --demo tetris
```

**Controls**
- **Arrows**: move.
- **R**: restart demo.
- **ESC**: quit.

### Input (InputMap)
```cpp
// Bind actions to keys
input.bind("left",  "LEFT");
input.bind("right", "RIGHT");
input.bind("up",    "UP");
input.bind("down",  "DOWN");

// Query state during the frame
bool pressed = input.down(engine, "left");
```

## Acceptance checklist (quick)

- [x] Creates a **640×480** window (via `SDL_RenderSetLogicalSize`).
- [x] Main loop **events → update → render → present**.
- [x] Drawing functions **`dibujar_ladrillo` / `dibujar_texto`** exist and are used.
- [x] Keyboard controls move the brick.
- [x] `--demo brick` displays a movable brick (rubric minimum).

## Common issues (and fixes)

- **“`pacman` not recognized”** → You are in PowerShell/CMD. Open **MSYS2 MinGW 64-bit**.
- **Black window / missing text** → Ensure `assets/DejaVuSans.ttf` exists.
- **Build errors** → Check `pkg-config --cflags/--libs sdl2 SDL2_ttf` returns include/lib flags.
- **SmartScreen blocks the EXE** → Click “More info → Run anyway”.
- **Run on another PC without MSYS2** → Use the portable packaging below.
