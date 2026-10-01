/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include "source.hpp"

#include <optional>
#include <stdexcept>
#include <string>

namespace mini {

enum class Phase {
  Cli,
  Io,
  Lex,
  Parse,
  Resolve,
  Codegen,
  Backend,
};

class CompileError final : public std::runtime_error {
public:
  CompileError(Phase phase, std::string message);
  CompileError(Phase phase, std::string source_name, std::string message);
  CompileError(Phase phase, std::string source_name, SourceSpan span,
               std::string message);

  Phase phase() const noexcept { return phase_; }
  const std::optional<std::string> &sourceName() const noexcept {
    return source_name_;
  }
  const std::optional<SourceSpan> &sourceSpan() const noexcept { return span_; }

private:
  Phase phase_;
  std::optional<std::string> source_name_;
  std::optional<SourceSpan> span_;
};

std::string formatDiagnostic(const CompileError &error);

} // namespace mini
