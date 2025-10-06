// =============================================================
// ast_parser.hpp  —  Header-only AST + Recursive-Descent Parser
// for the .brik language (Entrega 1)
//
// Target: Windows XP toolchains (C++11, no heavy deps). This version
// accepts top-level `available_pieces = [...];` and a `level { ... }` block.
// -------------------------------------------------------------
#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <memory>
#include <sstream>
#include <iostream>

// ==========================
//  AST NODES
// ==========================
struct Node {
    int line = 0;
    int col  = 0;
    virtual ~Node() {}
};

struct Value : Node { virtual ~Value(){} };
struct VInt    : Value { long v=0; };
struct VFloat  : Value { double v=0.0; };
struct VBool   : Value { bool v=false; };
struct VString : Value { std::string v; };
struct VIdent  : Value { std::string v; };
struct VArray  : Value { std::vector<std::unique_ptr<Value>> elems; };

struct Assign : Node {
    std::string ident;
    std::unique_ptr<Value> value;
};

struct Block : Node { virtual ~Block(){} };

struct BoardBlock : Block { std::vector<std::unique_ptr<Assign>> props; };
struct ControlsBlock : Block { std::vector<std::unique_ptr<Assign>> props; };

struct PieceDef : Node {
    std::string name;                               // e.g., "piece_I"
    std::vector<std::unique_ptr<Assign>> props;     // color, rotations, etc.
};

struct PiecesBlock : Block {
    std::vector<std::unique_ptr<PieceDef>> pieces;
    // optional: available_pieces inside pieces { } (compat)
    std::vector<std::unique_ptr<Assign>> extras;
};

struct RulesBlock : Block {
    std::string name; // full token text, e.g., "rules_line_clear"
    std::vector<std::unique_ptr<Assign>> props;
};

// Top-level single assignment as a block (e.g., available_pieces = [..];)
struct AssignBlock : Block {
    std::unique_ptr<Assign> stmt; // stmt->ident holds key (e.g., "available_pieces")
};

struct GameSpecificBlock : Block {
    std::string name; // "tetris", "snake", "level", etc.
    std::vector<std::unique_ptr<Assign>> props;
};

struct GameBlock : Node {
    std::string title; // from game "..."
    std::vector<std::unique_ptr<Block>> blocks; // board/controls/pieces/rules_*/tetris|snake|level|assign
};

struct Program : Node {
    std::string game_id; // "tetris" | "snake" | IDENT
    double version = 0.0; // from VERSION token
    std::unique_ptr<GameBlock> game;
};

// ==========================
//  SYMBOL TABLE (placeholder for later)
// ==========================
struct SymValue {
    enum class Kind { INT, FLOAT, BOOL, STRING, IDENT, ARRAY };
    Kind kind = Kind::INT;
    long i=0; double f=0.0; bool b=false; std::string s; // union-lite
    std::vector<SymValue> arr;                            // arrays
};

using Scope = std::map<std::string, SymValue>;

// ==========================
//  PARSER
// ==========================
struct PToken {
    enum class Kind {
        INTEGER, FLOAT, STRING, BOOLEAN,
        GAME, BOARD, CONTROLS, PIECES,
        SNAKE, TETRIS, LEVEL,
        IDENTIFIER, VERSION, RULES_PREFIX, AVAILABLE_PIECES,
        ASSIGN, SEMICOLON,
        LBRACE, RBRACE, LBRACKET, RBRACKET, COMMA,
        END 
    };
    Kind kind;
    std::string lex;
    int line = 0, col = 0;
};

struct ParseError : std::runtime_error {
    int line, col;
    ParseError(const std::string& msg, int l, int c)
        : std::runtime_error(msg), line(l), col(c) {}
};

class Parser {
public:
    explicit Parser(std::vector<PToken> toks) : ts(std::move(toks)) {}

