/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include "ast.hpp"
#include "resolver.hpp"

#include <string>

namespace mini {

std::string emitCpp(const Program &program, const Resolution &resolution);

} // namespace mini
