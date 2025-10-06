# .brik Language — Specification (Deliverable 1)

This document defines the **syntax**, **AST**, **symbol table**, and **semantic checks** implemented.

## 1. Overview

A `.brik` file describes a brick‑style game (e.g., Tetris/Snake). General form:

```<header> game "Title" {
  <blocks...>
}
```

- `<header>`: `tetris | snake | IDENT` followed by `VERSION` (number).
- `game "Title"`: human‑readable game title.
- `<blocks>`: sections such as `board`, `controls`, `pieces`, `rules_*`, or game‑specific blocks (`tetris`, `snake`, `level`, or other identifiers).

## 2. Lexical tokens

Recognized token kinds (as used in `PToken::Kind`):

- Literals: `INTEGER`, `FLOAT`, `STRING`, `BOOLEAN` (`true`/`false`).
- Identifiers: `IDENTIFIER` (letters/digits/underscore, starting with letter/underscore).
- Keywords: `GAME`, `BOARD`, `CONTROLS`, `PIECES`, `SNAKE`, `TETRIS`, `LEVEL`.
- Rules prefix: `RULES_PREFIX` (any identifier beginning with `rules_`, e.g., `rules_line_clear`).
- Punctuation/operators: `ASSIGN (=)`, `SEMICOLON (;)`, `LBRACE ({)`, `RBRACE (})`, `LBRACKET ([)`, `RBRACKET (])`, `COMMA (,)`.
- `VERSION` (number after the header).
- `AVAILABLE_PIECES` (special identifier used for that configuration key).

### Appendix: tokens_list.txt

```
TOKENS = {

    # Literals
    'INTEGER': '\d+',
    'FLOAT': '\d+\.\d+',
    'STRING': '"[^"]*"',
    'BOOLEAN': '(true|false)',
    
    # Keywords
    'GAME': 'game',
    'BOARD': 'board',
    'CONTROLS': 'controls',
    'PIECES': 'pieces',
    'PIECE': 'piece_[A-Z_]+',
    'SNAKE': 'snake',
    'TETRIS': 'tetris',
    'LEVEL': 'level',
    
    # Rule blocks
    'RULES_PREFIX': 'rules_\w+',
    'AVAILABLE_PIECES': 'available_pieces',
    
    # Identifiers
    'IDENTIFIER': '[a-z_][a-z0-9_]*',
    'VERSION': '\d+\.\d+',
    
    # Operators
    'ASSIGN': '=',
    'SEMICOLON': ';',
    
    # Delimiters
    'LBRACE': '{',
    'RBRACE': '}',
    'LBRACKET': '[',
    'RBRACKET': ']',
    'COMMA': ',',

    # Comments (optional)
    'COMMENT': '//[^\n\r]*',
    
    # Whitespace
    'WHITESPACE': '[ \t\n\r]+',
}

# Grammar Rules
GRAMMAR = """
program ::= game_name version game_block

game_name ::= IDENTIFIER
version ::= VERSION

game_block ::= GAME STRING LBRACE 
                   config_blocks 
               RBRACE

config_blocks ::= config_block*

config_block ::= board_block
               | controls_block
               | pieces_block
               | snake_block
               | tetris_block
               | level_block
               | rules_block
               | available_pieces_stmt
               | game_specific_block

board_block ::= BOARD LBRACE 
                    property_list 
                RBRACE

controls_block ::= CONTROLS LBRACE 
                       property_list 
                   RBRACE

pieces_block ::= PIECES LBRACE 
                     piece_definition* 
                 RBRACE

piece_definition ::= IDENTIFIER LBRACE 
                         property_list 
                     RBRACE

snake_block ::= SNAKE LBRACE 
                    property_list 
                RBRACE

tetris_block ::= TETRIS LBRACE 
                     property_list 
                 RBRACE

level_block ::= LEVEL LBRACE 
                    property_list 
                RBRACE

rules_block ::= RULES_PREFIX LBRACE 
                    property_list 
                RBRACE

game_specific_block ::= IDENTIFIER LBRACE 
                            property_list 
                        RBRACE

property_list ::= property*

property ::= IDENTIFIER ASSIGN value SEMICOLON

available_pieces_stmt ::= AVAILABLE_PIECES ASSIGN array SEMICOLON

value ::= STRING
        | INTEGER
        | FLOAT
        | BOOLEAN
        | array
        | IDENTIFIER

array ::= LBRACKET array_elements RBRACKET
        | LBRACKET RBRACKET

array_elements ::= value (COMMA value)*
```

## 3. Grammar (EBNF)

> Matches the shipped recursive‑descent parser. See `docs/GRAMMAR.ebnf` for a standalone file.

