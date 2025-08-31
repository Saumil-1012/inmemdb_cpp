
#pragma once

#include "ast.hpp"

#include <string>

namespace db {

enum class OutputFormat { ASCII, CSV };

std::string render_ascii(const ResultTable& t);
std::string render_csv(const ResultTable& t);

}