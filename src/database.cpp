
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

}
