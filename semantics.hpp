#pragma once
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "ast_parser.hpp"

// Messages (errors/warnings) with source location
struct Diagnostic {
    int line = 0, col = 0;
    std::string kind; // "error" | "warning"
    std::string msg;
};

// Helper to add diagnostics (C++11-friendly)
static void addDiag(std::vector<Diagnostic>& D, int line, int col,
                    const std::string& kind, const std::string& msg) {
    Diagnostic d;
    d.line = line; d.col = col;
    d.kind = kind; d.msg = msg;
    D.push_back(d);
}

struct SymbolTable {
    // One "scope" per block or sub-block (board, controls, pieces, piece:<name>, rules:<name>, game, etc.)
    std::map<std::string, Scope> scopes;

    Scope& scope(const std::string& name) { return scopes[name]; }
    const Scope* find(const std::string& name) const {
        std::map<std::string, Scope>::const_iterator it = scopes.find(name);
        return (it==scopes.end()? NULL : &it->second);
    }
};

// Helpers ------------------------------------------------------

static std::string unquote_str(const std::string& s) {
    if (s.size()>=2 && s.front()=='"' && s.back()=='"') return s.substr(1, s.size()-2);
    return s;
}

static SymValue toSymValue(const Value* v) {
    SymValue sv;
    if (const VInt* p = dynamic_cast<const VInt*>(v)) {
        sv.kind = SymValue::Kind::INT; sv.i = p->v; return sv;
    }
    if (const VFloat* p = dynamic_cast<const VFloat*>(v)) {
        sv.kind = SymValue::Kind::FLOAT; sv.f = p->v; return sv;
    }
    if (const VBool* p = dynamic_cast<const VBool*>(v)) {
        sv.kind = SymValue::Kind::BOOL; sv.b = p->v; return sv;
    }
    if (const VString* p = dynamic_cast<const VString*>(v)) {
        sv.kind = SymValue::Kind::STRING; sv.s = unquote_str(p->v); return sv;
    }
    if (const VIdent* p = dynamic_cast<const VIdent*>(v)) {
        sv.kind = SymValue::Kind::IDENT; sv.s = p->v; return sv;
    }
    if (const VArray* p = dynamic_cast<const VArray*>(v)) {
        sv.kind = SymValue::Kind::ARRAY;
        sv.arr.reserve(p->elems.size());
        for (size_t i=0;i<p->elems.size();++i) sv.arr.push_back(toSymValue(p->elems[i].get()));
        return sv;
    }
    // fallback
    sv.kind = SymValue::Kind::IDENT; sv.s = "<unknown>";
    return sv;
}

static void putKV(Scope& sc, const std::string& key, SymValue val,
                  int line, int col, std::vector<Diagnostic>& diags) {
    if (sc.count(key)) {
        addDiag(diags, line, col, "warning",
                std::string("redefinition of '") + key + "', overwriting previous value");
    }
    sc[key] = std::move(val);
}

static void collectAssigns(const std::vector<std::unique_ptr<Assign>>& props,
                           Scope& sc, std::vector<Diagnostic>& diags) {
    for (size_t i=0;i<props.size();++i) {
        const Assign* a = props[i].get();
        putKV(sc, a->ident, toSymValue(a->value.get()), a->line, a->col, diags);
    }
}

// Symbol table construction ------------------------------------

