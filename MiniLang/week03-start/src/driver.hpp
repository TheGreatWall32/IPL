/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include "cli.hpp"
#include "source.hpp"

#include <iosfwd>

namespace mini {

int executeCommand(const CommandLine &command, const SourceText &source,
                   std::ostream &output);

} // namespace mini