    std::unique_ptr<Program> parseProgram() {
        auto prog = std::unique_ptr<Program>(new Program());
        // header: (TETRIS|SNAKE|IDENT) VERSION GAME STRING '{' blocks '}'
        if (match(PToken::Kind::TETRIS) || match(PToken::Kind::SNAKE) || match(PToken::Kind::IDENTIFIER)) {
            const PToken& idtok = prev();
            prog->game_id = idtok.lex;
            prog->line = idtok.line; prog->col = idtok.col;
        } else {
            throw expected("game identifier (tetris|snake|IDENT)");
        }
        if (match(PToken::Kind::VERSION)) {
            const PToken& vt = prev();
            prog->version = parseDouble(vt.lex);
        } else {
            throw expected("VERSION (e.g., 1.0)");
        }
        consume(PToken::Kind::GAME, "'game'");
        const PToken& titleTok = consume(PToken::Kind::STRING, "game title string");
        auto game = std::unique_ptr<GameBlock>(new GameBlock());
        game->title = unquote(titleTok.lex);
        game->line = titleTok.line; game->col = titleTok.col;
        consume(PToken::Kind::LBRACE, "'{'");
        // blocks until '}'
        while (!check(PToken::Kind::RBRACE) && !isAtEnd()) {
            game->blocks.push_back(parseBlock());
        }
        consume(PToken::Kind::RBRACE, "'}'");
        prog->game = std::move(game);
        consume(PToken::Kind::END, "EOF");
        return prog;
    }

private:
    std::vector<PToken> ts;
    size_t i = 0;

    // ---------- helpers ----------
    bool isAtEnd() const { return i >= ts.size(); }
    const PToken& peek()  const { return ts[i]; }
    const PToken& prev()  const { return ts[i-1]; }

    bool check(PToken::Kind k) const { return !isAtEnd() && ts[i].kind == k; }
    bool match(PToken::Kind k) { if (check(k)) { ++i; return true; } return false; }

    const PToken& consume(PToken::Kind k, const char* what) {
        if (check(k)) { return ts[i++]; }
        throw expected(what);
    }

    ParseError expected(const std::string& what) const {
        std::ostringstream oss;
        oss << "Expected " << what << ", found '" << (isAtEnd()?"<EOF>":ts[i].lex) << "'";
        int l = isAtEnd()? (ts.empty()?0:ts.back().line) : ts[i].line;
        int c = isAtEnd()? (ts.empty()?0:ts.back().col ) : ts[i].col;
        return ParseError(oss.str(), l, c);
    }

    static double parseDouble(const std::string& s) {
        std::istringstream iss(s); double d=0.0; iss>>d; return d;
    }
    static long parseLong(const std::string& s) {
        std::istringstream iss(s); long d=0; iss>>d; return d;
    }
    static std::string unquote(const std::string& s) {
        if (s.size()>=2 && s.front()=='"' && s.back()=='"') return s.substr(1, s.size()-2);
        return s;
    }

    // ---------- blocks ----------
    std::unique_ptr<Block> parseBlock() {
        if (match(PToken::Kind::BOARD)) return parseBoardBlock();
        if (match(PToken::Kind::CONTROLS)) return parseControlsBlock();
        if (match(PToken::Kind::PIECES)) return parsePiecesBlock();
        if (check(PToken::Kind::RULES_PREFIX)) return parseRulesBlock();
        if (check(PToken::Kind::AVAILABLE_PIECES)) return parseTopLevelAssign();
        // Accept game-specific names including 'level'
        if (check(PToken::Kind::SNAKE) || check(PToken::Kind::TETRIS) || check(PToken::Kind::LEVEL) || check(PToken::Kind::IDENTIFIER))
            return parseGameSpecificBlock();
        throw expected("one of: board/controls/pieces/rules_*/available_pieces/game-specific block");
    }

    std::unique_ptr<Block> parseBoardBlock() {
        auto b = std::unique_ptr<BoardBlock>(new BoardBlock());
        const PToken& open = consume(PToken::Kind::LBRACE, "'{'");
        b->line = open.line; b->col = open.col;
        while (!check(PToken::Kind::RBRACE)) {
            b->props.push_back(parseAssign());
        }
        consume(PToken::Kind::RBRACE, "'}'");
        return std::unique_ptr<Block>(std::move(b));
    }

    std::unique_ptr<Block> parseControlsBlock() {
        auto c = std::unique_ptr<ControlsBlock>(new ControlsBlock());
        const PToken& open = consume(PToken::Kind::LBRACE, "'{'");
        c->line = open.line; c->col = open.col;
        while (!check(PToken::Kind::RBRACE)) {
            c->props.push_back(parseAssign());
        }
        consume(PToken::Kind::RBRACE, "'}'");
        return std::unique_ptr<Block>(std::move(c));
    }

