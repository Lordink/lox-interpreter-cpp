#include "parser.hpp"
#include "lexer.hpp"

#include <optional>

using std::expected;
using std::make_unique;
using std::pair;
using std::string;
using std::unexpected;
using std::make_pair;

using namespace grammar;

#define FAIL(err) return std::unexpected(err)
#define TODO FAIL("TODO")

// Put successful parse result ExprPtr into `expr`,
// and assign parse result's iterator to `it
// Otherwise - early return
#define UNWRAP_AND_ITER(parse_result, expr, it) \
    if (auto res_tmp = parse_result) { \
        expr = std::move(res_tmp.value().first); \
        it = res_tmp.value().second; \
    } else { \
        FAIL(res_tmp.error()); \
    }

namespace grammar {


ParseResult expression(TokenIter const& it, TokenIter const& end_it) {
    return comparison(it, end_it);
}

ParseResult equality(TokenIter const& start_it, TokenIter const& end_it) {
    static constexpr impl::TokenList<Equals, NotEquals> tok_list;
    using EOp = Expr_Binary::EBinaryOperator;

    // Copying in
    auto it = start_it;
    ExprPtr expr;
    UNWRAP_AND_ITER(comparison(it, end_it), expr, it);

    while (it < end_it && tok_matches_any(tok_list, it)) {
        EOp op = tok_matches<Equals>(it) ? EOp::EqEq : EOp::NotEq;
        it += 1;
        // TODO may not be needed if comparison etc are properly impl'd
        if (it >= end_it) {
            FAIL("Ran out of token list when matching for equality");
        }

        ExprPtr right;
        UNWRAP_AND_ITER(comparison(it, end_it), right, it);
        expr = make_unique<Expr_Binary>(std::move(expr), op, std::move(right));
    }

    return make_pair(std::move(expr), it);
}

ParseResult comparison(TokenIter const& start_it, TokenIter const& end_it) {
    TODO;
}
ParseResult primary(TokenIter const& start_it,
                             TokenIter const& end_it) {
    if (start_it >= end_it) {
        FAIL("Reached end iterator");
    }
    auto it = start_it;

    TokenVariant tok = *it;

    enum class EPrimaryMatchResult {
        Value,
        LeftParen,
        Other
    };

    ExprPtr expr;
    EPrimaryMatchResult res = std::visit(
        [&expr](auto&& var) {
            using T = std::decay_t<decltype(var)>;
            using std::is_same_v;

            if constexpr (is_same_v<T, NumberLiteral>) {
                expr = make_unique<Expr_Literal>(var.value);
                return EPrimaryMatchResult::Value;

            } else if constexpr (is_same_v<T, StringLiteral>) {
                expr = make_unique<Expr_Literal>(var.literal);
                return EPrimaryMatchResult::Value;

            } else if constexpr (is_same_v<T, True>) {
                expr = make_unique<Expr_Literal>(Expr_Literal::True());
                return EPrimaryMatchResult::Value;

            } else if constexpr (is_same_v<T, False>) {
                expr = make_unique<Expr_Literal>(Expr_Literal::False());
                return EPrimaryMatchResult::Value;

            } else if constexpr (is_same_v<T, Nil>) {
                expr = make_unique<Expr_Literal>(Expr_Literal::Nil());
                return EPrimaryMatchResult::Value;

            } else if constexpr (is_same_v<T, LeftParen>) {
                return EPrimaryMatchResult::LeftParen;
            }

            return EPrimaryMatchResult::Other;
        },
    tok);

    switch (res) {
    case EPrimaryMatchResult::Value:
        // Expr was already given a valid value from inside the std::visit()
        assert(expr != nullptr);
        return make_pair(std::move(expr), start_it + 1);

    case EPrimaryMatchResult::Other:
        // TODO ways to format n print the actual failed value
        FAIL("Unexpected literal when parsing primary.");

    case EPrimaryMatchResult::LeftParen:
        // expr should've been left unfilled
        assert(expr == nullptr);
        break;
    }

    it += 1;
    UNWRAP_AND_ITER(expression(it, end_it), expr, it);

    // Next one should be right paren
    if (!std::holds_alternative<RightParen>(*it)) {
        FAIL("After parsing expression in primary(), expected a right paren");
    }

    return make_pair(std::move(expr), it + 1);
}

}

ExprPtr mock_parsed() {
    auto expr = make_unique<Expr_Binary>(
        make_unique<Expr_Unary>(Expr_Unary::EUnaryOperator::Minus,
                                make_unique<Expr_Literal>(123.0)),
        Expr_Binary::EBinaryOperator::Mul,
        make_unique<Expr_Grouping>(make_unique<Expr_Literal>(45.67)));

    // auto expr = make_unique<Expr_Unary>(Expr_Unary::EUnaryOperator::Minus,
    //     make_unique<Expr_Literal>(500.19));

    return expr;
}

std::expected<ExprPtr, std::string> parse(TokenVec const& tokens) {
    auto result = grammar::expression(tokens.begin(), tokens.end());

    return std::move(result).transform([](auto&& pair) {
        return std::move(pair.first);
    });
}
