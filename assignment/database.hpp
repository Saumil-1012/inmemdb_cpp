
#pragma once

#include "ast.hpp"

#include <string>
#include <unordered_map>

namespace db {

class Database {
public:

    Table& create_table(const std::string& name, const std::vector<Column>& cols);


    Table& table(const std::string& name);


    const Table& table(const std::string& name) const;


    void exec(const CreateTable& s);
    void exec(const Insert& s);
    std::size_t exec(const Delete& s);
    std::size_t exec(const Update& s);
    ResultTable exec(const Select& s) const;


    void exec(const Dump& s);
    void exec(const Load& s);
    void exec(const DropTable& s);


    void dump(const std::string& filename) const;


    void load(const std::string& filename);

private:
    std::unordered_map<std::string, Table> tables_;
};


bool match(const Table& t, const Row& r, const std::optional<Condition>& cond);

}
