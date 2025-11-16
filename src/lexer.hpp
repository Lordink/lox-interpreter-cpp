#pragma once
/**
 * Lexer for the Lox interpreter
 * Using Concepts and templates for a lot of things to
 * avoid dyn dispatch and relegate as much to comptime as possible.
 * (mainly as an exercise)
 **/

#include <expected>
#include <format>
#include <string>
#include <variant>
#include <vector>

// "Base" concept for a token, i.e. smth that has a kind
// e.g. "STAR", "DOT" or "LEFT_PAREN"
template <typename T>
concept Token = requires {
    { T::KIND } -> std::convertible_to<std::string_view>;
    // Assert that its constexpr
    // (can't use integral_constant on a string_view)
    requires T::KIND.size() > 0;
};

// Character (one char)-based token, like LEFT_PAREN or PLUS
template <typename T>
concept CharToken = Token<T> && requires {
    { T::LEXEME } -> std::convertible_to<char>;
    // Assert that its constexpr
    typename std::integral_constant<char, T::LEXEME>;
};

#define CHAR_TOKEN(_name, _lexeme, _kind)                                      \
    struct _name {                                                             \
        static constexpr char LEXEME = _lexeme;                                \
        static constexpr std::string_view KIND = _kind;                        \
    };                                                                         \
    static_assert(CharToken<_name>);

// Multi-char token.
// Could just make all CharTokens StrTokens, but
// would lose on some perf and simplicity (arguable)
template <typename T>
concept StrToken = Token<T> && requires {
    { T::LEXEME } -> std::convertible_to<std::string_view>;
    // Make sure it's bigger than 1 char, cause in that case it would just
    // be a CharToken innit
    requires T::LEXEME.size() > 1;
};

#define STR_TOKEN(_name, _lexeme, _kind)                                       \
    struct _name {                                                             \
        static constexpr std::string_view LEXEME = _lexeme;                    \
        static constexpr std::string_view KIND = _kind;                        \
    };                                                                         \
    static_assert(StrToken<_name>);

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
CHAR_TOKEN(Assign, '=', "EQUAL");
CHAR_TOKEN(Bang, '!', "BANG");
CHAR_TOKEN(Less, '<', "LESS");
CHAR_TOKEN(Greater, '>', "GREATER");
CHAR_TOKEN(Slash, '/', "SLASH");

STR_TOKEN(Equals, "==", "EQUAL_EQUAL");
STR_TOKEN(NotEquals, "!=", "BANG_EQUAL");
STR_TOKEN(LessOrEq, "<=", "LESS_EQUAL");
STR_TOKEN(GreaterOrEq, ">=", "GREATER_EQUAL");

// EOF is a special token we nonetheless use
struct EndOfFile {
    static constexpr std::string_view KIND = "EOF";
};
// Keeping here for consistency and to avoid surprises later
static_assert(Token<EndOfFile>);

// String literal
struct StringLiteral {
    static constexpr std::string_view KIND = "STRING";
    std::string literal;

    StringLiteral(std::string literal): literal(std::move(literal)) {}
};
static_assert(Token<StringLiteral>);

using TokenVariant =
    std::variant<LeftParen, RightParen, LeftBrace, RightBrace, Star, Dot, Comma,
                 Minus, Plus, Semicol, Assign, Bang, Equals, NotEquals, Less,
                 Greater, LessOrEq, GreaterOrEq, Slash, EndOfFile, StringLiteral>;

// Ultimately, this is what lexer outputs.
// Each entry is either a valid token, or a string with an error
// (keeping it simple for now)
using TokenVec = std::vector<std::expected<TokenVariant, std::string>>;

// Template functions built for internal use
namespace impl {
using std::format;
using std::string;
using std::vector;

/// Writes token's kind to @out, if that token matches @c
template <CharToken T> bool set_if_matches(char const& c, TokenVariant& out) {
    if (c == T::LEXEME) {
        out = T();
        return true;
    }
    return false;
}

// Comptime template-level list of tokens,
// hence the struct is 0-sized
template <Token... Ts> struct TokenList {};

// Accepts a tokenList, but is templated over CharTokens
// That works, since they are a subtype
template <CharToken... Ts>
bool match_char_toks(TokenList<Ts...> _tlist, char const& c,
                     TokenVariant& out) {
    return (set_if_matches<Ts>(c, out) || ...);
}

template <Token T> inline string stringify_token(const T& token) {
    // TODO value when toks have values
    return format("{}  null", T::KIND);
}
template <CharToken T> inline string stringify_token(const T& token) {
    // TODO value when toks have values
    return format("{} {} null", T::KIND, T::LEXEME);
}
template <StrToken T> inline string stringify_token(const T& token) {
    // TODO value when toks have values
    return format("{} {} null", T::KIND, T::LEXEME);
}
template<> inline string stringify_token(const StringLiteral& token) {
    // TODO value when toks have values
    return format("{} \"{}\" {}", StringLiteral::KIND, token.literal, token.literal);
}

// If the token matches TTok's lexeme - we advance the it iterator accordingly,
// and add the token to tokens
template <StrToken TTok>
[[nodiscard]]
inline bool match_str_tok(TokenVec& tokens, string::const_iterator& it,
                          const size_t remaining_len) {
    if (TTok::LEXEME.starts_with(*it)) {
        constexpr size_t tok_len = TTok::LEXEME.size();
        if (remaining_len >= tok_len) {
            const auto substr = string(it, it + tok_len);
            if (substr == TTok::LEXEME) {
                tokens.push_back(TTok());
                it += tok_len;
                return true;
            }
        }
    }

    return false;
}

// Given a list of toks - call match_str_tok() until one of them yields true
// NOTE: We are assuming that the ones that ret false do NOT modify iterator or
// TokenVec
template <StrToken... Ts>
[[nodiscard]]
constexpr bool match_str_toks(TokenList<Ts...> tlist, TokenVec& tokens,
                              string::const_iterator& it, string const& str) {
    const size_t remaining_len = std::distance(it, str.end());
    return (match_str_tok<Ts>(tokens, it, remaining_len) || ...);
}

} // namespace impl

void print_token_variant(const TokenVariant& tok);

[[nodiscard]]
TokenVec lex(const std::string& file_contents, size_t& out_num_errs);
