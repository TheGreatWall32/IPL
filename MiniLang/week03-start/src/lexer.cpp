/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "lexer.hpp"

#include "diagnostics.hpp"

#include <array>
#include <locale>
#include <sstream>
#include <string_view>
#include <utility>

namespace mini {
namespace {

bool isAsciiDigit(const char character) noexcept {
  return character >= '0' && character <= '9';
}

bool isAsciiAlpha(const char character) noexcept {
  return (character >= 'a' && character <= 'z') ||
         (character >= 'A' && character <= 'Z') || character == '_';
}

[[maybe_unused]] bool isAsciiAlphaNumeric(const char character) noexcept {
  return isAsciiAlpha(character) || isAsciiDigit(character);
}

struct Keyword {
  std::string_view text;
  TokenKind kind;
};

constexpr std::array<Keyword, 12> keywords{{
    {"fun", TokenKind::Fun},       {"var", TokenKind::Var},
    {"return", TokenKind::Return}, {"if", TokenKind::If},
    {"else", TokenKind::Else},     {"for", TokenKind::For},
    {"print", TokenKind::Print},   {"true", TokenKind::True},
    {"false", TokenKind::False},   {"nil", TokenKind::Nil},
    {"and", TokenKind::And},       {"or", TokenKind::Or},
}};

[[maybe_unused]] TokenKind identifierKind(const std::string_view text) noexcept {
  for (const auto &keyword : keywords) {
    if (text == keyword.text) {
      return keyword.kind;
    }
  }
  return TokenKind::Identifier;
}

class Lexer {
public:
  explicit Lexer(const SourceText &source)
      : source_(source), contents_(source.contents()) {}

  std::vector<Token> run() {
    while (!atEnd()) {
      start_ = current_;
      scanToken();
    }

    tokens_.push_back(Token{TokenKind::EndOfFile, source_.span(current_), "",
                            std::monostate{}});
    return std::move(tokens_);
  }

private:
  bool atEnd() const noexcept { return current_ >= contents_.size(); }

  char advance() noexcept { return contents_[current_++]; }

  char peek() const noexcept { return atEnd() ? '\0' : contents_[current_]; }

  char peekNext() const noexcept {
    return current_ + 1 >= contents_.size() ? '\0'
                                            : contents_[current_ + 1];
  }

  bool match(const char expected) noexcept {
    if (atEnd() || contents_[current_] != expected) {
      return false;
    }
    ++current_;
    return true;
  }

  void addToken(const TokenKind kind,
                TokenLiteral literal = std::monostate{}) {
    const auto length = current_ - start_;
    tokens_.push_back(Token{kind, source_.span(start_, length),
                            std::string(contents_.substr(start_, length)),
                            std::move(literal)});
  }

  [[noreturn]] void unexpectedCharacter(const char character) const {
    std::string printable(1, character);
    throw CompileError(Phase::Lex, source_.name(), source_.span(start_, 1),
                       "unexpected character '" + printable + "'");
  }

  void scanToken() {
    const char character = advance();
    switch (character) {
    case '(':
      addToken(TokenKind::LeftParen);
      return;
    case ')':
      addToken(TokenKind::RightParen);
      return;
    case '{':
      addToken(TokenKind::LeftBrace);
      return;
    case '}':
      addToken(TokenKind::RightBrace);
      return;
    case ',':
      addToken(TokenKind::Comma);
      return;
    case ';':
      addToken(TokenKind::Semicolon);
      return;
    case '+':
      addToken(TokenKind::Plus);
      return;
    case '-':
      addToken(TokenKind::Minus);
      return;
    case '*':
      addToken(TokenKind::Star);
      return;
    case '!':
      addToken(match('=') ? TokenKind::BangEqual : TokenKind::Bang);
      return;
    case '=':
      addToken(match('=') ? TokenKind::EqualEqual : TokenKind::Equal);
      return;
    case '<':
      addToken(match('=') ? TokenKind::LessEqual : TokenKind::Less);
      return;
    case '>':
      addToken(match('=') ? TokenKind::GreaterEqual : TokenKind::Greater);
      return;
    case '/':
      if (match('/')) {
        while (!atEnd() && peek() != '\n' && peek() != '\r') {
          advance();
        }
        return;
      }
      addToken(TokenKind::Slash);
      return;
    case '"':
      string();
      return;
    case ' ':
    case '\t':
    case '\r':
    case '\n':
      return;
    default:
      if (isAsciiDigit(character)) {
        number();
        return;
      }
      if (isAsciiAlpha(character)) {
        identifier();
        return;
      }
      unexpectedCharacter(character);
    }
  }

  void identifier() {
    // TODO WEEK03: read the entire name, classify it, then add its token.
    throw CompileError(Phase::Lex, source_.name(), source_.span(start_, 1),
                       "TODO WEEK03: implement identifier scanning");
  }

  void number() {
    while (isAsciiDigit(peek())) {
      advance();
    }

    if (peek() == '.' && isAsciiDigit(peekNext())) {
      advance();
      while (isAsciiDigit(peek())) {
        advance();
      }
    }

    const std::string text(contents_.substr(start_, current_ - start_));
    std::istringstream input(text);
    input.imbue(std::locale::classic());
    double value = 0.0;
    input >> value;
    if (!input) {
      throw CompileError(Phase::Lex, source_.name(),
                         source_.span(start_, current_ - start_),
                         "invalid number literal");
    }
    addToken(TokenKind::Number, value);
  }

  void string() {
    while (!atEnd() && peek() != '"' && peek() != '\r' && peek() != '\n') {
      if (peek() == '\\') {
        throw CompileError(Phase::Lex, source_.name(),
                           source_.span(current_, 1),
                           "backslash is not allowed in a string literal");
      }
      advance();
    }

    if (!match('"')) {
      throw CompileError(Phase::Lex, source_.name(),
                         source_.span(start_, current_ - start_),
                         "unterminated string literal");
    }

    const auto contents_start = start_ + 1;
    const auto contents_length = current_ - start_ - 2;
    addToken(TokenKind::String,
             std::string(contents_.substr(contents_start, contents_length)));
  }

  const SourceText &source_;
  std::string_view contents_;
  std::vector<Token> tokens_;
  std::size_t start_{0};
  std::size_t current_{0};
};

} // namespace

std::vector<Token> scan(const SourceText &source) { return Lexer(source).run(); }

} // namespace mini
