
#pragma once

#include "ast.hpp"

#include <memory>
#include <string>
#include <vector>

namespace db {

enum class TokKind {
    IDENT, NUMBER, STRING, STAR, COMMA, LPAREN, RPAREN, SEMI,
    OP_EQ, OP_NEQ, OP_LT, OP_LE, OP_GT, OP_GE,
    KW_CREATE, KW_TABLE, KW_INSERT, KW_INTO, KW_VALUES,
    KW_DELETE, KW_FROM, KW_WHERE, KW_UPDATE, KW_SET, KW_SELECT,
    KW_AND, KW_OR,
    // Keywords added for join and persistence extensions
    KW_INNER, KW_JOIN, KW_ON, KW_DUMP, KW_LOAD,
    // Keyword for dropping tables
    KW_DROP,
    OP_ASSIGN, // alias for '=' when parsing SET
    DOT,       // dot used in table.column names
    END
};

struct Token {
    TokKind kind;
    std::string text;
};

struct TokenStream {
    std::vector<Token> toks;
    std::size_t i = 0;

    const Token& peek() const;
    const Token& get();
    bool match(TokKind k);
    void expect(TokKind k, const std::string& what);
    bool eof() const { return i >= toks.size() || toks[i].kind == TokKind::END; }
};

std::vector<std::string> split_statements(const std::string& input);
std::vector<Token> tokenize(const std::string& s);
std::unique_ptr<Statement> parse_statement(TokenStream& ts);

}
