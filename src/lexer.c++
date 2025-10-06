#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <stdexcept>
#include <regex>

#ifdef _WIN32
  #include <direct.h>     
#else
  #include <sys/stat.h>
  #include <sys/types.h>
#endif

static void ensureOutDir() {
#ifdef _WIN32
    _mkdir("out");    
#else
    mkdir("out", 0755);
#endif
}

enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    STRING,
    BOOLEAN,
    
    // Keywords
    GAME,
    BOARD,
    CONTROLS,
    PIECES,
    SNAKE,
    TETRIS,
    LEVEL,
    
    // Identifiers and rules
    IDENTIFIER,
    VERSION,
    RULES_PREFIX,
    AVAILABLE_PIECES,
    
    // Operators
    ASSIGN,
    SEMICOLON,
    
    // Delimiters
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    COMMA,
    
    // Special
    EOF_TOKEN
};

std::string tokenTypeToString(TokenType type) {
    switch(type) {
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::STRING: return "STRING";
        case TokenType::BOOLEAN: return "BOOLEAN";
        case TokenType::GAME: return "GAME";
        case TokenType::BOARD: return "BOARD";
        case TokenType::CONTROLS: return "CONTROLS";
        case TokenType::PIECES: return "PIECES";
        case TokenType::SNAKE: return "SNAKE";
        case TokenType::TETRIS: return "TETRIS";
        case TokenType::LEVEL: return "LEVEL";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::VERSION: return "VERSION";
        case TokenType::RULES_PREFIX: return "RULES_PREFIX";
        case TokenType::AVAILABLE_PIECES: return "AVAILABLE_PIECES";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::COMMA: return "COMMA";
        case TokenType::EOF_TOKEN: return "EOF";
        default: return "UNKNOWN";
    }
}

class Token {
public:
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType t, const std::string& v, int l, int c) 
        : type(t), value(v), line(l), column(c) {}
    
    std::string toString() const {
        return "Token(" + tokenTypeToString(type) + ", \"" + value + "\", " + 
               std::to_string(line) + ", " + std::to_string(column) + ")";
    }
};

class Lexer {
private:
    std::string text;
    size_t position;
    int line;
    int column;
    std::vector<Token> tokens;
    std::map<std::string, TokenType> keywords;
       
    void initializeKeywords() {
        keywords["game"] = TokenType::GAME;
        keywords["board"] = TokenType::BOARD;
        keywords["controls"] = TokenType::CONTROLS;
        keywords["pieces"] = TokenType::PIECES;
        keywords["snake"] = TokenType::SNAKE;
        keywords["tetris"] = TokenType::TETRIS;
        keywords["level"] = TokenType::LEVEL;
        keywords["true"] = TokenType::BOOLEAN;
        keywords["false"] = TokenType::BOOLEAN;
        keywords["available_pieces"] = TokenType::AVAILABLE_PIECES;
    }
    
