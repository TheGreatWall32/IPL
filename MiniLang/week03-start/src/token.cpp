/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "token.hpp"

#include "number_format.hpp"

#include <iomanip>
#include <locale>
#include <ostream>
#include <sstream>
#include <string_view>

namespace mini {
namespace {

std::string_view tokenKindName(const TokenKind kind) noexcept {
  switch (kind) {
  case TokenKind::LeftParen:
    return "LEFT_PAREN";
  case TokenKind::RightParen:
    return "RIGHT_PAREN";
  case TokenKind::LeftBrace:
    return "LEFT_BRACE";
  case TokenKind::RightBrace:
    return "RIGHT_BRACE";
  case TokenKind::Comma:
    return "COMMA";
  case TokenKind::Semicolon:
    return "SEMICOLON";
  case TokenKind::Plus:
    return "PLUS";
  case TokenKind::Minus:
    return "MINUS";
  case TokenKind::Star:
    return "STAR";
  case TokenKind::Slash:
    return "SLASH";
  case TokenKind::Bang:
    return "BANG";
  case TokenKind::BangEqual:
    return "BANG_EQUAL";
  case TokenKind::Equal:
    return "EQUAL";
  case TokenKind::EqualEqual:
    return "EQUAL_EQUAL";
  case TokenKind::Less:
    return "LESS";
  case TokenKind::LessEqual:
    return "LESS_EQUAL";
  case TokenKind::Greater:
    return "GREATER";
  case TokenKind::GreaterEqual:
    return "GREATER_EQUAL";
  case TokenKind::Identifier:
    return "IDENTIFIER";
  case TokenKind::Number:
    return "NUMBER";
  case TokenKind::String:
    return "STRING";
  case TokenKind::Fun:
    return "FUN";
  case TokenKind::Var:
    return "VAR";
  case TokenKind::Return:
    return "RETURN";
  case TokenKind::If:
    return "IF";
  case TokenKind::Else:
    return "ELSE";
  case TokenKind::For:
    return "FOR";
  case TokenKind::Print:
    return "PRINT";
  case TokenKind::True:
    return "TRUE";
  case TokenKind::False:
    return "FALSE";
  case TokenKind::Nil:
    return "NIL";
  case TokenKind::And:
    return "AND";
  case TokenKind::Or:
    return "OR";
  case TokenKind::EndOfFile:
    return "EOF";
  }

  return "UNKNOWN";
}

void writeToken(std::ostream &output, const Token &token) {
  output << tokenKindName(token.kind) << ' ' << token.span.line << ':'
         << token.span.column << ' ' << std::quoted(token.lexeme);

  if (const auto *number = std::get_if<double>(&token.literal)) {
    output << " = " << formatNumberForDisplay(*number);
  } else if (const auto *string = std::get_if<std::string>(&token.literal)) {
    output << " = " << std::quoted(*string);
  }
}

} // namespace

std::string formatTokens(const std::vector<Token> &tokens) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  for (const auto &token : tokens) {
    writeToken(output, token);
    output << '\n';
  }
  return output.str();
}

} // namespace mini
