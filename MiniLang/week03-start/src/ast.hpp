/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include "source.hpp"

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace mini {

struct NumberExpression {
  double value;
};

struct StringExpression {
  std::string value;
};

struct BoolExpression {
  bool value;
};

struct NilExpression {};

struct Expression;
using ExpressionPtr = std::unique_ptr<Expression>;

struct GroupingExpression {
  ExpressionPtr expression;
};

struct Expression {
  SourceSpan span;
  std::variant<NumberExpression, StringExpression, BoolExpression,
               NilExpression, GroupingExpression>
      node;
};

struct PrintStatement {
  Expression expression;
};

struct ExpressionStatement {
  Expression expression;
};

struct Statement {
  SourceSpan span;
  std::variant<PrintStatement, ExpressionStatement>
      node;
};

struct Program {
  std::vector<Statement> main_body;
};

std::string formatAst(const Program &program);

} // namespace mini