    bool isAlpha(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    
    bool isDigit(char c) {
        return c >= '0' && c <= '9';
    }
    
    bool isAlnum(char c) {
        return isAlpha(c) || isDigit(c);
    }
    
    bool isSpace(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }
    
    Token* matchString() {
        size_t startPos = position;
        int startColumn = column;
        position++; 
        column++;
        
        while (position < text.length() && text[position] != '"') {
            if (text[position] == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            position++;
        }
        
        if (position >= text.length()) {
            throw std::runtime_error("Unterminated string at line " + std::to_string(line) + 
                                   ", column " + std::to_string(startColumn));
        }
        
        position++;
        column++;
        
        std::string value = text.substr(startPos, position - startPos);
        return new Token(TokenType::STRING, value, line, startColumn);
    }
    
    Token* matchNumber() {
        size_t startPos = position;
        int startColumn = column;
        
        while (position < text.length() && isDigit(text[position])) {
            position++;
            column++;
        }
        
        if (position < text.length() && text[position] == '.') {
            position++;
            column++;
            
            while (position < text.length() && isDigit(text[position])) {
                position++;
                column++;
            }
            
            std::string value = text.substr(startPos, position - startPos);
            return new Token(TokenType::FLOAT, value, line, startColumn);
        }
        
        std::string value = text.substr(startPos, position - startPos);
        return new Token(TokenType::INTEGER, value, line, startColumn);
    }
    
    Token* matchIdentifierOrKeyword() {
        size_t startPos = position;
        int startColumn = column;
        
        while (position < text.length() && (isAlnum(text[position]) || text[position] == '_')) {
            position++;
            column++;
        }
        
        std::string value = text.substr(startPos, position - startPos);
        
        // Check for keywords first
        auto it = keywords.find(value);
        if (it != keywords.end()) {
            return new Token(it->second, value, line, startColumn);
        }
        
        // Check for rules prefix
        if (value.rfind("rules_", 0) == 0) {
            return new Token(TokenType::RULES_PREFIX, value, line, startColumn);
        }
        
        return new Token(TokenType::IDENTIFIER, value, line, startColumn);
    }
    
    Token* matchToken() {
        char currentChar = text[position];
        
        // Match specific patterns first
        std::smatch match;
        std::string lookahead = text.substr(position);
        if (std::regex_search(lookahead, match, std::regex("^(tetris|snake)\\s+(\\d+\\.\\d+)"))) {
            std::string keyword = match[1];
            std::string version_value = match[2];
            
            // Add the keyword token
            tokens.push_back(Token(keywords[keyword], keyword, line, column));
            
            // Advance the position past the keyword and whitespace
            size_t tempPos = position + keyword.length();
            int tempCol = column + keyword.length();
            while (tempPos < text.length() && isSpace(text[tempPos])) {
                if (text[tempPos] == '\n') {
                    line++;
                    tempCol = 1;
                } else {
                    tempCol++;
                }
                tempPos++;
            }
            
            position = tempPos;
            column = tempCol;
            
            // Return the version token
            position += version_value.length();
            column += version_value.length();
            return new Token(TokenType::VERSION, version_value, line, column - version_value.length());
        }

        if (currentChar == '"') {
            return matchString();
        }
        
        if (isDigit(currentChar)) {
            return matchNumber();
        }
        
        if (isAlpha(currentChar) || currentChar == '_') {
            return matchIdentifierOrKeyword();
        }
        
        // Single character tokens
        Token* token = nullptr;
        int startColumn = column;
        
        switch(currentChar) {
            case '=':
                token = new Token(TokenType::ASSIGN, "=", line, startColumn);
                break;
            case ';':
                token = new Token(TokenType::SEMICOLON, ";", line, startColumn);
                break;
            case '{':
                token = new Token(TokenType::LBRACE, "{", line, startColumn);
                break;
            case '}':
                token = new Token(TokenType::RBRACE, "}", line, startColumn);
                break;
            case '[':
                token = new Token(TokenType::LBRACKET, "[", line, startColumn);
                break;
            case ']':
                token = new Token(TokenType::RBRACKET, "]", line, startColumn);
                break;
            case ',':
                token = new Token(TokenType::COMMA, ",", line, startColumn);
                break;
            case '.':
                // Check if it's a part of a number, otherwise handle as a single char
                if (position + 1 < text.length() && isDigit(text[position + 1])) {
                    // This is a float, let matchNumber handle it
                    break;
                }
                // Fallthrough for single dot
                token = new Token(TokenType::IDENTIFIER, ".", line, startColumn);
                break;
        }
        
        if (token) {
            position++;
            column++;
        } else {
            throw std::runtime_error("Unexpected character '" + std::string(1, text[position]) + 
                                   "' at line " + std::to_string(line) + 
                                   ", column " + std::to_string(column));
        }
        
        return token;
    }
    
public:
    std::string source_name;
    
    static std::string baseName(const std::string& path) {
        size_t slash = path.find_last_of("/\\");
        std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
        size_t dot = name.find_last_of('.');
        if (dot != std::string::npos) name = name.substr(0, dot);
        return name;
    }
    
    Lexer(const std::string& inputText, const std::string& srcName)
        : text(inputText), position(0), line(1), column(1), source_name(srcName) {
        initializeKeywords();
    }

    Lexer(const std::string& inputText) 
        : text(inputText), position(0), line(1), column(1) {
        initializeKeywords();
    }
    
    std::vector<Token> tokenize() {
        while (position < text.length()) {
            if (isSpace(text[position])) {
                if (text[position] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
                position++;
                continue;
            }
            
            Token* token = matchToken();
            if (token) {
                tokens.push_back(*token);
                delete token;
            } else {
                throw std::runtime_error("Unexpected character '" + std::string(1, text[position]) + 
                                       "' at line " + std::to_string(line) + 
                                       ", column " + std::to_string(column));
            }
        }
        
        ensureOutDir();
        std::string base = source_name.empty() ? "tokens" : baseName(source_name);
        std::string outPath = "out/" + base + ".tokens.txt";

        std::ofstream outFile(outPath.c_str(), std::ios::out | std::ios::trunc);
        if (outFile.is_open()) {
            for (const auto& token : tokens) {
                outFile << token.toString() << std::endl;
            }
            outFile.close();
        }

        tokens.push_back(Token(TokenType::EOF_TOKEN, "", line, column));
        
        return tokens;
    }
};

void checkBrikFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    Lexer lexer(content, filename);
    try {
        std::vector<Token> tokens = lexer.tokenize();
        std::cout << "Syntax OK. Tokens written to out/" 
                  << Lexer::baseName(filename) << ".tokens.txt" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Syntax error: " << e.what() << std::endl;
    }
}
