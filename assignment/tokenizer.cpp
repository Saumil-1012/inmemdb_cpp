
#include "parser.hpp"

#include <cctype>
#include <stdexcept>

namespace db {

static bool is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
static bool is_ident_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

static void push_kw_or_ident(std::vector<Token>& toks, const std::string& t) {
    if (t == "CREATE") toks.push_back({TokKind::KW_CREATE, t});
    else if (t == "TABLE") toks.push_back({TokKind::KW_TABLE, t});
    else if (t == "INSERT") toks.push_back({TokKind::KW_INSERT, t});
    else if (t == "INTO") toks.push_back({TokKind::KW_INTO, t});
    else if (t == "VALUES") toks.push_back({TokKind::KW_VALUES, t});
    else if (t == "DELETE") toks.push_back({TokKind::KW_DELETE, t});
    else if (t == "FROM") toks.push_back({TokKind::KW_FROM, t});
    else if (t == "WHERE") toks.push_back({TokKind::KW_WHERE, t});
    else if (t == "UPDATE") toks.push_back({TokKind::KW_UPDATE, t});
    else if (t == "SET") toks.push_back({TokKind::KW_SET, t});
    else if (t == "SELECT") toks.push_back({TokKind::KW_SELECT, t});
    else if (t == "AND") toks.push_back({TokKind::KW_AND, t});
    else if (t == "OR") toks.push_back({TokKind::KW_OR, t});
    else if (t == "INNER") toks.push_back({TokKind::KW_INNER, t});
    else if (t == "JOIN") toks.push_back({TokKind::KW_JOIN, t});
    else if (t == "ON") toks.push_back({TokKind::KW_ON, t});
    else if (t == "DUMP") toks.push_back({TokKind::KW_DUMP, t});
    else if (t == "LOAD") toks.push_back({TokKind::KW_LOAD, t});
    else if (t == "DROP") toks.push_back({TokKind::KW_DROP, t});
    else toks.push_back({TokKind::IDENT, t});
}

std::vector<std::string> split_statements(const std::string& input) {
    std::vector<std::string> out;
    std::string cur;
    bool in_str = false;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '"') {
            in_str = !in_str;
            cur += c;
        } else if (c == ';' && !in_str) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::vector<Token> tokenize(const std::string& s) {
    std::vector<Token> toks;
    size_t i = 0;
    auto push = [&](TokKind k, char ch) { toks.push_back({k, std::string(1, ch)}); };
    while (i < s.size()) {
        char c = s[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }
        if (c == ',') {
            push(TokKind::COMMA, c);
            ++i;
            continue;
        }
        if (c == '(') {
            push(TokKind::LPAREN, c);
            ++i;
            continue;
        }
        if (c == ')') {
            push(TokKind::RPAREN, c);
            ++i;
            continue;
        }
        if (c == '*') {
            push(TokKind::STAR, c);
            ++i;
            continue;
        }
        if (c == ';') {
            push(TokKind::SEMI, c);
            ++i;
            continue;
        }
        if (c == '.') {
            // dot separates table and column names in JOIN and projection lists
            toks.push_back({TokKind::DOT, "."});
            ++i;
            continue;
        }
        if (c == '=') {
            toks.push_back({TokKind::OP_EQ, "="});
            ++i;
            continue;
        }
        if (c == '!') {
            if (i + 1 < s.size() && s[i + 1] == '=') {
                toks.push_back({TokKind::OP_NEQ, "!="});
                i += 2;
                continue;
            }
            throw std::runtime_error("Unexpected '!' (did you mean '!=' ?)");
        }
        if (c == '<') {
            if (i + 1 < s.size() && s[i + 1] == '=') {
                toks.push_back({TokKind::OP_LE, "<="});
                i += 2;
                continue;
            }
            toks.push_back({TokKind::OP_LT, "<"});
            ++i;
            continue;
        }
        if (c == '>') {
            if (i + 1 < s.size() && s[i + 1] == '=') {
                toks.push_back({TokKind::OP_GE, ">="});
                i += 2;
                continue;
            }
            toks.push_back({TokKind::OP_GT, ">"});
            ++i;
            continue;
        }
        if (c == '"') {
            // parse string until next quote
            ++i;
            std::string acc;
            while (i < s.size() && s[i] != '"') {
                acc += s[i];
                ++i;
            }
            if (i == s.size()) throw std::runtime_error("Unterminated string literal");
            ++i; // consume closing quote
            toks.push_back({TokKind::STRING, acc});
            continue;
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            // number literal (allow leading minus)
            size_t j = i;
            if (s[j] == '-') ++j;
            if (j >= s.size() || !std::isdigit(static_cast<unsigned char>(s[j]))) {
                throw std::runtime_error("Unexpected '-' (numbers must be like -42)");
            }
            while (j < s.size() && std::isdigit(static_cast<unsigned char>(s[j]))) ++j;
            toks.push_back({TokKind::NUMBER, s.substr(i, j - i)});
            i = j;
            continue;
        }
        if (is_ident_start(c)) {
            size_t j = i + 1;
            while (j < s.size() && is_ident_char(s[j])) ++j;
            std::string ident = s.substr(i, j - i);
            push_kw_or_ident(toks, ident);
            i = j;
            continue;
        }
        throw std::runtime_error(std::string("Unexpected character '") + c + "'");
    }
    toks.push_back({TokKind::END, ""});
    return toks;
}

// TokenStream methods
const Token& TokenStream::peek() const {
    return toks[i];
}

const Token& TokenStream::get() {
    return toks[i++];
}

bool TokenStream::match(TokKind k) {
    if (peek().kind == k) {
        ++i;
        return true;
    }
    return false;
}

void TokenStream::expect(TokKind k, const std::string& what) {
    if (!match(k)) throw std::runtime_error("Expected " + what);
}

// Forward declarations for recursive parser functions
static Value parse_value(TokenStream& ts);
static Condition::Op parse_op(TokenStream& ts);
static std::unique_ptr<Statement> parse_create(TokenStream& ts);
static std::unique_ptr<Statement> parse_insert(TokenStream& ts);
static std::unique_ptr<Statement> parse_delete(TokenStream& ts);
static std::unique_ptr<Statement> parse_update(TokenStream& ts);
static std::unique_ptr<Statement> parse_select(TokenStream& ts);
static std::unique_ptr<Statement> parse_dump(TokenStream& ts);
static std::unique_ptr<Statement> parse_load(TokenStream& ts);
static std::unique_ptr<Statement> parse_drop(TokenStream& ts); // forward decl

// Parse a literal value: number (int) or string (quoted)
static Value parse_value(TokenStream& ts) {
    const auto& t = ts.peek();
    if (t.kind == TokKind::NUMBER) {
        auto v = std::stoll(t.text);
        ts.get();
        return Value{Int(v)};
    }
    if (t.kind == TokKind::STRING) {
        auto s = t.text;
        ts.get();
        return Value{Str(s)};
    }
    throw std::runtime_error("Expected literal value (number or \"string\")");
}

// Parse a comparison operator in a WHERE clause
static Condition::Op parse_op(TokenStream& ts) {
    if (ts.match(TokKind::OP_EQ)) return Condition::Op::EQ;
    if (ts.match(TokKind::OP_NEQ)) return Condition::Op::NEQ;
    if (ts.match(TokKind::OP_LT)) return Condition::Op::LT;
    if (ts.match(TokKind::OP_LE)) return Condition::Op::LE;
    if (ts.match(TokKind::OP_GT)) return Condition::Op::GT;
    if (ts.match(TokKind::OP_GE)) return Condition::Op::GE;
    throw std::runtime_error("Expected comparison operator (=, !=, <, <=, >, >=)");
}

static Condition parse_where(TokenStream& ts) {
    Condition cond;
    if (!ts.match(TokKind::KW_WHERE)) return cond; // empty condition
    if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name in WHERE");
    cond.column = ts.get().text;
    cond.op = parse_op(ts);
    cond.literal = parse_value(ts);
    return cond;
}

// Parse a CREATE TABLE statement.
static std::unique_ptr<Statement> parse_create(TokenStream& ts) {
    ts.expect(TokKind::KW_CREATE, "CREATE");
    ts.expect(TokKind::KW_TABLE, "TABLE");
    if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected table name");
    auto name = ts.get().text;
    ts.expect(TokKind::LPAREN, "(");
    std::vector<Column> cols;
    bool first = true;
    while (!ts.match(TokKind::RPAREN)) {
        if (!first) ts.expect(TokKind::COMMA, ",");
        first = false;
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name");
        std::string cname = ts.get().text;
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected type (int/str)");
        auto tname = ts.get().text;
        Column c{cname, parse_type(tname)};
        cols.push_back(c);
    }
    auto out = std::make_unique<CreateTable>();
    out->table = name;
    out->columns = std::move(cols);
    return out;
}

// Parse an INSERT INTO statement. Supports multiple rows in one VALUES clause.
static std::unique_ptr<Statement> parse_insert(TokenStream& ts) {
    ts.expect(TokKind::KW_INSERT, "INSERT");
    ts.expect(TokKind::KW_INTO, "INTO");
    if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected table name");
    auto table = ts.get().text;
    ts.expect(TokKind::LPAREN, "(");
    std::vector<std::string> cols;
    bool first = true;
    while (!ts.match(TokKind::RPAREN)) {
        if (!first) ts.expect(TokKind::COMMA, ",");
        first = false;
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name");
        cols.push_back(ts.get().text);
    }
    ts.expect(TokKind::KW_VALUES, "VALUES");
    std::vector<std::vector<Value>> rows;
    do {
        ts.expect(TokKind::LPAREN, "(");
        std::vector<Value> vals;
        bool firstv = true;
        while (!ts.match(TokKind::RPAREN)) {
            if (!firstv) ts.expect(TokKind::COMMA, ",");
            firstv = false;
            vals.push_back(parse_value(ts));
        }
        rows.push_back(std::move(vals));
    } while (ts.match(TokKind::COMMA));
    auto out = std::make_unique<Insert>();
    out->table = table;
    out->columns = std::move(cols);
    out->values_lists = std::move(rows);
    return out;
}

// Parse a DELETE statement. May have an optional WHERE clause.
static std::unique_ptr<Statement> parse_delete(TokenStream& ts) {
    ts.expect(TokKind::KW_DELETE, "DELETE");
    ts.expect(TokKind::KW_FROM, "FROM");
    if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected table name");
    auto table = ts.get().text;
    Condition cond;
    std::optional<Condition> where;
    if (ts.match(TokKind::KW_WHERE)) {
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name in WHERE");
        cond.column = ts.get().text;
        cond.op = parse_op(ts);
        cond.literal = parse_value(ts);
        where = cond;
    }
    auto out = std::make_unique<Delete>();
    out->table = table;
    out->where = where;
    return out;
}

// Parse an UPDATE statement with optional WHERE clause.
static std::unique_ptr<Statement> parse_update(TokenStream& ts) {
    ts.expect(TokKind::KW_UPDATE, "UPDATE");
    if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected table name");
    auto table = ts.get().text;
    ts.expect(TokKind::KW_SET, "SET");
    std::vector<std::pair<std::string, Value>> assigns;
    bool first = true;
    while (true) {
        if (!first) ts.expect(TokKind::COMMA, ",");
        first = false;
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column in SET");
        std::string cname = ts.get().text;
        if (!ts.match(TokKind::OP_EQ)) throw std::runtime_error("Expected '=' in SET");
        Value v = parse_value(ts);
        assigns.emplace_back(cname, v);
        if (ts.peek().kind != TokKind::COMMA) break;
    }
    std::optional<Condition> where;
    if (ts.match(TokKind::KW_WHERE)) {
        Condition cond;
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name in WHERE");
        cond.column = ts.get().text;
        cond.op = parse_op(ts);
        cond.literal = parse_value(ts);
        where = cond;
    }
    auto out = std::make_unique<Update>();
    out->table = table;
    out->assignments = std::move(assigns);
    out->where = where;
    return out;
}

// Parse a SELECT statement with optional projection list and WHERE clause.
static std::unique_ptr<Statement> parse_select(TokenStream& ts) {
    ts.expect(TokKind::KW_SELECT, "SELECT");
    Projection proj;
    // Parse projection list. If '*' is present, leave columns empty for wildcard.
    if (ts.match(TokKind::STAR)) {
        // wildcard
    } else {
        bool first = true;
        while (true) {
            if (!first) ts.expect(TokKind::COMMA, ",");
            first = false;
            // Expect an identifier for column (optionally table.column)
            if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name in SELECT");
            std::string col = ts.get().text;
            // Support table.column notation
            if (ts.match(TokKind::DOT)) {
                if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name after '.' in SELECT");
                std::string sub = ts.get().text;
                col += "." + sub;
            }
            proj.columns.push_back(col);
            if (ts.peek().kind != TokKind::COMMA) break;
        }
    }
    ts.expect(TokKind::KW_FROM, "FROM");
    if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected table name after FROM");
    auto table = ts.get().text;
    std::optional<JoinClause> join;
    // Check for optional INNER JOIN clause
    if (ts.match(TokKind::KW_INNER)) {
        ts.expect(TokKind::KW_JOIN, "JOIN");
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected table name in JOIN");
        std::string right_table = ts.get().text;
        ts.expect(TokKind::KW_ON, "ON");
        // left side: table.column
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected table name in ON clause");
        std::string left_table = ts.get().text;
        ts.expect(TokKind::DOT, ".");
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name after '.' in ON clause");
        std::string left_col = ts.get().text;
        // equality operator
        if (!ts.match(TokKind::OP_EQ)) throw std::runtime_error("Expected '=' in ON clause");
        // right side: table.column
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected table name in ON clause");
        std::string r_table = ts.get().text;
        ts.expect(TokKind::DOT, ".");
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name after '.' in ON clause");
        std::string right_col = ts.get().text;
        JoinClause jc;
        jc.table = right_table;
        jc.left_table = left_table;
        jc.left_column = left_col;
        jc.right_table = r_table;
        jc.right_column = right_col;
        join = jc;
    }
    std::optional<Condition> where;
    // Parse optional WHERE clause (supporting table.column syntax)
    if (ts.match(TokKind::KW_WHERE)) {
        Condition cond;
        if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name in WHERE");
        std::string col = ts.get().text;
        if (ts.match(TokKind::DOT)) {
            if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected column name after '.' in WHERE");
            std::string sub = ts.get().text;
            col += "." + sub;
        }
        cond.column = col;
        cond.op = parse_op(ts);
        cond.literal = parse_value(ts);
        where = cond;
    }
    auto out = std::make_unique<Select>();
    out->projection = proj;
    out->table = table;
    out->join = join;
    out->where = where;
    return out;
}

// Parse a DUMP statement. Syntax: DUMP "filename"
static std::unique_ptr<Statement> parse_dump(TokenStream& ts) {
    ts.expect(TokKind::KW_DUMP, "DUMP");
    if (ts.peek().kind != TokKind::STRING) {
        throw std::runtime_error("Expected file name in DUMP");
    }
    auto filename = ts.get().text;
    auto out = std::make_unique<Dump>();
    out->filename = filename;
    return out;
}

// Parse a LOAD statement. Syntax: LOAD "filename"
static std::unique_ptr<Statement> parse_load(TokenStream& ts) {
    ts.expect(TokKind::KW_LOAD, "LOAD");
    if (ts.peek().kind != TokKind::STRING) {
        throw std::runtime_error("Expected file name in LOAD");
    }
    auto filename = ts.get().text;
    auto out = std::make_unique<Load>();
    out->filename = filename;
    return out;
}

// Parse a DROP statement. Syntax: DROP TABLE table_name
static std::unique_ptr<Statement> parse_drop(TokenStream& ts) {
    ts.expect(TokKind::KW_DROP, "DROP");
    ts.expect(TokKind::KW_TABLE, "TABLE");
    if (ts.peek().kind != TokKind::IDENT) throw std::runtime_error("Expected table name after DROP TABLE");
    auto name = ts.get().text;
    auto out = std::make_unique<DropTable>();
    out->table = name;
    return out;
}

// Top-level parser dispatch. Determine which statement to parse based on the
// leading keyword.
std::unique_ptr<Statement> parse_statement(TokenStream& ts) {
    switch (ts.peek().kind) {
        case TokKind::KW_CREATE: return parse_create(ts);
        case TokKind::KW_INSERT: return parse_insert(ts);
        case TokKind::KW_DELETE: return parse_delete(ts);
         case TokKind::KW_UPDATE: return parse_update(ts);
        case TokKind::KW_SELECT: return parse_select(ts);
        case TokKind::KW_DUMP: return parse_dump(ts);
        case TokKind::KW_LOAD: return parse_load(ts);
        case TokKind::KW_DROP: return parse_drop(ts);
        default:
            throw std::runtime_error("Expected a statement (CREATE/INSERT/DELETE/UPDATE/SELECT/DUMP/LOAD/DROP)");
    }
}

}
