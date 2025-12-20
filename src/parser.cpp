#include "parser.hpp"
#include "lexer.hpp"

using std::expected;
using std::make_unique;
using std::pair;
using std::string;
using std::unexpected;

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


ParseResult expression(TokenVec::const_iterator const& it, TokenVec::const_iterator const& end_it) {
    return comparison(it, end_it);
}

ParseResult equality(TokenVec::const_iterator const& start_it, TokenVec::const_iterator const& end_it) {
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

    return std::make_pair(std::move(expr), it);
}

ParseResult comparison(TokenVec::const_iterator const& it, TokenVec::const_iterator const& end_it) {
    TODO;
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
    auto it = tokens.begin();
    auto result = grammar::expression(it);

    return std::move(result).transform([](auto&& pair) {
        return std::move(pair.first);
    });
}
