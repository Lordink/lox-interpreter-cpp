//
// Created by user on 16-Nov-25.
//
#include <print>
#include <unordered_set>
#include <optional>
#include <cmath>
#include <assert.h>

#include "lexer.hpp"


using impl::TokenList;
using std::format;
using std::holds_alternative;
using std::string;
using std::unordered_set;

using AllCharTokens =
    TokenList<LeftParen, RightParen, LeftBrace, RightBrace, Star, Dot, Comma,
              Minus, Plus, Semicol, Assign, Bang, Less, Greater, Slash>;
using AllStrTokens = TokenList<Equals, NotEquals, LessOrEq, GreaterOrEq>;

double NumberLiteral::parse_float(std::string const& str) {
    std::string inner_str = str;
    // Trim trailing 0s
    for (auto it = str.end() - 1; it >= str.begin(); --it) {
        if (*it == '0') {
            // Remove 0 from the end
            inner_str.pop_back();
        } else {
            // Stop as soon as we see non-0
            break;
        }
    }
    // If after trimming the last thing is dot, we kick out the dot too
    if (inner_str.back() == '.') {
        inner_str.pop_back();
    }

    size_t dot_digit = 0;
    double output = 0.0;
    for (size_t i = 0; i < inner_str.size(); ++i) {
        if (inner_str[i] == '.') {
            dot_digit = i;
        } else {
            output *= 10.0;
            output += static_cast<double>(inner_str[i] - '0');
        }
    }
    if (dot_digit != 0) {
        output /= static_cast<double>(std::pow(10, dot_digit));
    }

    return output;
}

void print_token_variant(const TokenVariant& tok) {
    std::visit(
        [](const auto& token) {
            std::println("{}", impl::stringify_token(token));
        },
        tok);
}

constexpr bool DEBUG_LOG_LEXER = false;
static const unordered_set<char> ignored_chars = {' ', '\t', '\r'};

static const auto dbg = [](auto const& text) {
    if constexpr (DEBUG_LOG_LEXER) {
        println("{}", text);
    }
};

struct ParsedString {
    string value;
};

struct ParsedNum {
    string value;

    ParsedNum(const char digit_char) { value = digit_char; }
};

struct LexerState {
    size_t line_num = 1;
    std::variant<ParsedString, ParsedNum, std::monostate> parsed;

    LexerState() { parsed = std::monostate{}; }
};

// TODO lookeahead parsing?
TokenVec lex(const string& file_contents, size_t& out_num_errs) {
    TokenVariant token;
    TokenVec tokens;
    // Keeping track for print errors
    LexerState state;

    auto it = file_contents.begin();
    while (it != file_contents.end()) {
        const char& c = *it;
        dbg(format("Checking {}", c));

        if (holds_alternative<ParsedNum>(state.parsed)) {
            ParsedNum& numState = std::get<ParsedNum>(state.parsed);
            // Dot is only parsed as fractional sign if it's followed by a number
            const bool next_char_is_number =
                (it + 1) < file_contents.end() &&
                (*(it + 1) >= '0' || *(it + 1) <= '9');
            // ... and this is still a number
            if ((c >= '0' && c <= '9') || (c == '.' && next_char_is_number)) {
                numState.value += c;
                ++it;
                continue;
            } else {
                // It's not a number, not a dot either. We just stop parsing a
                // number and fall thru
                tokens.push_back(NumberLiteral(numState.value));
                state.parsed = std::monostate{};
            }
        }
        // Are we encountering a number while NOT parsing number or anything
        // else?
        if (c >= '0' && c <= '9' &&
            holds_alternative<std::monostate>(state.parsed)) {
            state.parsed = ParsedNum(c);
            ++it;
            continue;
        }
        // Check for string start/end
        if (c == '"') {
            // We are ending a string literal?
            if (holds_alternative<ParsedString>(state.parsed)) {
                tokens.emplace_back(StringLiteral(
                    std::move(std::get<ParsedString>(state.parsed).value)));
                // reset parsed
                state.parsed = std::monostate{};
            } else {
                // We are starting a string literal
                state.parsed.emplace<ParsedString>();
            }
            ++it;
            continue;
        }
        // Check for ongoing string
        if (holds_alternative<ParsedString>(state.parsed)) {
            std::get<ParsedString>(state.parsed).value += c;
            ++it;
            // Make sure we are still tracking line num
            if (c == '\n') {
                state.line_num += 1;
            }
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
    if (holds_alternative<ParsedString>(state.parsed)) {
        const string err_msg =
            format("[line {}] Error: Unterminated string.", state.line_num);
        tokens.emplace_back(std::unexpected(err_msg));
        out_num_errs += 1;
    } else if (holds_alternative<ParsedNum>(state.parsed)) {
        // Not actually an error, just gotta dump the number
        tokens.push_back(
            NumberLiteral(std::get<ParsedNum>(state.parsed).value));
        state.parsed = std::monostate{};
    }

    tokens.emplace_back(EndOfFile());

    return tokens;
}