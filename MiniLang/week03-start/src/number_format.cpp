/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "number_format.hpp"

#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace mini {
namespace {

std::ostringstream numberStream() {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  return output;
}

} // namespace

std::string formatNumberForDisplay(const double value) {
  auto output = numberStream();
  output << value;
  return output.str();
}

std::string formatNumberForCpp(const double value) {
  auto output = numberStream();
  output << std::setprecision(std::numeric_limits<double>::max_digits10) << value;

  auto result = output.str();
  if (result.find_first_of(".eE") == std::string::npos) {
    result += ".0";
  }
  return result;
}

} // namespace mini
