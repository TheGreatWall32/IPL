/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace mini {

enum class CommandKind {
  Help,
  Tokens,
  Ast,
  Lower,
  Check,
  Cpp,
  Build,
  Run,
};

struct CommandLine {
  CommandKind kind{CommandKind::Help};
  std::filesystem::path input_path;
  std::optional<std::filesystem::path> output_path;
};

CommandLine parseCommandLine(const std::vector<std::string> &arguments);
std::string helpText();

} // namespace mini
