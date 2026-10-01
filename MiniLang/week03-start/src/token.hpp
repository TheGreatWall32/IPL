/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include "source.hpp"

#include <string>
#include <variant>
#include <vector>

namespace mini {

enum class TokenKind {
  LeftParen,
  RightParen,
  LeftBrace,
  RightBrace,
  Comma,
  Semicolon,
  Plus,
  Minus,
  Star,
  Slash,
  Bang,
  BangEqual,
  Equal,
  EqualEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Identifier,
  Number,
  String,
  Fun,
  Var,
  Return,
  If,
  Else,
  For,
  Print,
  True,
  False,
  Nil,
  And,
  Or,
  EndOfFile,
};

using TokenLiteral = std::variant<std::monostate, double, std::string>;

struct Token {
  TokenKind kind;
  SourceSpan span;
  std::string lexeme;
  TokenLiteral literal{};
};

std::string formatTokens(const std::vector<Token> &tokens);

} // namespace mini
