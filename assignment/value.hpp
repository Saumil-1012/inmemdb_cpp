
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>

namespace db {

enum class Type { INT, STR };

using Int = std::int64_t;
using Str = std::string;
using Value = std::variant<Int, Str>;
inline std::string type_name(Type t) {
    return t == Type::INT ? "int" : "str";
}

inline Type parse_type(const std::string& s) {
    if (s == "int") return Type::INT;
    if (s == "str") return Type::STR;
    throw std::runtime_error("Unknown type '" + s + "' (expected 'int' or 'str')");
}

inline Value default_value(Type t) {
    return t == Type::INT ? Value{Int(0)} : Value{Str("")};
}
inline std::string to_string(const Value& v) {
    if (std::holds_alternative<Int>(v)) return std::to_string(std::get<Int>(v));
    return std::get<Str>(v);
}

inline bool is_int(const Value& v) { return std::holds_alternative<Int>(v); }
inline bool is_str(const Value& v) { return std::holds_alternative<Str>(v); }

}