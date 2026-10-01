/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "parser.hpp"

#include "diagnostics.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace mini {
namespace {

class Parser {
public:
  Parser(const SourceText &source, const std::vector<Token> &tokens)
      : source_(source), tokens_(tokens) {
    if (tokens_.empty() || tokens_.back().kind != TokenKind::EndOfFile) {
      throw std::invalid_argument("parser requires an EOF-terminated token list");
    }
  }

  Program run() {
    consume(TokenKind::Fun, "expected 'fun main()' at the start of the program");
    const auto &name = consume(TokenKind::Identifier, "expected 'main'");
    if (name.lexeme != "main") {
      throw CompileError(Phase::Parse, source_.name(), name.span,
                         "this version supports only 'fun main()'");
    }
    consume(TokenKind::LeftParen, "expected '(' after 'main'");
    consume(TokenKind::RightParen, "main has no parameters");
    consume(TokenKind::LeftBrace, "expected '{' before main body");
    auto body = blockDeclarations();
    consume(TokenKind::RightBrace, "expected '}' after main body");
    consume(TokenKind::EndOfFile, "expected end of file after main");
    return Program{std::move(body)};
  }

private:
  const Token &peek() const noexcept { return tokens_[current_]; }

  bool check(const TokenKind kind) const noexcept { return peek().kind == kind; }

  bool match(const TokenKind kind) noexcept {
    if (!check(kind)) {
      return false;
    }
    advance();
    return true;
  }

  const Token &advance() noexcept {
    const auto &token = peek();
    if (current_ + 1 < tokens_.size()) {
      ++current_;
    }
    return token;
  }

  const Token &previous() const noexcept { return tokens_[current_ - 1]; }

  const Token &consume(const TokenKind kind, const std::string &message) {
    if (check(kind)) {
      return advance();
    }
    throw CompileError(Phase::Parse, source_.name(), peek().span, message);
  }

  SourceSpan spanFrom(const SourceSpan &first, const SourceSpan &last) const {
    const auto last_end = last.offset + last.length;
    return source_.span(first.offset, last_end - first.offset);
  }

  Statement statement() {
    if (match(TokenKind::Print)) {
      return printStatement(previous());
    }
    return expressionStatement();
  }

  Statement printStatement(const Token &print_keyword) {
    auto value = expression();
    const auto &semicolon =
        consume(TokenKind::Semicolon, "expected ';' after print value");

    return Statement{spanFrom(print_keyword.span, semicolon.span),
                     PrintStatement{std::move(value)}};
  }

  Statement expressionStatement() {
    auto value = expression();
    const auto &semicolon =
        consume(TokenKind::Semicolon, "expected ';' after expression");
    const auto span = spanFrom(value.span, semicolon.span);
    return Statement{span, ExpressionStatement{std::move(value)}};
  }

  std::vector<Statement> blockDeclarations() {
    std::vector<Statement> statements;
    while (!check(TokenKind::RightBrace) &&
           !check(TokenKind::EndOfFile)) {
      statements.push_back(statement());
    }
    return statements;
  }

  Expression expression() { return primary(); }

  Expression primary() {
    if (match(TokenKind::Number)) {
      const auto &token = previous();
      return Expression{token.span,
                        NumberExpression{std::get<double>(token.literal)}};
    }

    if (match(TokenKind::String)) {
      const auto &token = previous();
      return Expression{token.span,
                        StringExpression{std::get<std::string>(token.literal)}};
    }

    if (match(TokenKind::True)) {
      return Expression{previous().span, BoolExpression{true}};
    }
    if (match(TokenKind::False)) {
      return Expression{previous().span, BoolExpression{false}};
    }
    if (match(TokenKind::Nil)) {
      return Expression{previous().span, NilExpression{}};
    }
    if (match(TokenKind::LeftParen)) {
      const auto &left_paren = previous();
      auto nested = expression();
      const auto &right_paren =
          consume(TokenKind::RightParen, "expected ')' after expression");
      return Expression{
          spanFrom(left_paren.span, right_paren.span),
          GroupingExpression{
              std::make_unique<Expression>(std::move(nested))}};
    }

    throw CompileError(Phase::Parse, source_.name(), peek().span,
                       "expected expression");
  }

  const SourceText &source_;
  const std::vector<Token> &tokens_;
  std::size_t current_{0};
};

} // namespace

Program parse(const SourceText &source, const std::vector<Token> &tokens) {
  return Parser(source, tokens).run();
}

} // namespace mini
