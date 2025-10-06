#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include <iomanip>
#include "lexer.c++"
#include "ast_parser.hpp"
#include "semantics.hpp"

static void jsonIndent(std::ostream& os, int n){ while(n--) os.put(' '); }

static void jsonEscape(const std::string& s, std::ostream& os){
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        switch (c) {
            case '\"': os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\b': os << "\\b";  break;
            case '\f': os << "\\f";  break;
            case '\n': os << "\\n";  break;
            case '\r': os << "\\r";  break;
            case '\t': os << "\\t";  break;
            default:
                if (c < 0x20) {
                    const char* hex = "0123456789ABCDEF";
                    os << "\\u00" << hex[(c >> 4) & 0xF] << hex[c & 0xF];
                } else {
                    os << s[i];
                }
        }
    }
}

static void writeSymValueJSON(const SymValue& v, std::ostream& os, int ind);

static void writeArrayJSON(const std::vector<SymValue>& arr, std::ostream& os, int ind){
    os << "[";
    for (size_t i = 0; i < arr.size(); ++i){
        if (i) os << ", ";
        writeSymValueJSON(arr[i], os, ind);
    }
    os << "]";
}

static void writeSymValueJSON(const SymValue& v, std::ostream& os, int /*ind*/){
    switch (v.kind){
        case SymValue::Kind::INT:    os << v.i; break;
        case SymValue::Kind::FLOAT:  os << v.f; break;
        case SymValue::Kind::BOOL:   os << (v.b ? "true" : "false"); break;
        case SymValue::Kind::STRING: os << '\"'; jsonEscape(v.s, os); os << '\"'; break;
        case SymValue::Kind::IDENT:  os << '\"'; jsonEscape(v.s, os); os << '\"'; break;
        case SymValue::Kind::ARRAY:  writeArrayJSON(v.arr, os, 0); break;
    }
}

static void writeSymbolsJSON(const SymbolTable& T, std::ostream& os){
    os << "{\n";
    jsonIndent(os, 2); os << "\"scopes\": {\n";

    size_t sidx = 0, scCount = T.scopes.size();
    for (std::map<std::string, Scope>::const_iterator it = T.scopes.begin();
         it != T.scopes.end(); ++it, ++sidx)
    {
        const std::string& scopeName = it->first;
        const Scope& sc = it->second;

        jsonIndent(os, 4); os << '\"'; jsonEscape(scopeName, os); os << "\": {\n";

        size_t kidx = 0, kCount = sc.size();
        for (Scope::const_iterator jt = sc.begin(); jt != sc.end(); ++jt, ++kidx) {
            const std::string& key = jt->first;
            const SymValue& val = jt->second;

            jsonIndent(os, 6);
            os << '\"'; jsonEscape(key, os); os << "\": ";
            writeSymValueJSON(val, os, 6);
            if (kidx + 1 < kCount) os << ",";
            os << "\n";
        }

        jsonIndent(os, 4); os << "}";
        if (sidx + 1 < scCount) os << ",";
        os << "\n";
    }

    jsonIndent(os, 2); os << "}\n";
    os << "}\n";
}

static void printUsage(const char* exe){
    std::cerr << "Usage:\n"
              << "  " << exe << " [--dump-ast] [--dump-symbols] [--json <file>] [--no-diag] <file.brik>\n"
              << "Examples:\n"
              << "  " << exe << " --dump-ast .\\games\\tetris.brik\n"
              << "  " << exe << " --dump-symbols --json .\\out\\snake.symbols.json .\\games\\snake.brik\n";
}

int main(int argc, char** argv) {
    try {
        // ---- parse flags ----
        bool dump_ast = false;
        bool dump_symbols = false;
        bool no_diag = false;
        const char* jsonOut = NULL;
        const char* input = NULL;

        for (int i = 1; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--dump-ast") { dump_ast = true; }
            else if (a == "--dump-symbols") { dump_symbols = true; }
            else if (a == "--no-diag") { no_diag = true; }
            else if (a == "--json") {
                if (i + 1 < argc) { jsonOut = argv[++i]; }
                else {
                    std::cerr << "[Error] --json needs an output file path\n";
                    printUsage(argv[0]); return 1;
                }
            } else if (a.size() >= 6 && a.substr(a.size()-6) == ".brik") {
                input = argv[i];
            } else if (a.size() >= 2 && a[0] != '-') {
                input = argv[i]; // allow plain file without .brik suffix
            } else {
                // unknown flag
                std::cerr << "[Error] Unknown option: " << a << "\n";
                printUsage(argv[0]); return 1;
            }
        }
        if (!input) { printUsage(argv[0]); return 1; }

        // ---- read source ----
        std::ifstream f(input, std::ios::in | std::ios::binary);
        if (!f) { std::cerr << "Cannot open: " << input << "\n"; return 1; }
        std::stringstream buf; buf << f.rdbuf();
        std::string source = buf.str();

        // ---- lex + parse ----
        Lexer lex(source);
        std::vector<Token> raw = lex.tokenize();
        std::vector<PToken> pt = make_ptokens(raw);
        Parser parser(std::move(pt));
        std::unique_ptr<Program> ast = parser.parseProgram();

        // ---- semantics ----
        std::vector<Diagnostic> diags;
        SymbolTable table = analyzeSemantics(ast.get(), diags);

        // ---- outputs controlled by flags ----
        if (dump_ast) {
            dumpAST(ast.get(), std::cout);
        }
        if (dump_symbols) {
            std::cout << "\n=== Symbol Table ===\n";
            printSymbols(table, std::cout);
        }
        if (jsonOut) {
            std::ofstream jf(jsonOut, std::ios::binary);
            if (!jf) { std::cerr << "Could not open JSON output file: " << jsonOut << "\n"; return 1; }
            writeSymbolsJSON(table, jf);   // <--- now the function is USED
        }

        // diagnostics only if there are any (and not silenced)
        if (!no_diag && !diags.empty()) {
            std::cerr << "=== Diagnostics ===\n";
            printDiagnostics(diags, std::cerr);
        }

        // exit code reflects semantic errors only
        bool hasError = false;
        for (size_t i = 0; i < diags.size(); ++i) {
            if (diags[i].kind == "error") { hasError = true; break; }
        }
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
