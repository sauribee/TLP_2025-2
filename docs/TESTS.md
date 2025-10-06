# Tests — Deliverable 1

This file describes how to validate the lexer+parser+AST+semantics.

## 1) Build

```
powershell
g++ -std=c++11 -O2 -Wall -Wextra -s -o brik_parser.exe main.cpp
```

## 2) Positive tests

### Tetris

```
powershell
.brik_parser.exe --dump-ast --dump-symbols --json . etris.symbols.json . etris.brik `
  > tetris.ast.txt 2> tetris.diag.txt
```

Expected: non-empty AST and Symbol Table, `tetris.symbols.json` created, and diagnostics without errors (`exit code 0`).

### Snake

```powershell
.brik_parser.exe --dump-ast --dump-symbols --json .\snake.symbols.json .\snake.brik `
  > snake.ast.txt 2> snake.diag.txt
```

Expected: similar to Tetris; no semantic errors if file is valid (`exit code 0`).

## 3) Negative tests (samples)

Create minimal files and run the parser; expect **exit code 2** and diagnostics with `error`.

### A) Negative: invalid board width

```text
tetris 1.0 game "BadBoard" {
  board {
    width = -1;
    height = 20;
  }
}
```

Run:

```
powershell
.brik_parser.exe --dump-symbols .bad_board.brik 2> bad_board.diag.txt
```

Expected: semantic error about `board.width > 0`.

### B) Negative: unknown piece in available_pieces

```text
tetris 1.0 game "BadPieces" {
  pieces {
    // no piece X defined
  }
  available_pieces = ["X"];
}
```

Run:

```
powershell
.brik_parser.exe --dump-symbols .bad_pieces.brik 2> bad_pieces.diag.txt
```

Expected: semantic error referencing undefined piece `X`.

### C) Negative: rotations not 4x4

```text
tetris 1.0 game "BadRot" {
  pieces {
    I {
      rotations = [ [ [1,1,1] ] ]; // not 4x4
    }
  }
  available_pieces = ["I"];
}
```

Run:

```
powershell
.brik_parser.exe --dump-symbols .bad_rot.brik 2> bad_rot.diag.txt
```

Expected: semantic error about 4x4 integer matrices.

## 4) Batch test (optional)

PowerShell one-liner to run all `.brik` files in the folder:

```powershell
Get-ChildItem -Filter *.brik | ForEach-Object { 
  $name = $_.BaseName
  .brik_parser.exe --dump-ast --dump-symbols --json "$name.symbols.json" $_.FullName `
    > "$name.ast.txt" 2> "$name.diag.txt"
  Write-Host "$name => ExitCode=$LASTEXITCODE"
}
```

> The program uses `stderr` for diagnostics so `stdout` stays clean for AST/symbols.
