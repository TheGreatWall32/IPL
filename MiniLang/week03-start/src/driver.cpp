/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "driver.hpp"

#include "ast.hpp"
#include "codegen.hpp"
#include "diagnostics.hpp"
#include "host_compiler.hpp"
#include "lexer.hpp"
#include "lower.hpp"
#include "parser.hpp"
#include "resolver.hpp"
#include "token.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>

namespace mini {
namespace {

class TemporaryDirectory {
public:
  explicit TemporaryDirectory(const std::string &purpose) {
    std::error_code error;
    const auto base = std::filesystem::temp_directory_path(error);
    if (error) {
      throw CompileError(Phase::Backend,
                         "cannot locate the temporary directory: " +
                             error.message());
    }

    // The CLI is single-threaded; create_directory handles process collisions.
    static unsigned long long serial{0};
    const auto timestamp = std::chrono::steady_clock::now()
                               .time_since_epoch()
                               .count();
    for (int attempt = 0; attempt < 100; ++attempt) {
      const auto unique = serial++;
      path_ = base / ("minilang-" + purpose + "-" +
                      std::to_string(timestamp) + "-" +
                      std::to_string(unique));
      error.clear();
      if (std::filesystem::create_directory(path_, error)) {
        return;
      }
      if (error && error != std::errc::file_exists) {
        throw CompileError(Phase::Backend,
                           "cannot create a temporary directory: " +
                               error.message());
      }
    }

    throw CompileError(Phase::Backend,
                       "cannot create a unique temporary directory");
  }

  TemporaryDirectory(const TemporaryDirectory &) = delete;
  TemporaryDirectory &operator=(const TemporaryDirectory &) = delete;

  ~TemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }

  const std::filesystem::path &path() const noexcept { return path_; }

private:
  std::filesystem::path path_;
};

void writeTextFile(const std::filesystem::path &path,
                   const std::string &contents) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw CompileError(Phase::Io, path.string(), "cannot open output file");
  }
  output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
  if (!output) {
    throw CompileError(Phase::Io, path.string(), "cannot write output file");
  }
}

std::filesystem::path comparableAbsolutePath(
    const std::filesystem::path &path) {
  std::error_code error;
  auto absolute = std::filesystem::absolute(path, error);
  if (error) {
    throw CompileError(Phase::Io, path.string(),
                       "cannot resolve output path: " + error.message());
  }
  return absolute.lexically_normal();
}

void rejectInputOverwrite(const std::filesystem::path &input,
                          const std::filesystem::path &output) {
  bool same_file = comparableAbsolutePath(input) ==
                   comparableAbsolutePath(output);

  std::error_code exists_error;
  if (!same_file && std::filesystem::exists(output, exists_error) &&
      !exists_error) {
    std::error_code equivalent_error;
    same_file = std::filesystem::equivalent(input, output, equivalent_error) &&
                !equivalent_error;
  }

  if (same_file) {
    throw CompileError(Phase::Io, output.string(),
                       "refusing to overwrite the Mini source file");
  }
}

std::filesystem::path defaultExecutablePath(
    const std::filesystem::path &input, const std::string &suffix) {
  auto output = input;
  output.replace_extension();
  if (output == input) {
    output += suffix.empty() ? ".out" : suffix;
  } else if (!suffix.empty()) {
    output += suffix;
  }
  return output;
}

} // namespace

int executeCommand(const CommandLine &command, const SourceText &source,
                   std::ostream &output) {
  const auto tokens = scan(source);
  if (command.kind == CommandKind::Tokens) {
    output << formatTokens(tokens);
    return 0;
  }

  auto program = parse(source, tokens);
  if (command.kind == CommandKind::Ast) {
    output << formatAst(program);
    return 0;
  }

  program = lower(std::move(program));
  if (command.kind == CommandKind::Lower) {
    output << formatAst(program);
    return 0;
  }

  const auto resolution = resolve(source, program);
  if (command.kind == CommandKind::Check) {
    return 0;
  }

  const auto generated_cpp = emitCpp(program, resolution);
  if (command.kind == CommandKind::Cpp) {
    if (command.output_path) {
      rejectInputOverwrite(command.input_path, *command.output_path);
      writeTextFile(*command.output_path, generated_cpp);
    } else {
      output << generated_cpp;
    }
    return 0;
  }

  const auto compiler = HostCompiler::configured();
  if (command.kind == CommandKind::Build) {
    const auto executable = command.output_path.value_or(
        defaultExecutablePath(command.input_path,
                              compiler.config().executable_suffix));
    rejectInputOverwrite(command.input_path, executable);

    TemporaryDirectory temporary("build");
    const auto cpp_path = temporary.path() / "generated.cpp";
    writeTextFile(cpp_path, generated_cpp);
    compiler.compile(cpp_path, executable, temporary.path());
    return 0;
  }

  if (command.kind == CommandKind::Run) {
    TemporaryDirectory temporary("run");
    const auto cpp_path = temporary.path() / "generated.cpp";
    const auto executable = temporary.path() /
                            ("program" + compiler.config().executable_suffix);
    writeTextFile(cpp_path, generated_cpp);
    compiler.compile(cpp_path, executable, temporary.path());
    return compiler.run(executable);
  }

  return 0;
}

} // namespace mini
