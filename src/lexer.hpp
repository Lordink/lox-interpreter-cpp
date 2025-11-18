#pragma once
/**
 * Lexer for the Lox interpreter
 * Using Concepts and templates for a lot of things to
 * avoid dyn dispatch and relegate as much to comptime as possible.
 * (mainly as an exercise)
 **/

#include <assert.h>
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

// Generic token which has a stable (static) lexeme
// This excludes e.g. literals and idents
template <typename T>
concept StrToken = Token<T> && requires {
    { T::LEXEME } -> std::convertible_to<std::string_view>;
};

#define TOKEN(name_, lexeme, kind)                                             \
    struct name_ {                                                             \
        static constexpr std::string_view LEXEME = lexeme;                     \
        static constexpr std::string_view KIND = #kind;                        \
    };                                                                         \
    static_assert(StrToken<name_>);

TOKEN(LeftParen, "(", LEFT_PAREN);
TOKEN(RightParen, ")", RIGHT_PAREN);
TOKEN(LeftBrace, "{", LEFT_BRACE);
TOKEN(RightBrace, "}", RIGHT_BRACE);
TOKEN(Star, "*", STAR);
TOKEN(Dot, ".", DOT);
TOKEN(Comma, ",", COMMA);
TOKEN(Minus, "-", MINUS);
TOKEN(Plus, "+", PLUS);
TOKEN(Semicol, ";", SEMICOLON);
TOKEN(Assign, "=", EQUAL);
TOKEN(Bang, "!", BANG);
TOKEN(Less, "<", LESS);
TOKEN(Greater, ">", GREATER);
TOKEN(Slash, "/", SLASH);

TOKEN(Equals, "==", EQUAL_EQUAL);
TOKEN(NotEquals, "!=", BANG_EQUAL);
TOKEN(LessOrEq, "<=", LESS_EQUAL);
TOKEN(GreaterOrEq, ">=", GREATER_EQUAL);

// Reserved:
TOKEN(And, "and", AND);
TOKEN(Class, "class", CLASS);
TOKEN(Else, "else", ELSE);
TOKEN(False, "false", FALSE);
TOKEN(For, "for", FOR);
TOKEN(Fun, "fun", FUN);
TOKEN(If, "if", IF);
TOKEN(Nil, "nil", NIL);
TOKEN(Or, "or", OR);
TOKEN(Print, "print", PRINT);
TOKEN(Return, "return", RETURN);
TOKEN(Super, "super", SUPER);
TOKEN(This, "this", THIS);
TOKEN(True, "true", TRUE);
TOKEN(Var, "var", VAR);
TOKEN(While, "while", WHILE);

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

    StringLiteral(std::string literal) : literal(std::move(literal)) {}
};
static_assert(Token<StringLiteral>);

// Number literal
struct NumberLiteral {
    static constexpr std::string_view KIND = "NUMBER";
    std::string literal;
    double value;

    NumberLiteral(std::string literal) {
        this->literal = std::move(literal);
        // ensure it's a valid format
        size_t num_dots = 0;
        for (const char& c : this->literal) {
            if (c == '.') {
                ++num_dots;
            }
        }
        assert(num_dots <= 1);

        this->value = parse_float(this->literal);
    }

    static double parse_float(std::string const& str);
};
static_assert(Token<NumberLiteral>);

// Identifier (e.g. reserved keywords)
struct Ident {
    static constexpr std::string_view KIND = "IDENTIFIER";
    std::string literal;

    Ident(std::string literal) : literal(std::move(literal)) {}
};
static_assert(Token<Ident>);

// All tokens, including those defined manually, not just via TOKEN()
using TokenVariant =
    std::variant<LeftParen, RightParen, LeftBrace, RightBrace, Star, Dot, Comma,
                 Minus, Plus, Semicol, Assign, Bang, Equals, NotEquals, Less,
                 Greater, LessOrEq, GreaterOrEq, Slash, EndOfFile,
                 StringLiteral, NumberLiteral, Ident, And, Class, Else, False,
                 For, Fun, If, Nil, Or, Print, Return, Super, This, True, Var,
                 While>;

// Ultimately, this is what lexer outputs.
// Each entry is either a valid token, or a string with an error
// (keeping it simple for now)
using TokenVec = std::vector<std::expected<TokenVariant, std::string>>;

// Template functions built for internal use
namespace impl {
using std::format;
using std::string;
using std::vector;

// Comptime template-level list of tokens,
// hence the struct is 0-sized
template <Token... Ts> struct TokenList {};

template <Token T> inline string stringify_token(const T& token) {
    return format("{} {} null", T::KIND, T::LEXEME);
}
template <> inline string stringify_token(const EndOfFile& token) {
    return format("{}  null", EndOfFile::KIND);
}
template <> inline string stringify_token(const StringLiteral& token) {
    return format("{} \"{}\" {}", StringLiteral::KIND, token.literal,
                  token.literal);
}
template <> inline string stringify_token(const NumberLiteral& token) {
    const bool is_fract_part_meaningful =
        (token.value - static_cast<double>(static_cast<int>(token.value))) >
        0.0;
    const string formatted_value = is_fract_part_meaningful
                                       ? format("{}", token.value)
                                       : format("{:.1f}", token.value);
    return format("{} {} {}", NumberLiteral::KIND, token.literal,
                  formatted_value);
}
template <> inline string stringify_token(const Ident& token) {
    return format("{} {} null", Ident::KIND, token.literal);
}

[[nodiscard]]
bool is_ident(const char& c) noexcept;
[[nodiscard]]
bool is_digit(const char& c) noexcept;

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
            // Is the char following this tok NOT a num, underscore or letter?
            // If it was, that means we're inside an ident instead
            const bool is_not_ident =
                (remaining_len == tok_len) ||
                (!is_ident(*(it + tok_len)) && !is_digit(*(it + tok_len)));
            if (substr == TTok::LEXEME && is_not_ident) {
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
