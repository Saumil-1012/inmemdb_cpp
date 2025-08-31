
#pragma once

#include "value.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace db {


struct Column {
    std::string name;
    Type type;
};

struct Row {
    std::vector<Value> cells;
};


struct Table {
    std::string name;
    std::vector<Column> schema;
    std::unordered_map<std::string, std::size_t> index;
    std::vector<Row> rows;


    std::size_t col_index(const std::string& c) const;

    Type col_type(const std::string& c) const;
};


struct ResultTable {
    std::vector<std::string> headers;
    std::vector<std::vector<Value>> rows;
};


struct Condition {
    enum class Op { EQ, NEQ, LT, LE, GT, GE };
    std::string column;
    Value literal;
    Op op = Op::EQ;
};

struct Projection {
    std::vector<std::string> columns;
};


struct JoinClause {
    std::string table;
    std::string left_table;
    std::string left_column;
    std::string right_table;
    std::string right_column;
};


struct Statement {
    virtual ~Statement() = default;
};


struct Dump : Statement {
    std::string filename;
    virtual ~Dump() = default;
};

struct Load : Statement {
    std::string filename;
    virtual ~Load() = default;
};
struct CreateTable : Statement {
    std::string table;
    std::vector<Column> columns;
    virtual ~CreateTable() = default;
};
struct Insert : Statement {
    std::string table;
    std::vector<std::string> columns;
    std::vector<std::vector<Value>> values_lists;
    virtual ~Insert() = default;
};

struct Delete : Statement {
    std::string table;
    std::optional<Condition> where;
    virtual ~Delete() = default;
};
struct Update : Statement {
    std::string table;
    std::vector<std::pair<std::string, Value>> assignments;
    std::optional<Condition> where;
    virtual ~Update() = default;
};
struct Select : Statement {
    Projection projection;
    std::string table;
    std::optional<JoinClause> join;
    std::optional<Condition> where;
    virtual ~Select() = default;
};
struct DropTable : Statement {
    std::string table;
    virtual ~DropTable() = default;
};

}
