#include "database.hpp"
#include "parser.hpp"

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cctype>

namespace db {

std::size_t Table::col_index(const std::string& c) const {
    auto it = index.find(c);
    if (it == index.end()) throw std::runtime_error("Unknown column '" + c + "'");
    return it->second;
}

Type Table::col_type(const std::string& c) const {
    return schema[col_index(c)].type;
}

Table& Database::create_table(const std::string& name, const std::vector<Column>& cols) {
    if (tables_.count(name)) throw std::runtime_error("Table '" + name + "' already exists");
    Table t;
    t.name = name;
    t.schema = cols;
    for (std::size_t i = 0; i < cols.size(); ++i) {
        t.index.emplace(cols[i].name, i);
    }
    tables_.emplace(name, std::move(t));
    return tables_.at(name);
}

Table& Database::table(const std::string& name) {
    auto it = tables_.find(name);
    if (it == tables_.end()) throw std::runtime_error("Unknown table '" + name + "'");
    return it->second;
}

const Table& Database::table(const std::string& name) const {
    auto it = tables_.find(name);
    if (it == tables_.end()) throw std::runtime_error("Unknown table '" + name + "'");
    return it->second;
}

void Database::exec(const CreateTable& s) {
    create_table(s.table, s.columns);
}

void Database::exec(const Insert& s) {
    auto& t = table(s.table);
    // Determine mapping from insertion columns to schema indices
    std::vector<std::size_t> map_idx;
    map_idx.reserve(s.columns.size());
    for (auto& cname : s.columns) {
        map_idx.push_back(t.col_index(cname));
    }
    for (auto& vals : s.values_lists) {
        if (vals.size() != s.columns.size())
            throw std::runtime_error("INSERT values count does not match column list");
        Row r;
        r.cells.resize(t.schema.size());
        // Fill with defaults
        for (std::size_t i = 0; i < t.schema.size(); ++i) {
            r.cells[i] = default_value(t.schema[i].type);
        }
        // Copy provided values into the correct positions
        for (std::size_t i = 0; i < vals.size(); ++i) {
            auto tgt = map_idx[i];
            auto expected = t.schema[tgt].type;
            const Value& v = vals[i];
            // Type check each value
            if ((expected == Type::INT && !std::holds_alternative<Int>(v)) ||
                (expected == Type::STR && !std::holds_alternative<Str>(v))) {
                throw std::runtime_error("INSERT type mismatch for column '" + t.schema[tgt].name + "'");
            }
            r.cells[tgt] = v;
        }
        t.rows.push_back(std::move(r));
    }
}

// ---- WHERE helpers -------------------------------------------------

static bool eval_cond(const Table& t, const Row& r, const Condition& c) {
    auto col = t.col_index(c.column);
    const auto& v = r.cells[col];
    if (std::holds_alternative<Int>(v)) {
        auto lhs = std::get<Int>(v);
        if (std::holds_alternative<Int>(c.literal)) {
            auto rhs = std::get<Int>(c.literal);
            switch (c.op) {
                case Condition::Op::EQ: return lhs == rhs;
                case Condition::Op::NEQ: return lhs != rhs;
                case Condition::Op::LT: return lhs < rhs;
                case Condition::Op::LE: return lhs <= rhs;
                case Condition::Op::GT: return lhs > rhs;
                case Condition::Op::GE: return lhs >= rhs;
            }
        } else {
            throw std::runtime_error("WHERE type mismatch: comparing int column to string");
        }
    } else {
        auto lhs = std::get<Str>(v);
        if (!std::holds_alternative<Str>(c.literal)) {
            throw std::runtime_error("WHERE type mismatch: comparing string column to int");
        }
        auto rhs = std::get<Str>(c.literal);
        switch (c.op) {
            case Condition::Op::EQ: return lhs == rhs;
            case Condition::Op::NEQ: return lhs != rhs;
            case Condition::Op::LT: return lhs < rhs;
            case Condition::Op::LE: return lhs <= rhs;
            case Condition::Op::GT: return lhs > rhs;
            case Condition::Op::GE: return lhs >= rhs;
        }
    }
    return false;
}

bool match(const Table& t, const Row& r, const std::optional<Condition>& cond) {
    if (!cond) return true;
    return eval_cond(t, r, *cond);
}

// ---- DELETE / UPDATE / DROP ----------------------------------------

std::size_t Database::exec(const Delete& s) {
    auto& t = table(s.table);
    auto old = t.rows.size();
    if (!s.where) {
        t.rows.clear();
        return old;
    }
    std::vector<Row> keep;
    keep.reserve(t.rows.size());
    for (auto& r : t.rows) {
        if (!match(t, r, s.where)) {
            keep.push_back(std::move(r));
        }
    }
    t.rows.swap(keep);
    return old - t.rows.size();
}

std::size_t Database::exec(const Update& s) {
    auto& t = table(s.table);
    std::vector<std::size_t> idxs;
    std::vector<Value> vals;
    idxs.reserve(s.assignments.size());
    vals.reserve(s.assignments.size());
    // Validate assignments
    for (auto& [name, val] : s.assignments) {
        auto idx = t.col_index(name);
        auto expected = t.schema[idx].type;
        if ((expected == Type::INT && !std::holds_alternative<Int>(val)) ||
            (expected == Type::STR && !std::holds_alternative<Str>(val))) {
            throw std::runtime_error("UPDATE type mismatch for column '" + name + "'");
        }
        idxs.push_back(idx);
        vals.push_back(val);
    }
    std::size_t count = 0;
    for (auto& r : t.rows) {
        if (!match(t, r, s.where)) continue;
        for (std::size_t i = 0; i < idxs.size(); ++i) {
            r.cells[idxs[i]] = vals[i];
        }
        ++count;
    }
    return count;
}

void Database::exec(const DropTable& s) {
    auto it = tables_.find(s.table);
    if (it == tables_.end()) {
        throw std::runtime_error("Unknown table '" + s.table + "'");
    }
    tables_.erase(it);
}


ResultTable Database::exec(const Select& s) const {
    // Helpers to deal with qualified names like "table.col"
    auto strip_qual = [](const std::string& col) -> std::string {
        auto p = col.find('.');
        return (p == std::string::npos) ? col : col.substr(p + 1);
    };
    auto split_qual = [](const std::string& col) -> std::pair<std::string,std::string> {
        auto p = col.find('.');
        if (p == std::string::npos) return {"", col};
        return {col.substr(0, p), col.substr(p + 1)};
    };
    auto has_col = [](const Table& t, const std::string& c) -> bool {
        try { t.col_index(c); return true; } catch (...) { return false; }
    };

    ResultTable out;

    if (!s.join) {
        const Table& t = table(s.table);

        // Build projection indices
        std::vector<std::size_t> proj_idx;
        const auto& cols = s.projection.columns; // empty => *
        const bool select_all = cols.empty() || (cols.size() == 1 && cols[0] == "*");

        if (select_all) {
            out.headers.reserve(t.schema.size());
            proj_idx.resize(t.schema.size());
            for (std::size_t i = 0; i < t.schema.size(); ++i) {
                out.headers.push_back(t.schema[i].name);
                proj_idx[i] = i;
            }
        } else {
            out.headers = cols;
            proj_idx.reserve(cols.size());
            for (const auto& c : cols) {
                proj_idx.push_back(t.col_index(strip_qual(c)));
            }
        }

        // WHERE (allow table.col or col)
        std::optional<Condition> cond = s.where;
        if (cond && cond->column.find('.') != std::string::npos) {
            cond->column = strip_qual(cond->column);
        }

        // Rows
        for (const auto& r : t.rows) {
            if (!match(t, r, cond)) continue;
            std::vector<Value> row;
            row.reserve(proj_idx.size());
            for (auto idx : proj_idx) row.push_back(r.cells[idx]);
            out.rows.push_back(std::move(row));
        }
        return out;
    }

    // ---------- INNER JOIN ----------
    const Table& left  = table(s.table);
    const Table& right = table(s.join->table); // right table from AST

    const std::string& leftName  = left.name;
    const std::string& rightName = right.name;

    const std::size_t lkey = left.col_index(s.join->left_column);
    const std::size_t rkey = right.col_index(s.join->right_column);

    // Build projection mapping: (table*, col_index)
    struct ProjCol { const Table* t; std::size_t idx; };
    std::vector<ProjCol> proj;

    const auto& cols = s.projection.columns;
    const bool select_all = cols.empty() || (cols.size() == 1 && cols[0] == "*");

    if (select_all) {
        // SELECT *  => all left cols then all right cols (qualified headers)
        for (std::size_t i = 0; i < left.schema.size(); ++i) {
            proj.push_back({&left, i});
            out.headers.push_back(leftName + "." + left.schema[i].name);
        }
        for (std::size_t i = 0; i < right.schema.size(); ++i) {
            proj.push_back({&right, i});
            out.headers.push_back(rightName + "." + right.schema[i].name);
        }
    } else {
        out.headers = cols;
        proj.reserve(cols.size());
        for (const auto& colspec : cols) {
            auto [tname, cname] = split_qual(colspec);
            if (tname.empty()) {
                // unqualified: prefer left, then right
                if (has_col(left, cname))       proj.push_back({&left,  left.col_index(cname)});
                else if (has_col(right, cname)) proj.push_back({&right, right.col_index(cname)});
                else throw std::runtime_error("Unknown column '" + cname + "' in SELECT");
            } else if (tname == leftName) {
                proj.push_back({&left, left.col_index(cname)});
            } else if (tname == rightName) {
                proj.push_back({&right, right.col_index(cname)});
            } else {
                throw std::runtime_error("Unknown table '" + tname + "' in SELECT list");
            }
        }
    }

    // WHERE evaluator for joined rows
    auto cmp_values = [](const Value& lhs, Condition::Op op, const Value& lit) -> bool {
        if (std::holds_alternative<Int>(lhs)) {
            if (!std::holds_alternative<Int>(lit))
                throw std::runtime_error("WHERE type mismatch: comparing int to string");
            auto a = std::get<Int>(lhs), b = std::get<Int>(lit);
            switch (op) {
                case Condition::Op::EQ: return a == b;
                case Condition::Op::NEQ: return a != b;
                case Condition::Op::LT: return a < b;
                case Condition::Op::LE: return a <= b;
                case Condition::Op::GT: return a > b;
                case Condition::Op::GE: return a >= b;
            }
        } else {
            if (!std::holds_alternative<Str>(lit))
                throw std::runtime_error("WHERE type mismatch: comparing string to int");
            const auto& a = std::get<Str>(lhs);
            const auto& b = std::get<Str>(lit);
            switch (op) {
                case Condition::Op::EQ: return a == b;
                case Condition::Op::NEQ: return a != b;
                case Condition::Op::LT: return a < b;
                case Condition::Op::LE: return a <= b;
                case Condition::Op::GT: return a > b;
                case Condition::Op::GE: return a >= b;
            }
        }
        return false;
    };

    auto where_ok = [&](const Row& lr, const Row& rr) -> bool {
        if (!s.where) return true;
        const auto& c = *s.where;
        auto [tname, cname] = split_qual(c.column);
        const Value* lhs = nullptr;
        if (tname.empty()) {
            if (has_col(left, cname))       lhs = &lr.cells[left.col_index(cname)];
            else if (has_col(right, cname)) lhs = &rr.cells[right.col_index(cname)];
            else throw std::runtime_error("Unknown column '" + cname + "' in WHERE");
        } else if (tname == leftName) {
            lhs = &lr.cells[left.col_index(cname)];
        } else if (tname == rightName) {
            lhs = &rr.cells[right.col_index(cname)];
        } else {
            throw std::runtime_error("Unknown table '" + tname + "' in WHERE");
        }
        return cmp_values(*lhs, c.op, c.literal);
    };

    // Nested-loop equi-join
    for (const auto& lr : left.rows) {
        const auto& lv = lr.cells[lkey];
        for (const auto& rr : right.rows) {
            const auto& rv = rr.cells[rkey];
            if (lv.index() != rv.index()) continue; // type mismatch → not equal
            bool eq = std::holds_alternative<Int>(lv)
                        ? (std::get<Int>(lv) == std::get<Int>(rv))
                        : (std::get<Str>(lv) == std::get<Str>(rv));
            if (!eq) continue;
            if (!where_ok(lr, rr)) continue;

            std::vector<Value> row;
            row.reserve(proj.size());
            for (const auto& p : proj) {
                if (p.t == &left)  row.push_back(lr.cells[p.idx]);
                else               row.push_back(rr.cells[p.idx]);
            }
            out.rows.push_back(std::move(row));
        }
    }
    return out;
}

void Database::dump(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out) throw std::runtime_error("dump: cannot open '" + filename + "' for writing");

