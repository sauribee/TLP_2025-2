# -*- coding: utf-8 -*-
from __future__ import print_function

import lexer

class ParseError(Exception):
    def __init__(self, message, token):
        Exception.__init__(self, message)
        self.message = message
        self.token = token

    def __str__(self):
        if self.token is None:
            return self.message
        return "%s at line %d, col %d (token %r)" % (
            self.message, self.token.line, self.token.col, self.token.value
        )


class Parser(object):
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0
        self.current = tokens[0]

    # ---------- utilidades básicas ----------

    def _advance(self):
        tok = self.current
        self.pos += 1
        if self.pos < len(self.tokens):
            self.current = self.tokens[self.pos]
        else:
            self.current = self.tokens[-1]
        return tok

    def _expect(self, ttype):
        tok = self.current
        if tok.type != ttype:
            raise ParseError("Expected %s, got %s" % (ttype, tok.type), tok)
        self._advance()
        return tok

    def _expect_ident(self, value):
        tok = self.current
        if tok.type != 'IDENT' or tok.value != value:
            raise ParseError("Expected identifier %r" % value, tok)
        self._advance()
        return tok

    def _peek_type(self, offset):
        idx = self.pos + offset
        if 0 <= idx < len(self.tokens):
            return self.tokens[idx].type
        return 'EOF'

    # ---------- entrada principal ----------

    def parse_file(self):
        """
        file ::= header? 'game' STRING '{' section_body '}' EOF
        header ::= IDENT IDENT NUMBER     ; p.ej. "snake version 1.0"
        """
        result = {}

        # header opcional: e.g. "snake version 1.0"
        if (self.current.type == 'IDENT' and
            self._peek_type(1) == 'IDENT' and
            self._peek_type(2) == 'NUMBER'):
            kind_tok = self._advance()      # snake / tetris
            ver_kw  = self._advance()       # version
            ver_tok = self._advance()       # 1.0
            # si quieres ser estricto:
            # if ver_kw.value != 'version':
            #     raise ParseError("Expected 'version' keyword", ver_kw)
            result['kind'] = kind_tok.value
            result['version'] = self._convert_number(ver_tok.value)

        # game "name" { ... }
        self._expect_ident('game')
        name_tok = self._expect('STRING')
        result['name'] = name_tok.value

        self._expect('{')
        self._parse_section_body(result)
        self._expect('}')
        self._expect('EOF')
        return result

    # ---------- secciones y sentencias ----------

    def _parse_section_body(self, container):
        """
        section_body ::= ( assignment | subsection )*
        assignment   ::= IDENT '=' expr ';'
        subsection   ::= IDENT '{' section_body '}'
        """
        while self.current.type not in ('}', 'EOF'):
            if self.current.type != 'IDENT':
                raise ParseError("Expected identifier in section body", self.current)
            name_tok = self._advance()
            name = name_tok.value

            if self.current.type == '=':
                # assignment
                self._advance()
                value = self._parse_expr()
                self._expect(';')
                container[name] = value

            elif self.current.type == '{':
                # subsection
                self._advance()
                sub = {}
                self._parse_section_body(sub)
                self._expect('}')
                container[name] = sub

            else:
                raise ParseError('Expected "=" or "{" after identifier', self.current)

    # ---------- expresiones ----------

    def _parse_expr(self):
        """
        expr ::= NUMBER | STRING | COLOR | BOOL | list | IDENT
        BOOL ::= 'true' | 'false'
        """
        tok = self.current

        if tok.type == 'NUMBER':
            self._advance()
            return self._convert_number(tok.value)

        if tok.type == 'STRING':
            self._advance()
            return tok.value

        if tok.type == 'COLOR':
            self._advance()
            return tok.value

        if tok.type == 'IDENT':
            # booleanos simples
            if tok.value == 'true':
                self._advance()
                return True
            if tok.value == 'false':
                self._advance()
                return False
            # otros identificadores como valores simples (por ahora string)
            self._advance()
            return tok.value

        if tok.type == '[':
            return self._parse_list()

        raise ParseError("Unexpected token in expression", tok)

    def _parse_list(self):
        # current == '['
        self._expect('[')
        items = []
        if self.current.type != ']':
            items.append(self._parse_expr())
            while self.current.type == ',':
                self._advance()
                items.append(self._parse_expr())
        self._expect(']')
        return items

    def _convert_number(self, text):
        if '.' in text:
            try:
                return float(text)
            except ValueError:
                return text  # fallback: lo dejamos como string
        try:
            return int(text)
        except ValueError:
            return text


# ---------- funciones de conveniencia ----------

def parse_text(text):
    tokens = lexer.tokenize(text)
    p = Parser(tokens)
    return p.parse_file()


def parse_file(path):
    f = open(path, 'r')
    try:
        text = f.read()
    finally:
        f.close()
    return parse_text(text)


if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
        print("Usage: python parser.py <file.brik>")
        sys.exit(1)
    path = sys.argv[1]
    try:
        cfg = parse_file(path)
    except (lexer.LexError, ParseError) as e:
        print("ERROR:", e)
        sys.exit(1)
    print("OK. Parsed config:")
    import pprint
    pprint.pprint(cfg)
