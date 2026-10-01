/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include "source.hpp"
#include "token.hpp"

#include <vector>

namespace mini {

std::vector<Token> scan(const SourceText &source);

} // namespace mini
