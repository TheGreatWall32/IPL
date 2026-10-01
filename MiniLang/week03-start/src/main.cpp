/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "cli.hpp"
#include "diagnostics.hpp"
#include "driver.hpp"
#include "source.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  try {
    std::vector<std::string> arguments;
    arguments.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1) : 0);
    for (int index = 1; index < argc; ++index) {
      arguments.emplace_back(argv[index]);
    }

    const auto command = mini::parseCommandLine(arguments);
    if (command.kind == mini::CommandKind::Help) {
      std::cout << mini::helpText();
      return 0;
    }

    const auto source = mini::SourceText::fromFile(command.input_path);
    return mini::executeCommand(command, source, std::cout);
  } catch (const mini::CompileError &error) {
    std::cerr << mini::formatDiagnostic(error) << '\n';
    return error.phase() == mini::Phase::Cli ? 2 : 1;
  } catch (const std::exception &error) {
    std::cerr << "internal: " << error.what() << '\n';
    return 70;
  }
}
