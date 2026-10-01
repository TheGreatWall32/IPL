/*
 * Educational Mini compiler.
 * Demonstrates the translation pipeline for documented Mini 1.0 programs.
 * Diagnostics and edge-case handling are intentionally limited.
 * See the project specification and documented limitations.
 */

#pragma once

#include <string>

namespace mini {

std::string formatNumberForDisplay(double value);
std::string formatNumberForCpp(double value);

} // namespace mini
