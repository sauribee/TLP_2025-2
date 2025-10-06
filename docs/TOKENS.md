## Lexer Tokens

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
