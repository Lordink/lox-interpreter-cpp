#pragma once
/**
 * Shared runtime types
 **/
#include <print>
#include <string>
#include <variant>

namespace rt {
using std::monostate;
using std::println;
using std::string;

using Value = std::variant<monostate, bool, double, string>;

static void print_value(Value const& val) {
    std::visit(
        [](auto&& var) {
            using T = std::decay_t<decltype(var)>;
            using std::is_same_v;

            if constexpr (is_same_v<T, monostate>) {
                println("void");
            } else {
                println("{}", var);
            }
        },
        val);
}

} // namespace rt
