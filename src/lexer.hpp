#pragma once

#include <cstring>
#include <iostream>
#include <string>
#include <format>
#include <vector>

using std::string;
using std::cout;
using std::endl;
using std::format;
using std::vector;

/*
 * Lexer for the Lox interpreter
 */

constexpr bool DEBUG_LOG_LEXER = false;

[[nodiscard]]
inline vector<string> lex(const string& file_contents) {
    string token;
    vector<string> tokens;
    static const auto dbg = [](auto const& text) {
        if constexpr (DEBUG_LOG_LEXER) {
            cout << text << endl;
        }
    };

    for (char const &c : file_contents) {
        dbg(format("Checking {}", c));

        switch (c) {
            case '(':
                token = "LEFT_PAREN";
                break;
            case ')':
                token = "RIGHT_PAREN";
                break;
            case '{':
                token = "LEFT_BRACE";
                break;
            case '}':
                token = "RIGHT_BRACE";
                break;
            default:
                // Ignoring unsupported output
                break;
        }
        token += format(" {} null", c);
        tokens.push_back(token);
    }
    tokens.push_back("EOF  null"); // NOLINT(*-use-emplace)

    return tokens;
}
