/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include "ast.hpp"
#include "source.hpp"
#include "token.hpp"

#include <vector>

namespace mini {

Program parse(const SourceText &source, const std::vector<Token> &tokens);

} // namespace mini
