//
// Created by user on 16-Nov-25.
//
#include <unordered_set>
#include <print>

#include "lexer.hpp"

#include <optional>

using impl::TokenList;
using std::unordered_set;
using std::format;
using std::string;
using impl::TokenList;

using AllCharTokens =
    TokenList<LeftParen, RightParen, LeftBrace, RightBrace, Star, Dot, Comma,
              Minus, Plus, Semicol, Assign, Bang, Less, Greater, Slash>;
using AllStrTokens = TokenList<Equals, NotEquals, LessOrEq, GreaterOrEq>;

void print_token_variant(const TokenVariant& tok) {
    std::visit(
        [](const auto& token) {
            std::println("{}", impl::stringify_token(token));
        },
        tok);
}

constexpr bool DEBUG_LOG_LEXER = false;

struct LexerState {
    size_t line_num = 1;
    std::optional<string> parsed_string;
};

TokenVec lex(const string& file_contents, size_t& out_num_errs) {
    static const auto dbg = [](auto const& text) {
        if constexpr (DEBUG_LOG_LEXER) {
            println("{}", text);
        }
    };
    static const unordered_set<char> ignored_chars = {' ', '\t', '\r'};

    TokenVariant token;
    TokenVec tokens;
    // Keeping track for print errors
    LexerState state;

    auto it = file_contents.begin();
    while (it != file_contents.end()) {
        const char& c = *it;
        dbg(format("Checking {}", c));

        // Check for string start/end
        if (c == '"') {
            // We are ending a string literal?
            if (state.parsed_string.has_value()) {
                tokens.emplace_back(StringLiteral(std::move(*state.parsed_string)));
                state.parsed_string.reset();
            } else {
                // We are starting a string literal
                state.parsed_string.emplace("");
            }
            ++it;
            continue;
        }
        // Check for ongoing string literal
        if (state.parsed_string.has_value()) {
            *state.parsed_string += c;
            ++it;
            continue;
        }
        // Check for comment, after string
        if (c == '/' && ((it + 1) < file_contents.end()) && *(it + 1) == '/') {
            while (it != file_contents.end() && *it != '\n') {
                ++it;
            }
            // increment one last time to drop us to the next line
            if (it != file_contents.end()) {
                ++it;
                state.line_num += 1;
            }
            continue;
        }
        if (impl::match_str_toks(AllStrTokens(), tokens, it, file_contents)) {
            continue;
        }
        if (impl::match_char_toks(AllCharTokens(), c, token)) {
            tokens.push_back(token);
        } else if (c == '\n') {
            state.line_num += 1;
        } else if (ignored_chars.contains(c)) {
            // Do nothing if we run into ignored characters
        } else {
            // Failure case. Add as a string.
            const string err_msg = format(
                "[line {}] Error: Unexpected character: {}", state.line_num, c);
            tokens.emplace_back(std::unexpected(err_msg));
            out_num_errs += 1;
        }

        ++it;
    }

    // Forgot to terminate string
    if (state.parsed_string.has_value()) {
        const string err_msg = format(
            "[line {}] Error: Unterminated string.", state.line_num);
        tokens.emplace_back(std::unexpected(err_msg));
        out_num_errs += 1;
    }

    tokens.emplace_back(EndOfFile());

    return tokens;
}