    // Stable order for reproducible dumps
    std::vector<std::string> names;
    names.reserve(tables_.size());
    for (const auto& kv : tables_) names.push_back(kv.first);
    std::sort(names.begin(), names.end());

    auto type_sql = [](Type t) -> const char* {
        // match your parser's expected keywords (see sample3.sql): int / str
        return (t == Type::INT) ? "int" : "str";
    };
    auto q = [](const std::string& s) -> std::string {
        // Your lexer accepts only double-quoted strings and has no escaping.
        // To keep it loadable, replace embedded " with ' in the dump.
        std::string cpy;
        cpy.reserve(s.size());
        for (char ch : s) cpy += (ch == '"') ? '\'' : ch;
        return std::string("\"") + cpy + "\"";
    };

    for (const auto& name : names) {
        const Table& t = tables_.at(name);

        // --- CREATE TABLE
        out << "CREATE TABLE " << t.name << " (";
        for (std::size_t i = 0; i < t.schema.size(); ++i) {
            if (i) out << ", ";
            out << t.schema[i].name << " " << type_sql(t.schema[i].type);
        }
        out << ");\n";

        // --- INSERT rows (skip if empty)
        if (!t.rows.empty()) {
            out << "INSERT INTO " << t.name << " (";
            for (std::size_t i = 0; i < t.schema.size(); ++i) {
                if (i) out << ", ";
                out << t.schema[i].name;
            }
            out << ") VALUES\n";

            for (std::size_t r = 0; r < t.rows.size(); ++r) {
                if (r) out << ",\n";
                out << "  (";
                for (std::size_t c = 0; c < t.schema.size(); ++c) {
                    if (c) out << ", ";
                    const Value& v = t.rows[r].cells[c];
                    if (std::holds_alternative<Int>(v)) {
                        out << std::get<Int>(v);
                    } else {
                        out << q(std::get<Str>(v));
                    }
                }
                out << ")";
            }
            out << ";\n";
        }

        out << "\n";
    }
}

