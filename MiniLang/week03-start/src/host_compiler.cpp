/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "host_compiler.hpp"

#include "diagnostics.hpp"
#include "mini_build_config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace mini {
namespace {

std::string environmentValue(const char *name) {
  const char *value = std::getenv(name);
  return value == nullptr ? std::string{} : std::string(value);
}

std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](char character) {
    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(character)));
  });
  return value;
}

CompilerFamily familyForOverride(const std::string &executable) {
  const auto name = lowercase(std::filesystem::path(executable).filename().string());
  if (name == "cl" || name == "cl.exe" || name == "clang-cl" ||
      name == "clang-cl.exe") {
    return CompilerFamily::Msvc;
  }
  return CompilerFamily::GnuLike;
}

std::filesystem::path absolutePath(const std::filesystem::path &path,
                                   const std::string_view purpose) {
  std::error_code error;
  auto result = std::filesystem::absolute(path, error);
  if (error) {
    throw CompileError(Phase::Backend,
                       "cannot resolve " + std::string(purpose) + " path '" +
                           path.string() + "': " + error.message());
  }
  return result.lexically_normal();
}

#ifdef _WIN32
std::string quoteShellArgument(const std::string &argument) {
  std::string result{"\""};
  std::size_t backslashes = 0;
  for (const char character : argument) {
    if (character == '\\') {
      ++backslashes;
      continue;
    }

    if (character == '\"') {
      result.append(backslashes * 2 + 1, '\\');
      result.push_back('\"');
    } else {
      result.append(backslashes, '\\');
      result.push_back(character);
    }
    backslashes = 0;
  }
  result.append(backslashes * 2, '\\');
  result.push_back('\"');
  return result;
}
#else
std::string quoteShellArgument(const std::string &argument) {
  std::string result{"'"};
  for (const char character : argument) {
    if (character == '\'') {
      result += "'\\''";
    } else {
      result.push_back(character);
    }
  }
  result.push_back('\'');
  return result;
}
#endif

std::string shellCommand(const std::vector<std::string> &arguments) {
  std::string command;
  for (const auto &argument : arguments) {
    if (!command.empty()) {
      command.push_back(' ');
    }
    command += quoteShellArgument(argument);
  }
  return command;
}

std::string systemCommand(std::string command) {
#ifdef _WIN32
  return "\"" + command + "\"";
#else
  return command;
#endif
}

int normalizedSystemStatus(const int status) {
  if (status == -1) {
    throw CompileError(Phase::Backend, "cannot start a host process");
  }

#ifdef _WIN32
  return status;
#else
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  throw CompileError(Phase::Backend,
                     "host process ended with an unsupported wait status");
#endif
}

std::string readIfPresent(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return {};
  }
  return std::string{std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>()};
}

void removeExistingExecutable(const std::filesystem::path &path) {
  std::error_code status_error;
  const auto status = std::filesystem::symlink_status(path, status_error);
  if (status_error == std::errc::no_such_file_or_directory ||
      status.type() == std::filesystem::file_type::not_found) {
    return;
  }
  if (status_error) {
    throw CompileError(Phase::Backend,
                       "cannot inspect executable output path '" +
                           path.string() + "': " + status_error.message());
  }

  if (!std::filesystem::is_regular_file(status) &&
      !std::filesystem::is_symlink(status)) {
    throw CompileError(
        Phase::Backend,
        "executable output path is not a replaceable file: '" +
            path.string() + "'");
  }

  std::error_code remove_error;
  const bool removed = std::filesystem::remove(path, remove_error);
  if (remove_error || !removed) {
    auto message = "cannot replace existing executable output '" +
                   path.string() + "'";
    if (remove_error) {
      message += ": " + remove_error.message();
    }
    throw CompileError(Phase::Backend, std::move(message));
  }
}

void removeIncompleteExecutable(const std::filesystem::path &path) noexcept {
  std::error_code status_error;
  const auto status = std::filesystem::symlink_status(path, status_error);
  if (!status_error && (std::filesystem::is_regular_file(status) ||
                        std::filesystem::is_symlink(status))) {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
  }
}

} // namespace

HostCompiler::HostCompiler(HostCompilerConfig config)
    : config_(std::move(config)) {}

HostCompiler HostCompiler::configured() {
  auto executable = environmentValue("MINI_CXX");
  bool is_override = !executable.empty();
  if (executable.empty()) {
    executable = environmentValue("CXX");
    is_override = !executable.empty();
  }
  if (executable.empty()) {
    executable = build_config::configured_cxx;
  }

  const auto family =
      is_override ? familyForOverride(executable)
                  : (build_config::configured_msvc ? CompilerFamily::Msvc
                                                   : CompilerFamily::GnuLike);

  return HostCompiler(HostCompilerConfig{
      std::move(executable), family, build_config::runtime_include_directory,
      build_config::executable_suffix});
}

void HostCompiler::compile(const std::filesystem::path &cpp_source,
                           const std::filesystem::path &executable,
                           const std::filesystem::path &scratch_directory) const {
  const auto absolute_source = absolutePath(cpp_source, "generated C++");
  const auto absolute_executable = absolutePath(executable, "executable");
  const auto absolute_runtime =
      absolutePath(config_.runtime_include_directory, "runtime include");
  const auto absolute_scratch = absolutePath(scratch_directory, "scratch");
  const auto compiler_log = absolute_scratch / "host-compiler.log";

  // A successful invocation must create this build's artifact. Otherwise a
  // stale file could make a compiler that produced nothing look successful.
  removeExistingExecutable(absolute_executable);

  std::vector<std::string> arguments{config_.executable};
  if (config_.family == CompilerFamily::Msvc) {
    arguments.insert(arguments.end(),
                     {"/nologo", "/std:c++17", "/EHsc", "/utf-8",
                      "/I", absolute_runtime.string(), absolute_source.string(),
                      "/Fo:" + (absolute_scratch / "mini-generated.obj").string(),
                      "/Fe:" + absolute_executable.string()});
  } else {
    arguments.insert(arguments.end(),
                     {"-std=c++17", "-I", absolute_runtime.string(),
                      absolute_source.string(), "-o",
                      absolute_executable.string()});
  }

  auto command = shellCommand(arguments);
  command += " > " + quoteShellArgument(compiler_log.string()) + " 2>&1";
  const int exit_code = normalizedSystemStatus(
      std::system(systemCommand(std::move(command)).c_str()));
  if (exit_code != 0) {
    auto message = "host compiler '" + config_.executable +
                   "' failed with exit code " + std::to_string(exit_code);
    const auto compiler_output = readIfPresent(compiler_log);
    if (!compiler_output.empty()) {
      message += "\n" + compiler_output;
    }
    removeIncompleteExecutable(absolute_executable);
    throw CompileError(Phase::Backend, std::move(message));
  }

  std::error_code output_error;
  if (!std::filesystem::is_regular_file(absolute_executable, output_error)) {
    auto message = "host compiler '" + config_.executable +
                   "' reported success but did not create the executable";
    if (output_error &&
        output_error != std::errc::no_such_file_or_directory) {
      message += ": " + output_error.message();
    }
    removeIncompleteExecutable(absolute_executable);
    throw CompileError(Phase::Backend, std::move(message));
  }
}

int HostCompiler::run(const std::filesystem::path &executable) const {
  const auto absolute_executable = absolutePath(executable, "executable");
  const auto command = shellCommand({absolute_executable.string()});
  return normalizedSystemStatus(
      std::system(systemCommand(command).c_str()));
}

} // namespace mini