    std::unique_ptr<Block> parsePiecesBlock() {
        auto p = std::unique_ptr<PiecesBlock>(new PiecesBlock());
        const PToken& open = consume(PToken::Kind::LBRACE, "'{'");
        p->line = open.line; p->col = open.col;
        while (!check(PToken::Kind::RBRACE)) {
            if (check(PToken::Kind::AVAILABLE_PIECES)) {
                // available_pieces = array; (still allowed inside pieces for compat)
                auto asn = std::unique_ptr<Assign>(new Assign());
                const PToken& id = consume(PToken::Kind::AVAILABLE_PIECES, "available_pieces");
                asn->ident = id.lex; asn->line=id.line; asn->col=id.col;
                consume(PToken::Kind::ASSIGN, "'='");
                asn->value = parseValue();
                consume(PToken::Kind::SEMICOLON, "';'");
                p->extras.push_back(std::move(asn));
            } else if (check(PToken::Kind::IDENTIFIER)) {
                // piece_* { ... }
                const PToken& name = consume(PToken::Kind::IDENTIFIER, "piece identifier");
                auto def = std::unique_ptr<PieceDef>(new PieceDef());
                def->name = name.lex; def->line=name.line; def->col=name.col;
                consume(PToken::Kind::LBRACE, "'{'");
                while (!check(PToken::Kind::RBRACE)) {
                    def->props.push_back(parseAssign());
                }
                consume(PToken::Kind::RBRACE, "'}'");
                p->pieces.push_back(std::move(def));
            } else {
                throw expected("piece definition or 'available_pieces'");
            }
        }
        consume(PToken::Kind::RBRACE, "'}'");
        return std::unique_ptr<Block>(std::move(p));
    }

    std::unique_ptr<Block> parseRulesBlock() {
        const PToken& name = consume(PToken::Kind::RULES_PREFIX, "rules_* name");
        auto r = std::unique_ptr<RulesBlock>(new RulesBlock());
        r->name = name.lex; r->line=name.line; r->col=name.col;
        consume(PToken::Kind::LBRACE, "'{'");
        while (!check(PToken::Kind::RBRACE)) {
            r->props.push_back(parseAssign());
        }
        consume(PToken::Kind::RBRACE, "'}'");
        return std::unique_ptr<Block>(std::move(r));
    }

    std::unique_ptr<Block> parseGameSpecificBlock() {
        // name can be 'tetris'|'snake'|'level' or another IDENT used as a block name
        PToken nameTok;
        if (match(PToken::Kind::TETRIS) || match(PToken::Kind::SNAKE) || match(PToken::Kind::LEVEL) || match(PToken::Kind::IDENTIFIER)) {
            nameTok = prev();
        } else {
            throw expected("block name (tetris|snake|level|IDENT)");
        }
        auto g = std::unique_ptr<GameSpecificBlock>(new GameSpecificBlock());
        g->name = nameTok.lex; g->line=nameTok.line; g->col=nameTok.col;
        consume(PToken::Kind::LBRACE, "'{'");
        while (!check(PToken::Kind::RBRACE)) {
            g->props.push_back(parseAssign());
        }
        consume(PToken::Kind::RBRACE, "'}'");
        return std::unique_ptr<Block>(std::move(g));
    }

    std::unique_ptr<Block> parseTopLevelAssign() {
        // For statements like: available_pieces = [ ... ]; at top-level inside game { }
        const PToken& id = consume(PToken::Kind::AVAILABLE_PIECES, "available_pieces");
        auto a = std::unique_ptr<Assign>(new Assign());
        a->ident = id.lex; a->line=id.line; a->col=id.col;
        consume(PToken::Kind::ASSIGN, "'='");
        a->value = parseValue();
        consume(PToken::Kind::SEMICOLON, "';'");
        auto blk = std::unique_ptr<AssignBlock>(new AssignBlock());
        blk->line = id.line; blk->col = id.col;
        blk->stmt = std::move(a);
        return std::unique_ptr<Block>(std::move(blk));
    }

