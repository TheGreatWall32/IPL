/*
 * Educational Mini compiler runtime.
 * Supports documented Mini 1.0 programs on the intentional happy path.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

namespace mini {

using Value = std::variant<std::monostate, double, bool, std::string>;

class RuntimeError final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

inline std::string formatValueForOutput(const Value &value) {
  if (const auto *number = std::get_if<double>(&value)) {
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << *number;
    return output.str();
  }
  if (const auto *boolean = std::get_if<bool>(&value)) {
    return *boolean ? "true" : "false";
  }
  if (const auto *string = std::get_if<std::string>(&value)) {
    return *string;
  }
  return "nil";
}

inline void printValue(const Value &value) {
  std::cout << formatValueForOutput(value) << '\n';
}

} // namespace mini
