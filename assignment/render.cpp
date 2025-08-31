
#include "render.hpp"
#include "value.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace db {

std::string render_ascii(const ResultTable& t) {
    std::vector<size_t> widths(t.headers.size(), 0);

    for (size_t i = 0; i < t.headers.size(); ++i) {
        widths[i] = std::max<size_t>(widths[i], t.headers[i].size());
    }
    for (const auto& row : t.rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            widths[i] = std::max(widths[i], to_string(row[i]).size());
        }
    }
    auto make_line = [&]() {
        std::ostringstream os;
        os << "+";
        for (auto w : widths) {
            os << std::string(w + 2, '-') << "+";
        }
        os << "\n";
        return os.str();
    };
    std::ostringstream out;
    out << make_line();

    out << "|";
    for (size_t i = 0; i < t.headers.size(); ++i) {
        out << " " << t.headers[i];
        out << std::string(widths[i] - t.headers[i].size(), ' ') << " |";
    }
    out << "\n" << make_line();

    for (const auto& row : t.rows) {
        out << "|";
        for (size_t i = 0; i < row.size(); ++i) {
            auto s = to_string(row[i]);
            out << " " << s << std::string(widths[i] - s.size(), ' ') << " |";
        }
        out << "\n";
    }
    out << make_line();
    return out.str();
}

static std::string csv_escape(const std::string& s) {
    bool need_quotes = s.find_first_of(",\"\n\r") != std::string::npos;
    if (!need_quotes) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '\"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}
std::string render_csv(const ResultTable& t) {
    std::ostringstream out;

    for (size_t i = 0; i < t.headers.size(); ++i) {
        if (i) out << ",";
        out << csv_escape(t.headers[i]);
    }
    out << "\n";
    // rows
    for (const auto& row : t.rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i) out << ",";
            if (std::holds_alternative<Int>(row[i])) out << std::get<Int>(row[i]);
            else out << csv_escape(std::get<Str>(row[i]));
        }
        out << "\n";
    }
    return out.str();
}

}