    // ---------- statements & values ----------
    std::unique_ptr<Assign> parseAssign() {
        const PToken& id = consume(PToken::Kind::IDENTIFIER, "identifier");
        auto a = std::unique_ptr<Assign>(new Assign());
        a->ident = id.lex; a->line=id.line; a->col=id.col;
        consume(PToken::Kind::ASSIGN, "'='");
        a->value = parseValue();
        consume(PToken::Kind::SEMICOLON, "';'");
        return a;
    }

    std::unique_ptr<Value> parseValue() {
        if (match(PToken::Kind::STRING)) {
            const PToken& t = prev();
            auto n = std::unique_ptr<VString>(new VString());
            n->v = t.lex; n->line=t.line; n->col=t.col; return std::unique_ptr<Value>(std::move(n));
        }
        if (match(PToken::Kind::INTEGER)) {
            const PToken& t = prev();
            auto n = std::unique_ptr<VInt>(new VInt());
            n->v = parseLong(t.lex); n->line=t.line; n->col=t.col; return std::unique_ptr<Value>(std::move(n));
        }
        if (match(PToken::Kind::FLOAT)) {
            const PToken& t = prev();
            auto n = std::unique_ptr<VFloat>(new VFloat());
            n->v = parseDouble(t.lex); n->line=t.line; n->col=t.col; return std::unique_ptr<Value>(std::move(n));
        }
        if (match(PToken::Kind::BOOLEAN)) {
            const PToken& t = prev();
            auto n = std::unique_ptr<VBool>(new VBool());
            n->v = (t.lex == "true"); n->line=t.line; n->col=t.col; return std::unique_ptr<Value>(std::move(n));
        }
        if (check(PToken::Kind::LBRACKET)) return parseArray();
        if (match(PToken::Kind::IDENTIFIER)) {
            const PToken& t = prev();
            auto n = std::unique_ptr<VIdent>(new VIdent());
            n->v = t.lex; n->line=t.line; n->col=t.col; return std::unique_ptr<Value>(std::move(n));
        }
        throw expected("value (string|int|float|bool|array|IDENT)");
    }

    std::unique_ptr<Value> parseArray() {
        consume(PToken::Kind::LBRACKET, "'['");
        auto arr = std::unique_ptr<VArray>(new VArray());
        if (!check(PToken::Kind::RBRACKET)) {
            arr->elems.push_back(parseValue());
            while (match(PToken::Kind::COMMA)) {
                arr->elems.push_back(parseValue());
            }
        }
        consume(PToken::Kind::RBRACKET, "']'");
        return std::unique_ptr<Value>(std::move(arr));
    }
};

static void indent(std::ostream& os, int n){ for(int i=0;i<n;++i) os<<' '; }

void dumpValue(const Value* v, std::ostream& os, int ind);

void dumpAssign(const Assign* a, std::ostream& os, int ind){
    indent(os, ind);
    os << a->ident << " = ";
    dumpValue(a->value.get(), os, ind);
    os << ";\n";
}

void dumpArray(const VArray* a, std::ostream& os, int ind){
    (void)ind;
    os << "[";
    for (size_t i=0;i<a->elems.size();++i){
        dumpValue(a->elems[i].get(), os, ind);
        if (i+1<a->elems.size()) os << ", ";
    }
    os << "]";
}

void dumpValue(const Value* v, std::ostream& os, int ind){
    if (const VInt* x = dynamic_cast<const VInt*>(v)) { os << x->v; return; }
    if (const VFloat* x = dynamic_cast<const VFloat*>(v)) { os << x->v; return; }
    if (const VBool* x = dynamic_cast<const VBool*>(v)) { os << (x->v?"true":"false"); return; }
    if (const VString* x = dynamic_cast<const VString*>(v)) { os << x->v; return; }
    if (const VIdent* x = dynamic_cast<const VIdent*>(v)) { os << x->v; return; }
    if (const VArray* x = dynamic_cast<const VArray*>(v)) { dumpArray(x, os, ind); return; }
    os << "<unknown>";
}

