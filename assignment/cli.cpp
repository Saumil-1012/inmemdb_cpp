
#include "lib.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    using namespace db;

    OutputFormat fmt = OutputFormat::ASCII;
    std::string input;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--csv") {
            fmt = OutputFormat::CSV;
        } else {
            std::ifstream f(arg);
            if (!f) {
                std::cerr << "Failed to open file: " << arg << "\n";
                return 1;
            }
            std::ostringstream ss;
            ss << f.rdbuf();
            input = ss.str();
        }
    }

    if (input.empty()) {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        input = ss.str();
    }
    Database db;
    try {

        auto stmts_src = split_statements(input);
        for (auto& one : stmts_src) {

            if (std::all_of(one.begin(), one.end(), [](unsigned char c) {
                    return std::isspace(c);
                })) {
                continue;
            }

            auto toks = tokenize(one);
            TokenStream ts{toks};
            auto stmt = parse_statement(ts);
            if (!stmt) continue;

            if (auto* s = dynamic_cast<Select*>(stmt.get())) {
                auto out = db.exec(*s);
                if (fmt == OutputFormat::ASCII)
                    std::cout << render_ascii(out);
                else

                    std::cout << render_csv(out);
            } else if (auto* s = dynamic_cast<CreateTable*>(stmt.get())) {
                db.exec(*s);
            } else if (auto* s = dynamic_cast<Insert*>(stmt.get())) {
                db.exec(*s);
            } else if (auto* s = dynamic_cast<Delete*>(stmt.get())) {
                db.exec(*s);
            } else if (auto* s = dynamic_cast<Update*>(stmt.get())) {
                db.exec(*s);
            } else if (auto* s = dynamic_cast<Dump*>(stmt.get())) {
                db.exec(*s);
            } else if (auto* s = dynamic_cast<Load*>(stmt.get())) {
                db.exec(*s);
            } else if (auto* s = dynamic_cast<DropTable*>(stmt.get())) {
                db.exec(*s);
            }
        }
    } catch (const std::exception& e) {

        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}
