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
#include <variant>
#include <expected>
#include <print>

template<typename T>
concept Token = requires
{
    { T::KIND } -> std::convertible_to<std::string_view>;
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
        static constexpr std::string_view KIND = _kind; \
    }; \
    static_assert(CharToken<_name>);

CHAR_TOKEN(LeftParen, '(', "LEFT_PAREN");
CHAR_TOKEN(RightParen, ')', "RIGHT_PAREN");
CHAR_TOKEN(LeftBrace, '{', "LEFT_BRACE");
CHAR_TOKEN(RightBrace, '}', "RIGHT_BRACE");
CHAR_TOKEN(Star, '*', "STAR");
CHAR_TOKEN(Dot, '.', "DOT");
CHAR_TOKEN(Comma, ',', "COMMA");
CHAR_TOKEN(Minus, '-', "MINUS");
CHAR_TOKEN(Plus, '+', "PLUS");
CHAR_TOKEN(Semicol, ';', "SEMICOLON");


// EOF is a special token we nonetheless use
struct EndOfFile {
    static constexpr std::string_view KIND = "EOF";
};
// Keeping here for consistency and to avoid surprises later
static_assert(Token<EndOfFile>);

using TokenVariant = std::variant<
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Star,
    Dot,
    Comma,
    Minus,
    Plus,
    Semicol,
    EndOfFile
>;

// Template functions built for internal use
namespace impl {
    using std::string;
    using std::cout;
    using std::endl;
    using std::format;
    using std::vector;
    using std::string_view;
    using std::unordered_set;
    using std::variant;
    using std::expected;
    using std::println;

    /// Writes token's kind to @out, if that token matches @c
    template<CharToken T>
    bool set_if_matches(char const &c, TokenVariant &out) {
        if (c == T::LEXEME) {
            out = T();
            return true;
        }
        return false;
    }

    // Comptime template-level list of tokens,
    // hence the struct is 0-sized
    template<Token... Ts>
    struct TokenList {
    };

    // Accepts a tokenList, but is templated over CharTokens
    // That works, since they are a subtype
    template<CharToken... Ts>
    bool set_if_matches_any(TokenList<Ts...> _tlist, char const &c, TokenVariant &out) {
        return ( set_if_matches<Ts>(c, out) || ... );
    }


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

    template<Token T>
    string stringify_token(const T &token) {
        // TODO value when toks have values
        return format("{}  null", T::KIND);
    }

    template<CharToken T>
    string stringify_token(const T &token) {
        // TODO value when toks have values
        return format("{} {} null", T::KIND, T::LEXEME);
    }
}


inline void print_token_variant(const TokenVariant &tok) {
    std::visit([](const auto &token) {
        std::println("{}", impl::stringify_token(token));
    }, tok);
}

static const std::unordered_set<char> ignored_chars = {' '};
constexpr bool DEBUG_LOG_LEXER = false;

[[nodiscard]]
inline std::vector<std::expected<TokenVariant, std::string>> lex(
    const std::string &file_contents,
    size_t& out_num_errs)
{
    static const auto dbg = [](auto const &text) {
        if constexpr (DEBUG_LOG_LEXER) {
            println("{}", text);
        }
    };
    TokenVariant token;
    std::vector<std::expected<TokenVariant, std::string>> tokens;
    // Keeping track for print errors
    size_t line_num = 1;

    for (char const &c: file_contents) {
        dbg(impl::format("Checking {}", c));

        if (impl::set_if_matches_any(impl::AllCharTokens(), c, token)) {
            tokens.push_back(token);
        } else if (c == '\n') {
            // New line symbol. Ignore, but bump line_num to keep track
            line_num += 1;
        } else if (ignored_chars.contains(c)) {
            // Do nothing if we run into ignored characters
        } else {
            // Failure case. Add as a string.
            const std::string err_msg = std::format(
                "[line {}] Error: Unexpected character: {}",
                line_num,
                c
            );
            tokens.emplace_back(std::unexpected(err_msg));
            out_num_errs += 1;
        }
    }
    tokens.emplace_back(EndOfFile());

    return tokens;
}
