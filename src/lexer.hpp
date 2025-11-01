#pragma once
/*
 * Lexer for the Lox interpreter
 */

#include <iostream>
#include <string>
#include <format>
#include <vector>
#include <stdexcept>
#include <unordered_set>

using std::string;
using std::cout;
using std::endl;
using std::format;
using std::vector;
using std::string_view;
using std::unordered_set;


template<typename T>
concept Token = requires
{
    { T::LEXEME } -> std::convertible_to<char>;
    { T::KIND } -> std::convertible_to<string_view>;
    // Assert that they are constexpr
    typename std::integral_constant<char, T::LEXEME>;
    // Can't use integral_constant on a string_view
    requires T::KIND.size() > 0;
};

#define DECLARE_SIMPLE_TOKEN(_name, _lexeme, _kind) \
    struct _name { \
        static constexpr char LEXEME = _lexeme; \
        static constexpr string_view KIND = _kind; \
    }; \
    static_assert(Token<_name>);

DECLARE_SIMPLE_TOKEN(LeftParen, '(', "LEFT_PAREN");
DECLARE_SIMPLE_TOKEN(RightParen, ')', "RIGHT_PAREN");
DECLARE_SIMPLE_TOKEN(LeftBrace, '{', "LEFT_BRACE");
DECLARE_SIMPLE_TOKEN(RightBrace, '}', "RIGHT_BRACE");

constexpr bool DEBUG_LOG_LEXER = false;

/// Writes token's kind to @out, if that token matches @c
template<Token T>
bool write_kind_if_matches(char const &c, string &out) {
    if (c == T::LEXEME) {
        out = T::KIND;
        return true;
    }
    return false;
}

template<Token... Ts>
bool write_kind_if_matches_any(char const &c, string &out) {
    return ( write_kind_if_matches<Ts>(c, out) || ...);
}

static const unordered_set<char> ignored_chars = {' ', '\n'};

[[nodiscard]]
inline vector<string> lex(const string &file_contents) {
    string token;
    vector<string> tokens;
    static const auto dbg = [](auto const &text) {
        if constexpr (DEBUG_LOG_LEXER) {
            cout << text << endl;
        }
    };

    for (char const &c: file_contents) {
        dbg(format("Checking {}", c));

        if (write_kind_if_matches_any<LeftParen, RightParen, LeftBrace, RightBrace>(c, token)) {
            token += format(" {} null", c);
            tokens.push_back(token);
        } else if (ignored_chars.contains(c)) {
            // Do nothing if we run into ignored characters
        } else {
            throw std::runtime_error("Unrecognized token");
        }
    }
    tokens.push_back("EOF  null"); // NOLINT(*-use-emplace)

    return tokens;
}
