#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include <cstring>

#include "lexer.c++"
#include "ast_parser.hpp"
#include "semantics.hpp"

// ---- JSON emit helpers ----------------------
static std::string json_escape(const std::string& s){
    std::string out; out.reserve(s.size()+8);
    for (size_t i=0;i<s.size();++i){
        unsigned char c = (unsigned char)s[i];
        switch(c){
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) { 
                    char buf[7]; 
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += s[i];
                }
        }
    }
    return out;
}

static void writeSymValueJSON(const SymValue& v, std::ostream& os);

static void writeArrayJSON(const std::vector<SymValue>& arr, std::ostream& os){
    os << "[";
    for (size_t i=0;i<arr.size();++i){
        if (i) os << ",";
        writeSymValueJSON(arr[i], os);
    }
    os << "]";
}

static void writeSymValueJSON(const SymValue& v, std::ostream& os){
    switch(v.kind){
        case SymValue::Kind::INT:    os << v.i; break;
        case SymValue::Kind::FLOAT:  os << v.f; break;
        case SymValue::Kind::BOOL:   os << (v.b ? "true" : "false"); break;
        case SymValue::Kind::STRING: os << "\"" << json_escape(v.s) << "\""; break;
        case SymValue::Kind::IDENT:  os << "{\"$ident\":\"" << json_escape(v.s) << "\"}"; break;
        case SymValue::Kind::ARRAY:  writeArrayJSON(v.arr, os); break;
    }
}

static void writeSymbolsJSON(const SymbolTable& T, std::ostream& os){
    os << "{\n  \"scopes\": {";
    bool firstScope = true;
    for (std::map<std::string, Scope>::const_iterator it = T.scopes.begin();
         it != T.scopes.end(); ++it) {
        if (!firstScope) os << ",";
        firstScope = false;
        os << "\n    \"" << json_escape(it->first) << "\": {";
        const Scope& sc = it->second;
        bool firstK = true;
        for (Scope::const_iterator jt = sc.begin(); jt != sc.end(); ++jt) {
            if (!firstK) os << ",";
            firstK = false;
            os << "\n      \"" << json_escape(jt->first) << "\": ";
            writeSymValueJSON(jt->second, os);
        }
        os << "\n    }";
    }
    os << "\n  }\n}\n";
}

static void printUsage(const char* argv0){
    std::cerr
      << "Usage: " << argv0 << " [--dump-ast] [--dump-symbols] [--json <file>] <file.brik>\n"
      << "Examples:\n"
      << "  " << argv0 << " --dump-ast tetris.brik\n"
      << "  " << argv0 << " --dump-symbols --json tetris.symbols.json tetris.brik\n";
}

int main(int argc, char** argv) {
    try {
        bool dump_ast = false;
        bool dump_symbols = false;
        const char* jsonOut = NULL;
        const char* path = NULL;

        for (int i=1;i<argc;++i){
            const char* a = argv[i];
            if (std::strcmp(a, "--dump-ast")==0) { dump_ast = true; continue; }
            if (std::strcmp(a, "--dump-symbols")==0) { dump_symbols = true; continue; }
            if (std::strcmp(a, "--json")==0) {
                if (i+1<argc) { jsonOut = argv[++i]; continue; }
                std::cerr << "--json requires a path\n"; return 1;
            }
            if (a[0]=='-') { printUsage(argv[0]); return 1; }
            if (!path) { path = a; } else { printUsage(argv[0]); return 1; }
        }
        if (!path){ printUsage(argv[0]); return 1; }

        std::ifstream f(path, std::ios::in | std::ios::binary);
        if (!f) { std::cerr << "Could not open: " << path << "\n"; return 1; }

        std::stringstream buf; buf << f.rdbuf();
        std::string source = buf.str();

        // 1) Lex + tokens
        Lexer lex(source);
        std::vector<Token> raw = lex.tokenize();
        std::vector<PToken> pt = make_ptokens(raw);

        // 2) Parse -> AST
        Parser parser(std::move(pt));
        std::unique_ptr<Program> ast = parser.parseProgram();

        // 3) AST dump 
        if (dump_ast) {
            dumpAST(ast.get(), std::cout);
        }

        // 4) Semantics + symbol table 
        std::vector<Diagnostic> diags;
        SymbolTable table = analyzeSemantics(ast.get(), diags);

        if (dump_symbols) {
            std::cout << "\n=== Symbol Table ===\n";
            printSymbols(table, std::cout);
        }

        if (jsonOut) {
            std::ofstream jf(jsonOut, std::ios::out | std::ios::binary);
            if (!jf) { std::cerr << "Could not open JSON output: " << jsonOut << "\n"; return 1; }
            writeSymbolsJSON(table, jf);
        }

        std::cerr << "=== Diagnostics ===\n";
        printDiagnostics(diags, std::cerr);

        bool hasError = false;
        for (size_t i=0;i<diags.size();++i) if (diags[i].kind=="error"){ hasError=true; break; }
        return hasError ? 2 : 0;

    } catch (const ParseError& e) {
        std::cerr << "Syntax error at (" << e.line << ":" << e.col << "): "
                  << e.what() << "\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 3;
    }
}
