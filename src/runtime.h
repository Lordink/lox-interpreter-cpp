#pragma once
/**
 * Shared runtime types
 **/
#include <string>
#include <variant>

namespace rt {
using std::monostate;
using std::string;

using Value = std::variant<monostate, bool, double, string>;
} // namespace rt