static SymbolTable buildSymbols(const Program* p, std::vector<Diagnostic>& diags) {
    SymbolTable T;

    // Scope "game": metadata + top-level single assignments (AssignBlock)
    Scope& g = T.scope("game");
    {
        SymValue title; title.kind=SymValue::Kind::STRING; title.s = p->game->title;
        g["title"] = title;
        SymValue ver; ver.kind=SymValue::Kind::FLOAT; ver.f = p->version;
        g["version"] = ver;
        SymValue id; id.kind=SymValue::Kind::IDENT; id.s = p->game_id;
        g["id"] = id;
    }

    for (size_t i=0;i<p->game->blocks.size();++i) {
        Block* b = p->game->blocks[i].get();
        if (BoardBlock* x = dynamic_cast<BoardBlock*>(b)) {
            Scope& sc = T.scope("board");
            collectAssigns(x->props, sc, diags);
        } else if (ControlsBlock* x = dynamic_cast<ControlsBlock*>(b)) {
            Scope& sc = T.scope("controls");
            collectAssigns(x->props, sc, diags);
        } else if (PiecesBlock* x = dynamic_cast<PiecesBlock*>(b)) {
            Scope& scPieces = T.scope("pieces");
            collectAssigns(x->extras, scPieces, diags); // optional available_pieces inside pieces
            for (size_t j=0;j<x->pieces.size();++j) {
                PieceDef* pd = x->pieces[j].get();
                Scope& sc = T.scope(std::string("piece:") + pd->name);
                collectAssigns(pd->props, sc, diags);
            }
        } else if (RulesBlock* x = dynamic_cast<RulesBlock*>(b)) {
            Scope& sc = T.scope(std::string("rules:") + x->name);
            collectAssigns(x->props, sc, diags);
        } else if (GameSpecificBlock* x = dynamic_cast<GameSpecificBlock*>(b)) {
            Scope& sc = T.scope(x->name); // "tetris", "snake", "level", etc.
            collectAssigns(x->props, sc, diags);
        } else if (AssignBlock* x = dynamic_cast<AssignBlock*>(b)) {
            // Single assignment at game { } level
            putKV(g, x->stmt->ident, toSymValue(x->stmt->value.get()), x->stmt->line, x->stmt->col, diags);
        }
    }
    return T;
}

// Basic semantic checks ---------------------------------------

static bool expectInt  (const SymValue& v){ return v.kind==SymValue::Kind::INT; }
static bool expectFloat(const SymValue& v){ return v.kind==SymValue::Kind::FLOAT; }
static bool expectString(const SymValue& v){ return v.kind==SymValue::Kind::STRING; }
static bool expectIdent(const SymValue& v){ return v.kind==SymValue::Kind::IDENT; }
static bool expectArray(const SymValue& v){ return v.kind==SymValue::Kind::ARRAY; }

static void checkBoard(const SymbolTable& T, std::vector<Diagnostic>& D){
    const Scope* scp = T.find("board"); if (!scp) return;
    Scope::const_iterator itW = scp->find("width");
    Scope::const_iterator itH = scp->find("height");
    if (itW!=scp->end() && !expectInt(itW->second))
        addDiag(D, 0, 0, "error", "board.width must be integer");
    if (itH!=scp->end() && !expectInt(itH->second))
        addDiag(D, 0, 0, "error", "board.height must be integer");
    if (itW!=scp->end() && expectInt(itW->second) && itW->second.i<=0)
        addDiag(D, 0, 0, "error", "board.width must be > 0");
    if (itH!=scp->end() && expectInt(itH->second) && itH->second.i<=0)
        addDiag(D, 0, 0, "error", "board.height must be > 0");
}

static bool isInt4x4(const SymValue& v){
    if (!expectArray(v) || v.arr.size()!=4) return false;
    for (size_t r=0;r<4;++r){
        const SymValue& row = v.arr[r];
        if (!expectArray(row) || row.arr.size()!=4) return false;
        for (size_t c=0;c<4;++c){
            if (!expectInt(row.arr[c])) return false;
        }
    }
    return true;
}

