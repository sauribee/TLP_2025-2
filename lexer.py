import re
from enum import Enum, auto

class TokenType(Enum):
    # Literals
    INTEGER = auto()
    FLOAT = auto()
    STRING = auto()
    BOOLEAN = auto()
    
    # Keywords
    GAME = auto()
    BOARD = auto()
    CONTROLS = auto()
    PIECES = auto()
    SNAKE = auto()
    TETRIS = auto()
    LEVEL = auto()
    
    # Identifiers and rules
    IDENTIFIER = auto()
    VERSION = auto()
    RULES_PREFIX = auto()
    AVAILABLE_PIECES = auto()
    
    # Operators
    ASSIGN = auto()
    SEMICOLON = auto()
    
    # Delimiters
    LBRACE = auto()
    RBRACE = auto()
    LBRACKET = auto()
    RBRACKET = auto()
    COMMA = auto()
    
    # Special
    EOF = auto()

class Token:
    def __init__(self, type, value, line, column):
        self.type = type
        self.value = value
        self.line = line
        self.column = column
    
    def __repr__(self):
        return f"Token({self.type}, {self.value}, {self.line}, {self.column})"

class Lexer:
    def __init__(self, text):
        self.text = text
        self.position = 0
        self.line = 1
        self.column = 1
        self.tokens = []
        
    def tokenize(self):
        while self.position < len(self.text):
            # Skip whitespace
            if self.text[self.position].isspace():
                if self.text[self.position] == '\n':
                    self.line += 1
                    self.column = 1
                else:
                    self.column += 1
                self.position += 1
                continue
            
            # Match tokens
            token = self.match_token()
            if token:
                self.tokens.append(token)
            else:
                raise SyntaxError(f"Unexpected character '{self.text[self.position]}' at line {self.line}, column {self.column}")
        
        # Clear the resultados.txt file before writing (for each iteration)
        with open("resultados.txt", "w", encoding="utf-8") as fl:
            for token in self.tokens:
                fl.write(f"{token}\n")
        self.tokens.append(Token(TokenType.EOF, None, self.line, self.column))
        return self.tokens
    
    def match_token(self):
        # Skip whitespace
        if self.text[self.position].isspace():
            return None  # Let the main loop handle this

        match = re.match(r'(tetris|snake)\s+\d+\.\d+', self.text[self.position:])
        if match:
            value = match.group(0)
            token = Token(TokenType.VERSION, value, self.line, self.column)
            self.position += len(value)
            self.column += len(value)
            return token

        # Numbers
        if self.text[self.position].isdigit():
            return self.match_number()
        
        # Identifiers and keywords
        if self.text[self.position].isalpha() or self.text[self.position] == '_':
            return self.match_identifier_or_keyword()
        
        # String literals
        if self.text[self.position] == '"':
            return self.match_string()

        # Single character tokens
        single_chars = {
            '=': TokenType.ASSIGN,
            ';': TokenType.SEMICOLON,
            '{': TokenType.LBRACE,
            '}': TokenType.RBRACE,
            '[': TokenType.LBRACKET,
            ']': TokenType.RBRACKET,
            ',': TokenType.COMMA
        }
        
        if self.text[self.position] in single_chars:
            token = Token(single_chars[self.text[self.position]], self.text[self.position], self.line, self.column)
            self.position += 1
            self.column += 1
            return token
        
        return None
    
    def match_string(self):
        start_pos = self.position
        start_column = self.column
        self.position += 1  # Skip opening quote
        self.column += 1
        
        while self.position < len(self.text) and self.text[self.position] != '"':
            self.position += 1
            self.column += 1
        
        if self.position >= len(self.text):
            raise SyntaxError(f"Unterminated string at line {self.line}, column {start_column}")
        
        self.position += 1  # Skip closing quote
        self.column += 1
        
        value = self.text[start_pos:self.position]
        return Token(TokenType.STRING, value, self.line, start_column)
    
    def match_number(self):
        start_pos = self.position
        start_column = self.column
        
        while self.position < len(self.text) and self.text[self.position].isdigit():
            self.position += 1
            self.column += 1
        
        # Check for float
        if self.position < len(self.text) and self.text[self.position] == '.':
            self.position += 1
            self.column += 1
            
            while self.position < len(self.text) and self.text[self.position].isdigit():
                self.position += 1
                self.column += 1
            
            value = self.text[start_pos:self.position]
            return Token(TokenType.FLOAT, float(value), self.line, start_column)
        
        value = self.text[start_pos:self.position]
        return Token(TokenType.INTEGER, int(value), self.line, start_column)
    
    def match_identifier_or_keyword(self):
        start_pos = self.position
        start_column = self.column
        
        while self.position < len(self.text) and (self.text[self.position].isalnum() or self.text[self.position] == '_'):
            self.position += 1
            self.column += 1
        
        value = self.text[start_pos:self.position]
        
        # Check for keywords
        keywords = {
            'game': TokenType.GAME,
            'board': TokenType.BOARD,
            'controls': TokenType.CONTROLS,
            'pieces': TokenType.PIECES,
            'snake': TokenType.SNAKE,
            'tetris': TokenType.TETRIS,
            'level': TokenType.LEVEL,
            'true': TokenType.BOOLEAN,
            'false': TokenType.BOOLEAN,
            'available_pieces': TokenType.AVAILABLE_PIECES
        }
        
        if value in keywords:
            return Token(keywords[value], value, self.line, start_column)
        
        # Check for rules prefix
        if value.startswith('rules_'):
            return Token(TokenType.RULES_PREFIX, value, self.line, start_column)
        
        # Check for version number (e.g., "1.0")
        if re.match(r'^\d+\.\d+$', value):
            return Token(TokenType.VERSION, value, self.line, start_column)
        
        return Token(TokenType.IDENTIFIER, value, self.line, start_column)
    

def check_brik_file(filename):
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()
    lexer = Lexer(content)
    try:
        tokens = lexer.tokenize()
        print("Syntax OK. Tokens:")
        # for token in tokens:
        #     print(token)
    except SyntaxError as e:
        print(f"Syntax error: {e}")

check_brik_file("tetris.brik")

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        check_brik_file(sys.argv[1])
    else:
        print("Usage: python lexer.py <file.brik>")
