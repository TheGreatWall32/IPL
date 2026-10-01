/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace mini {

struct SourceSpan {
  std::size_t offset{0};
  std::size_t length{0};
  std::size_t line{1};
  std::size_t column{1};
};

class SourceText {
public:
  SourceText(std::string name, std::string contents);

  static SourceText fromFile(const std::filesystem::path &path);

  const std::string &name() const noexcept { return name_; }
  const std::string &contents() const noexcept { return contents_; }
  std::size_t size() const noexcept { return contents_.size(); }

  SourceSpan span(std::size_t offset, std::size_t length = 0) const;

private:
  void buildLineStarts();

  std::string name_;
  std::string contents_;
  std::vector<std::size_t> line_starts_;
};

} // namespace mini