static void checkPieces(const SymbolTable& T, std::vector<Diagnostic>& D){
    // Collect defined piece names
    std::set<std::string> defined;
    for (std::map<std::string, Scope>::const_iterator it = T.scopes.begin();
         it != T.scopes.end(); ++it) {
        const std::string& scopeName = it->first;
        if (scopeName.size() >= 6 && scopeName.compare(0,6,"piece:")==0) {
            defined.insert(scopeName.substr(6));
        }
    }

    // available_pieces may live in "game" or "pieces"
    const Scope* whereAP = NULL;
    const Scope* scGame = T.find("game");
    if (scGame) {
        if (scGame->find("available_pieces") != scGame->end())
            whereAP = scGame;
    }
    if (!whereAP) {
        const Scope* scPieces = T.find("pieces");
        if (scPieces && scPieces->find("available_pieces") != scPieces->end())
            whereAP = scPieces;
    }

    if (whereAP) {
        Scope::const_iterator it = whereAP->find("available_pieces");
        if (!expectArray(it->second)) {
            addDiag(D, 0, 0, "error", "available_pieces must be an array");
        } else {
            for (size_t i=0; i<it->second.arr.size(); ++i) {
                const SymValue& elem = it->second.arr[i];
                std::string name;
                if (expectString(elem) || expectIdent(elem)) {
                    name = elem.s;
                } else {
                    addDiag(D, 0, 0, "error", "available_pieces only accepts strings or idents");
                    continue;
                }
                if (defined.find(name) == defined.end()) {
                    addDiag(D, 0, 0, "error",
                        std::string("available_pieces references undefined piece: ")+name);
                }
            }
        }
    }

    // Validate rotations 4x4 (each rotation must be a 4x4 matrix of ints)
    for (std::set<std::string>::const_iterator itn = defined.begin();
         itn != defined.end(); ++itn) {
        const std::string& nm = *itn;
        const Scope* scp = T.find(std::string("piece:") + nm);
        if (!scp) continue;
        Scope::const_iterator itR = scp->find("rotations");
        if (itR == scp->end()) continue;
        if (!expectArray(itR->second)) {
            addDiag(D, 0, 0, "error", std::string("rotations of ")+nm+" must be an array");
            continue;
        }
        bool ok = true;
        for (size_t r=0; r<itR->second.arr.size(); ++r) {
            if (!isInt4x4(itR->second.arr[r])) { ok=false; break; }
        }
        if (!ok) {
            addDiag(D, 0, 0, "error",
                std::string("rotations of ")+nm+" must contain 4x4 integer matrices");
        }
    }
}

static void checkGameSpecific(const SymbolTable& T, const Program* p, std::vector<Diagnostic>& D){
    if (p->game_id == "snake") {
        const Scope* sc = T.find("level");
        if (sc) {
            Scope::const_iterator it = sc->find("speed");
            if (it!=sc->end() && !expectInt(it->second))
                addDiag(D, 0, 0, "error", "level.speed (snake) must be integer");
        }
    }
    if (p->game_id == "tetris") {
        const Scope* sc = T.find("tetris");
        if (sc) {
            Scope::const_iterator itg = sc->find("gravity");
            if (itg!=sc->end() && !(expectFloat(itg->second) || expectInt(itg->second)))
                addDiag(D, 0, 0, "error", "tetris.gravity must be numeric");
        }
    }
}

// Orchestrator -------------------------------------------------

static SymbolTable analyzeSemantics(const Program* p, std::vector<Diagnostic>& D) {
    SymbolTable T = buildSymbols(p, D);
    checkBoard(T, D);
    checkPieces(T, D);
    checkGameSpecific(T, p, D);
    return T;
}

// Pretty-printing ---------------------------------------------

static void printSymbols(const SymbolTable& T, std::ostream& os) {
    for (std::map<std::string, Scope>::const_iterator it = T.scopes.begin();
         it != T.scopes.end(); ++it) {
        const std::string& scopeName = it->first;
        const Scope& sc = it->second;
        os << "[" << scopeName << "]\n";
        for (Scope::const_iterator jt = sc.begin(); jt != sc.end(); ++jt) {
            const std::string& k = jt->first;
            const SymValue& v = jt->second;
            os << "  " << k << " = ";
            switch (v.kind) {
                case SymValue::Kind::INT:    os << v.i; break;
                case SymValue::Kind::FLOAT:  os << v.f; break;
                case SymValue::Kind::BOOL:   os << (v.b?"true":"false"); break;
                case SymValue::Kind::STRING: os << '"' << v.s << '"'; break;
                case SymValue::Kind::IDENT:  os << v.s; break;
                case SymValue::Kind::ARRAY:  os << "[...](" << v.arr.size() << ")"; break;
            }
            os << "\n";
        }
        os << "\n";
    }
}

static void printDiagnostics(const std::vector<Diagnostic>& D, std::ostream& os) {
    if (D.empty()) { os << "Semantics: 0 issues\n"; return; }
    for (size_t i=0;i<D.size();++i) {
        const Diagnostic& d = D[i];
        os << d.kind << " (" << d.line << ":" << d.col << "): " << d.msg << "\n";
    }
}