```
Program     := Header GAME STRING LBRACE Block* RBRACE END ;
Header      := (TETRIS | SNAKE | IDENTIFIER) VERSION ;

Block       := BoardBlock
             | ControlsBlock
             | PiecesBlock
             | RulesBlock
             | AssignBlock          // top-level available_pieces
             | GameSpecificBlock ;

BoardBlock      := BOARD LBRACE Prop* RBRACE ;
ControlsBlock   := CONTROLS LBRACE Prop* RBRACE ;

PiecesBlock     := PIECES LBRACE ( PiecesExtra | PieceDef )* RBRACE ;
PiecesExtra     := AVAILABLE_PIECES ASSIGN Value SEMICOLON ;
PieceDef        := IDENTIFIER LBRACE Prop* RBRACE ;

RulesBlock      := RULES_PREFIX LBRACE Prop* RBRACE ;

GameSpecificBlock := (TETRIS | SNAKE | LEVEL | IDENTIFIER) LBRACE Prop* RBRACE ;

AssignBlock     := AVAILABLE_PIECES ASSIGN Value SEMICOLON ;

Prop        := IDENTIFIER ASSIGN Value SEMICOLON ;

Value       := STRING | INTEGER | FLOAT | BOOLEAN | IDENTIFIER | Array ;
Array       := LBRACKET (Value (COMMA Value)*)? RBRACKET ;
```

### Design notes

- `available_pieces` is accepted **both** within `pieces { ... }` and **at** `game { ... }` level (compat).
- Piece names are plain identifiers (no enforced `piece_` prefix).
- `RULES_PREFIX` captures any identifier starting with `rules_`.
- `GameSpecificBlock` accepts `tetris`, `snake`, `level`, or any other identifier as a game‑specific block name.

## 4. AST (summary)

See `ast_parser.hpp`. Main nodes:

- `Program { game_id, version, game }`
- `GameBlock { title, blocks[] }`
- `BoardBlock { props[] }`, `ControlsBlock { props[] }`, `PiecesBlock { pieces[], extras[] }`
- `PieceDef { name, props[] }`
- `RulesBlock { name, props[] }`
- `GameSpecificBlock { name, props[] }`
- `AssignBlock { stmt }`  *(for `available_pieces = [...] ;` at `game` level)*
- `Assign { ident, value }`
- `Value` variants: `VInt`, `VFloat`, `VBool`, `VString`, `VIdent`, `VArray`

The optional AST dump prints a readable, indented structure.

## 5. Symbol Table

Built per **scope** (`std::map<std::string, SymValue>`), e.g.:

- `game`: `title`, `version`, `id`, and `available_pieces` if provided at game level.
- `board`, `controls`, `pieces` (extras only), `piece:<name>`, `rules:<name>`, `tetris`, `snake`, `level`, …

`SymValue` supports: `INT`, `FLOAT`, `BOOL`, `STRING`, `IDENT`, `ARRAY`.  
Arrays preserve nested structure for matrices (e.g., 4×4 rotations).

### JSON output (with `--json`)

Schema:

```
json
{
  "scopes": {
    "<scope-name>": {
      "<key>": <value> | {"$ident":"NAME"} | [ ... ]
    }
  }
}
```

- Strings are JSON strings.
- Idents are emitted as objects `{"$ident":"NAME"}` to distinguish from strings.

## 6. Semantic rules (implemented)

- **Board**:
  - `board.width` and `board.height` must be **integers > 0** (if present).
- **Pieces**:
  - `available_pieces`: must be an **array** of **strings or idents**; each name **must exist** as `piece:<name>`.
  - `piece:<name>.rotations` (if present): must be an **array of 4×4** integer matrices.
- **Game‑specific**:
  - `snake`: if `level.speed` exists, it must be **integer**.
  - `tetris`: if `tetris.gravity` exists, it must be **numeric** (int or float).
- **Redefinitions** inside the same scope: allowed but reported as a **warning** (“redefinition… overwriting previous value”).

> Extending rules is straightforward: add checks in `semantics.hpp` (`checkBoard`, `checkPieces`, `checkGameSpecific`).

## 7. Errors and diagnostics

- **Syntax**: `Syntax error at (line:col): <message>` with exit code `2`.
- **Semantics**: list of `error`/`warning` to **stderr**. Program returns:
  - `0` if **no** semantic errors,
  - `2` if there **are** semantic errors.

## 8. Recommended tests

- **Positive**: `tetris.brik`, `snake.brik`.
- **Negative**: examples with
  - `board.width = -1;`
  - `available_pieces = [ "X" ];` without a `piece:X` definition
  - malformed `rotations` (not 4×4)
  - wrong types (e.g., `board.width = "10";`)

See `docs/TESTS.md` for commands and expected exits.

## 9. Current limitations

- Comments are not formally specified.
- No validation yet for specific `controls` keys.
- `rules_*` blocks stored but not interpreted.

## 10. Portability & size

- Pure C++11, header‑only parser/semantics.
- Build with `-O2 -s` to reduce size.

## 11. Roadmap (Deliverable 2)

- Interpreter/evaluator consuming Symbol Table / JSON.
- Implement `controls` mapping and `rules_*` logic.
- Add semantic ranges (e.g., gravity limits, scores).
