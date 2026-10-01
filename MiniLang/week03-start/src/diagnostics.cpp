/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "diagnostics.hpp"

#include <sstream>
#include <string_view>
#include <utility>

namespace mini {
namespace {

std::string_view phaseName(const Phase phase) noexcept {
  switch (phase) {
  case Phase::Cli:
    return "cli";
  case Phase::Io:
    return "io";
  case Phase::Lex:
    return "lex";
  case Phase::Parse:
    return "parse";
  case Phase::Resolve:
    return "resolve";
  case Phase::Codegen:
    return "codegen";
  case Phase::Backend:
    return "backend";
  }

  return "unknown";
}

} // namespace

CompileError::CompileError(const Phase phase, std::string message)
    : std::runtime_error(std::move(message)), phase_(phase) {}

CompileError::CompileError(const Phase phase, std::string source_name,
                           std::string message)
    : std::runtime_error(std::move(message)), phase_(phase),
      source_name_(std::move(source_name)) {}

CompileError::CompileError(const Phase phase, std::string source_name,
                           const SourceSpan span, std::string message)
    : std::runtime_error(std::move(message)), phase_(phase),
      source_name_(std::move(source_name)), span_(span) {}

std::string formatDiagnostic(const CompileError &error) {
  std::ostringstream output;

  if (error.sourceName()) {
    output << *error.sourceName();
    if (error.sourceSpan()) {
      output << ':' << error.sourceSpan()->line << ':'
             << error.sourceSpan()->column;
    }
    output << ": ";
  }

  output << phaseName(error.phase()) << ": " << error.what();
  return output.str();
}

} // namespace mini
