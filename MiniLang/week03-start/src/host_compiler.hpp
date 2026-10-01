/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include <filesystem>
#include <string>

namespace mini {

enum class CompilerFamily {
  GnuLike,
  Msvc,
};

struct HostCompilerConfig {
  std::string executable;
  CompilerFamily family{CompilerFamily::GnuLike};
  std::filesystem::path runtime_include_directory;
  std::string executable_suffix;
};

class HostCompiler {
public:
  explicit HostCompiler(HostCompilerConfig config);

  static HostCompiler configured();

  const HostCompilerConfig &config() const noexcept { return config_; }

  void compile(const std::filesystem::path &cpp_source,
               const std::filesystem::path &executable,
               const std::filesystem::path &scratch_directory) const;
  int run(const std::filesystem::path &executable) const;

private:
  HostCompilerConfig config_;
};

} // namespace mini
