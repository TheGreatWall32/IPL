/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "ast.hpp"

#include "number_format.hpp"

#include <iomanip>
#include <sstream>

namespace mini {
namespace {

void formatExpression(std::ostringstream &output, const Expression &expression) {
  if (const auto *number = std::get_if<NumberExpression>(&expression.node)) {
    output << "(number " << formatNumberForDisplay(number->value) << ')';
    return;
  }
  if (const auto *string = std::get_if<StringExpression>(&expression.node)) {
    output << "(string " << std::quoted(string->value) << ')';
    return;
  }
  if (const auto *boolean = std::get_if<BoolExpression>(&expression.node)) {
    output << "(bool " << (boolean->value ? "true" : "false") << ')';
    return;
  }
  if (std::holds_alternative<NilExpression>(expression.node)) {
    output << "(nil)";
    return;
  }
  if (const auto *grouping =
          std::get_if<GroupingExpression>(&expression.node)) {
    output << "(group ";
    formatExpression(output, *grouping->expression);
    output << ')';
    return;
  }
}

void formatStatement(std::ostringstream &output, const Statement &statement,
                     const std::size_t indentation) {
  output << std::string(indentation, ' ');
  if (const auto *print = std::get_if<PrintStatement>(&statement.node)) {
    output << "(print ";
    formatExpression(output, print->expression);
    output << ')';
    return;
  }
  if (const auto *expression =
          std::get_if<ExpressionStatement>(&statement.node)) {
    output << "(expr ";
    formatExpression(output, expression->expression);
    output << ')';
    return;
  }
}

} // namespace

std::string formatAst(const Program &program) {
  std::ostringstream output;
  output << "(program\n";
  output << "  (fun main ()\n";
  for (const auto &statement : program.main_body) {
    formatStatement(output, statement, 4);
    output << '\n';
  }
  output << "  )\n";
  output << ")\n";
  return output.str();
}

} // namespace mini
