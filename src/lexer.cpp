#include <assert.h>
#include <cmath>
#include <optional>
#include <print>
#include <unordered_set>

#include "lexer.hpp"

using impl::TokenList;
using std::format;
using std::holds_alternative;
using std::string;
using std::unordered_set;

// Note: Order matters. Go from longer toks to shorter,
// e.g. making sure that /= is before /
using AllStrTokens =
    TokenList<Equals, NotEquals, LessOrEq, GreaterOrEq, And, Class, Else, False,
              For, Fun, If, Nil, Or, Print, Return, Super, This, True, Var,
              While, LeftParen, RightParen, LeftBrace, RightBrace, Star, Dot,
              Comma, Minus, Plus, Semicol, Assign, Bang, Less, Greater, Slash>;

double NumberLiteral::parse_float(std::string const& str) {
    std::string inner_str = str;
    int32_t dot_digit = 0;
    // Trim trailing 0s, if there's '.'
    if (inner_str.contains('.')) {
        for (auto it = str.end() - 1; it >= str.begin(); --it) {
            if (*it == '0') {
                // Remove 0 from the end
                inner_str.pop_back();
                --dot_digit;
            } else {
                // Stop as soon as we see non-0
                break;
            }
        }
        // If after trimming the last thing is dot, we kick out the dot too
        if (inner_str.back() == '.') {
            // reset it
            dot_digit = 0;
            inner_str.pop_back();
        }
    }

    double output = 0.0;
    for (int32_t i = 0; i < inner_str.size(); ++i) {
        if (inner_str[i] == '.') {
            // Note "+" here, not assignment
            // That's because trailing 0 deletion affects this
            dot_digit += i;
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

// TODO if they have identical forms - just move things to enum
struct ParsedIdent {
    string value;

    ParsedIdent(const char c) { value = c; }
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
    std::variant<ParsedString, ParsedNum, ParsedIdent, std::monostate> parsed;

    LexerState() { parsed = std::monostate{}; }
    void reset() { parsed = std::monostate{}; }
    bool is_empty() const {
        return holds_alternative<std::monostate>(this->parsed);
    }
};

bool impl::is_ident(const char& c) noexcept {
    return (c == '_') || ((c >= 'a') && (c <= 'z')) ||
           ((c >= 'A') && (c <= 'Z'));
}
bool impl::is_digit(const char& c) noexcept { return c >= '0' && c <= '9'; }

// TODO lookeahead parsing?
[[nodiscard]]
TokenVec lex(const string& file_contents, size_t& out_num_errs) {
    TokenVariant token;
    TokenVec tokens;
    // Keeping track for print errors
    LexerState state;

    using impl::is_digit;
    using impl::is_ident;

    auto it = file_contents.begin();
    while (it != file_contents.end()) {
        const char& c = *it;
        dbg(format("Checking {}", c));

        if (holds_alternative<ParsedIdent>(state.parsed)) {
            ParsedIdent& ident = std::get<ParsedIdent>(state.parsed);
            // Could be a digit, cause it's not the starting char
            // of an ident
            if (is_ident(c) || is_digit(c)) {
                ident.value += c;
                ++it;
                continue;
            } else {
                // if (impl::match_str_toks(ReservedKeywords(), tokens,
                // file_contents)) {
                //
                // } else {
                tokens.push_back(Ident(ident.value));
                // }
                state.reset();
            }
        }
        if (holds_alternative<ParsedNum>(state.parsed)) {
            ParsedNum& numState = std::get<ParsedNum>(state.parsed);
            // Dot is only parsed as fractional sign if it's followed by a
            // number
            const bool next_char_is_number =
                ((it + 1) < file_contents.end()) && is_digit(*(it + 1));
            // ... and this is still a number
            if (is_digit(c) || (c == '.' && next_char_is_number)) {
                numState.value += c;
                ++it;
                continue;
            } else {
                // It's not a number, not a dot either. We just stop parsing a
                // number and fall thru
                tokens.push_back(NumberLiteral(numState.value));
                state.reset();
            }
        }
        // Are we encountering a number while NOT parsing number or anything
        // else?
        if (is_digit(c) && state.is_empty()) {
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
                state.reset();
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
        if (c == '\n') {
            state.line_num += 1;
        } else if (ignored_chars.contains(c)) {
            // Do nothing if we run into ignored characters
        } else if (is_ident(c)) {
            state.parsed = ParsedIdent(c);
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
        state.reset();
    } else if (holds_alternative<ParsedNum>(state.parsed)) {
        // Not actually an error, just gotta dump the number
        tokens.push_back(
            NumberLiteral(std::get<ParsedNum>(state.parsed).value));
        state.reset();
    } else if (holds_alternative<ParsedIdent>(state.parsed)) {
        tokens.push_back(Ident(std::get<ParsedIdent>(state.parsed).value));
        state.reset();
    }

    tokens.emplace_back(EndOfFile());

    return tokens;
}