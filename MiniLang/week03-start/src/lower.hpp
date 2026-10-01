/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include "ast.hpp"

namespace mini {

// This version has no syntax to lower; preserve the parsed tree.
Program lower(Program program);

} // namespace mini