void dumpBlock(const Block* b, std::ostream& os, int ind){
    if (const BoardBlock* x = dynamic_cast<const BoardBlock*>(b)){
        indent(os, ind); os << "board {\n";
        for (auto& p: x->props) dumpAssign(p.get(), os, ind+2);
        indent(os, ind); os << "}\n";
        return;
    }
    if (const ControlsBlock* x = dynamic_cast<const ControlsBlock*>(b)){
        indent(os, ind); os << "controls {\n";
        for (auto& p: x->props) dumpAssign(p.get(), os, ind+2);
        indent(os, ind); os << "}\n";
        return;
    }
    if (const PiecesBlock* x = dynamic_cast<const PiecesBlock*>(b)){
        indent(os, ind); os << "pieces {\n";
        for (auto& e: x->extras) dumpAssign(e.get(), os, ind+2);
        for (auto& pc: x->pieces){
            indent(os, ind+2); os << pc->name << " {\n";
            for (auto& p: pc->props) dumpAssign(p.get(), os, ind+4);
            indent(os, ind+2); os << "}\n";
        }
        indent(os, ind); os << "}\n";
        return;
    }
    if (const RulesBlock* x = dynamic_cast<const RulesBlock*>(b)){
        indent(os, ind); os << x->name << " {\n";
        for (auto& p: x->props) dumpAssign(p.get(), os, ind+2);
        indent(os, ind); os << "}\n";
        return;
    }
    if (const GameSpecificBlock* x = dynamic_cast<const GameSpecificBlock*>(b)){
        indent(os, ind); os << x->name << " {\n";
        for (auto& p: x->props) dumpAssign(p.get(), os, ind+2);
        indent(os, ind); os << "}\n";
        return;
    }
    if (const AssignBlock* x = dynamic_cast<const AssignBlock*>(b)){
        dumpAssign(x->stmt.get(), os, ind);
        return;
    }
    indent(os, ind); os << "<unknown block>\n";
}

void dumpAST(const Program* p, std::ostream& os){
    os << p->game_id << " " << p->version << "\n";
    os << "game \"" << p->game->title << "\" {\n";
    for (auto& b: p->game->blocks) dumpBlock(b.get(), os, 2);
    os << "}\n";
}

enum class TokenType; // forward decl
class Token;          // forward decl

static PToken::Kind map_kind_from_lexer(TokenType t);

static std::vector<PToken> make_ptokens(const std::vector<Token>& in){
    std::vector<PToken> out;
    out.reserve(in.size()+1);
    PToken::Kind lastKind = PToken::Kind::END; // default
    for (const auto& tk : in){
        PToken pt;
        pt.kind = map_kind_from_lexer(tk.type);
        pt.lex  = tk.value;
        pt.line = tk.line;
        pt.col  = tk.column;
        lastKind = pt.kind;
        out.push_back(pt);
    }
    // Add synthetic END only if input didn't already end with END
    if (out.empty() || lastKind != PToken::Kind::END) {
        PToken end; end.kind=PToken::Kind::END; end.lex="<END>";
        end.line = in.empty()?0:in.back().line; end.col = in.empty()?0:in.back().column;
        out.push_back(end);
    }
    return out;
}

static PToken::Kind map_kind_from_lexer(TokenType t){
    const int v = (int) t;
    switch (v) {
        case 0: return PToken::Kind::INTEGER;
        case 1: return PToken::Kind::FLOAT;
        case 2: return PToken::Kind::STRING;
        case 3: return PToken::Kind::BOOLEAN;
        case 4: return PToken::Kind::GAME;
        case 5: return PToken::Kind::BOARD;
        case 6: return PToken::Kind::CONTROLS;
        case 7: return PToken::Kind::PIECES;
        case 8: return PToken::Kind::SNAKE;
        case 9: return PToken::Kind::TETRIS;
        case 10:return PToken::Kind::LEVEL;
        case 11:return PToken::Kind::IDENTIFIER;
        case 12:return PToken::Kind::VERSION;
        case 13:return PToken::Kind::RULES_PREFIX;
        case 14:return PToken::Kind::AVAILABLE_PIECES;
        case 15:return PToken::Kind::ASSIGN;
        case 16:return PToken::Kind::SEMICOLON;
        case 17:return PToken::Kind::LBRACE;
        case 18:return PToken::Kind::RBRACE;
        case 19:return PToken::Kind::LBRACKET;
        case 20:return PToken::Kind::RBRACKET;
        case 21:return PToken::Kind::COMMA;
        default:return PToken::Kind::END;
    }
}