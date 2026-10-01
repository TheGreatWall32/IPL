/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "cli.hpp"

#include "diagnostics.hpp"

#include <array>
#include <string_view>

namespace mini {
namespace {

struct CommandEntry {
  std::string_view name;
  CommandKind kind;
  bool accepts_output;
};

constexpr std::array<CommandEntry, 7> commands{{
    {"tokens", CommandKind::Tokens, false},
    {"ast", CommandKind::Ast, false},
    {"lower", CommandKind::Lower, false},
    {"check", CommandKind::Check, false},
    {"cpp", CommandKind::Cpp, true},
    {"build", CommandKind::Build, true},
    {"run", CommandKind::Run, false},
}};

const CommandEntry *findCommand(const std::string_view name) noexcept {
  for (const auto &entry : commands) {
    if (entry.name == name) {
      return &entry;
    }
  }
  return nullptr;
}

} // namespace

CommandLine parseCommandLine(const std::vector<std::string> &arguments) {
  if (arguments.empty()) {
    throw CompileError(Phase::Cli, "missing command; try 'minic --help'");
  }

  if (arguments.front() == "--help") {
    if (arguments.size() != 1) {
      throw CompileError(Phase::Cli,
                         "the help command does not accept additional arguments");
    }
    return CommandLine{CommandKind::Help, {}, std::nullopt};
  }

  const auto *entry = findCommand(arguments.front());
  if (entry == nullptr) {
    throw CompileError(Phase::Cli,
                       "unknown command '" + arguments.front() +
                           "'; try 'minic --help'");
  }

  if (arguments.size() < 2) {
    throw CompileError(Phase::Cli,
                       "command '" + arguments.front() +
                           "' requires a Mini source file");
  }

  CommandLine result{entry->kind, std::filesystem::path(arguments[1]),
                     std::nullopt};

  if (!entry->accepts_output) {
    if (arguments.size() != 2) {
      throw CompileError(Phase::Cli,
                         "command '" + arguments.front() +
                             "' accepts exactly one source file");
    }
    return result;
  }

  if (arguments.size() == 2) {
    return result;
  }

  if (arguments.size() != 4 || arguments[2] != "-o" ||
      arguments[3].empty()) {
    throw CompileError(Phase::Cli,
                       "command '" + arguments.front() +
                           "' accepts FILE followed optionally by '-o OUTPUT'");
  }

  result.output_path = std::filesystem::path(arguments[3]);
  return result;
}

std::string helpText() {
  return
      "Mini 1.0 educational compiler\n"
      "\n"
      "Usage:\n"
      "  minic tokens FILE\n"
      "  minic ast FILE\n"
      "  minic lower FILE\n"
      "  minic check FILE\n"
      "  minic cpp FILE [-o OUTPUT.cpp]\n"
      "  minic build FILE [-o EXECUTABLE]\n"
      "  minic run FILE\n"
      "  minic --help\n"
      "\n"
      "Commands:\n"
      "  tokens  Scan FILE and print its tokens.\n"
      "  ast     Parse FILE and print its parsed syntax tree.\n"
      "  lower   Lower FILE and print its core syntax tree.\n"
      "  check   Run the frontend checks without generating C++.\n"
      "  cpp     Generate C++17 source code.\n"
      "  build   Generate C++17 and build a native executable.\n"
      "  run     Build and run FILE.\n"
      "\n"
      "Output defaults:\n"
      "  cpp writes C++ to standard output unless -o is provided.\n"
      "  build writes an executable beside FILE unless -o is provided.\n"
      "\n"
      "Mini 1.0 implements expressions, local variables, branches, limited for loops,\n"
      "parameters, function calls, returns, and recursion.\n";
}

} // namespace mini