void Database::load(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) throw std::runtime_error("load: cannot open '" + filename + "' for reading");

    std::ostringstream ss; ss << in.rdbuf();
    std::string all = ss.str();

    // Use your existing tokenizer+parser to re-create the DB
    auto stmts_src = split_statements(all);
    for (const auto& stmt_src : stmts_src) {
        // skip pure whitespace chunks
        bool only_ws = true;
        for (unsigned char c : stmt_src) { if (!std::isspace(c)) { only_ws = false; break; } }
        if (only_ws) continue;

        auto toks = tokenize(stmt_src);
        TokenStream ts{toks};
        auto stmt = parse_statement(ts);
        if (!stmt) continue;

        // Dispatch: CREATE / INSERT / UPDATE / DELETE / DROP are supported.
        if      (auto* s = dynamic_cast<CreateTable*>(stmt.get())) { exec(*s); }
        else if (auto* s = dynamic_cast<Insert*>(stmt.get()))      { exec(*s); }
        else if (auto* s = dynamic_cast<Update*>(stmt.get()))      { exec(*s); }
        else if (auto* s = dynamic_cast<Delete*>(stmt.get()))      { exec(*s); }
        else if (auto* s = dynamic_cast<DropTable*>(stmt.get()))   { exec(*s); }
        else if (auto* s = dynamic_cast<Select*>(stmt.get())) {
            // Ignore SELECT during load (no side effects)
            (void)s;
        } else {
            // Unknown statement in dump (e.g., DUMP/LOAD directives) -> error
            throw std::runtime_error("load: unsupported statement encountered");
        }
    }
}

}
