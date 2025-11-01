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

// TODO EOF as a token

template<typename T>
concept Token = requires
{
    { T::KIND } -> std::convertible_to<string_view>;
    // Assert that its constexpr
    // (can't use integral_constant on a string_view)
    requires T::KIND.size() > 0;
};

// Character (one char)-based token, like LEFT_PAREN or PLUS
template<typename T>
concept CharToken = Token<T> && requires
{
    { T::LEXEME } -> std::convertible_to<char>;
    // Assert that its constexpr
    typename std::integral_constant<char, T::LEXEME>;
};

#define CHAR_TOKEN(_name, _lexeme, _kind) \
    struct _name { \
        static constexpr char LEXEME = _lexeme; \
        static constexpr string_view KIND = _kind; \
    }; \
    static_assert(CharToken<_name>);

CHAR_TOKEN(LeftParen,  '(', "LEFT_PAREN");
CHAR_TOKEN(RightParen, ')', "RIGHT_PAREN");
CHAR_TOKEN(LeftBrace,  '{', "LEFT_BRACE");
CHAR_TOKEN(RightBrace, '}', "RIGHT_BRACE");
CHAR_TOKEN(Star,       '*', "STAR");
CHAR_TOKEN(Dot,        '.', "DOT");
CHAR_TOKEN(Comma,      ',', "COMMA");
CHAR_TOKEN(Minus,      '-', "MINUS");
CHAR_TOKEN(Plus,       '+', "PLUS");
CHAR_TOKEN(Semicol,    ';', "SEMICOLON");


// EOF is a special token we nonetheless use
struct EndOfFile {
    static constexpr string_view KIND = "EOF";
};
// Keeping here for consistency. Won't be needed later,
// hopefully, when we just use some function to parse all the lex output tokens
static_assert(Token<EndOfFile>);

/// Writes token's kind to @out, if that token matches @c
template<CharToken T>
bool write_kind_if_matches(char const &c, string &out) {
    if (c == T::LEXEME) {
        out = T::KIND;
        return true;
    }
    return false;
}

// Comptime template-level list of tokens,
// hence the struct is 0-sized
template<Token... Ts>
struct TokenList {};

// Accepts a tokenList, but is templated over CharTokens
// That works, since they are a subtype
template<CharToken... Ts>
bool write_kind_if_matches_any(TokenList<Ts...> _tlist, char const &c, string &out) {
    return ( write_kind_if_matches<Ts>(c, out) || ...);
}

static const unordered_set<char> ignored_chars = {' ', '\n'};
constexpr bool DEBUG_LOG_LEXER = false;


using AllCharTokens = TokenList<
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Star,
    Dot,
    Comma,
    Minus,
    Plus,
    Semicol
>;

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

        if (write_kind_if_matches_any(AllCharTokens(), c, token)) {
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
