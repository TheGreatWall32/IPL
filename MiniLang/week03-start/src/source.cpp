/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#include "source.hpp"

#include "diagnostics.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace mini {

SourceText::SourceText(std::string name, std::string contents)
    : name_(std::move(name)), contents_(std::move(contents)) {
  buildLineStarts();
}

SourceText SourceText::fromFile(const std::filesystem::path &path) {
  std::error_code ignored_error;
  const bool is_regular = std::filesystem::is_regular_file(path, ignored_error);
  if (!is_regular) {
    throw CompileError(Phase::Io, path.string(),
                       "source path is not a regular file");
  }

  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw CompileError(Phase::Io, path.string(), "cannot open source file");
  }

  std::string contents{std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>()};
  if (input.bad()) {
    throw CompileError(Phase::Io, path.string(), "cannot read source file");
  }

  return SourceText(path.string(), std::move(contents));
}

SourceSpan SourceText::span(const std::size_t offset,
                            const std::size_t length) const {
  if (offset > contents_.size() || length > contents_.size() - offset) {
    throw std::out_of_range("source span is outside the source text");
  }

  const auto next_line =
      std::upper_bound(line_starts_.begin(), line_starts_.end(), offset);
  const auto line_index =
      static_cast<std::size_t>(std::distance(line_starts_.begin(), next_line));
  const auto line_start =
      next_line == line_starts_.begin() ? 0 : *std::prev(next_line);

  return SourceSpan{offset, length, line_index + 1, offset - line_start + 1};
}

void SourceText::buildLineStarts() {
  for (std::size_t index = 0; index < contents_.size(); ++index) {
    if (contents_[index] == '\r') {
      if (index + 1 < contents_.size() && contents_[index + 1] == '\n') {
        ++index;
      }
      line_starts_.push_back(index + 1);
    } else if (contents_[index] == '\n') {
      line_starts_.push_back(index + 1);
    }
  }
}

} // namespace mini
