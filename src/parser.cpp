#include "parser.hpp"
#include "lexer.hpp"

#include <functional>
#include <optional>

using std::expected;
using std::holds_alternative;
using std::make_pair;
using std::make_unique;
using std::pair;
using std::string;
using std::unexpected;

using namespace grammar;

using EBinOp = Expr_Binary::EBinaryOperator;

#define FAIL(err) return std::unexpected(err)
#define TODO FAIL("TODO")

// Put successful parse result ExprPtr into `expr`,
// and assign parse result's iterator to `it
// Otherwise - early return
// Includes bounds check.
#define UNWRAP_AND_ITER(fn, expr, it, end_it)                                  \
    if (auto res_tmp = bounds_check(fn, it, end_it)) {                         \
        expr = std::move(res_tmp.value().first);                               \
        it = res_tmp.value().second;                                           \
    } else {                                                                   \
        FAIL(res_tmp.error());                                                 \
    }

namespace grammar {

ParseResult expression(TokenIter const& start_it, TokenIter const& end_it) {
    return bounds_check(equality, start_it, end_it);
}

ParseResult equality(TokenIter const& start_it, TokenIter const& end_it) {
    static constexpr impl::TokenList<Equals, NotEquals> tok_list;

    // Copying in
    auto it = start_it;
    ExprPtr expr;
    UNWRAP_AND_ITER(comparison, expr, it, end_it);

    while (it < end_it && tok_matches_any(tok_list, it)) {
        EBinOp op = tok_matches<Equals>(it) ? EBinOp::EqEq : EBinOp::NotEq;
        it += 1;

        ExprPtr right;
        UNWRAP_AND_ITER(comparison, right, it, end_it);
        expr = make_unique<Expr_Binary>(std::move(expr), op, std::move(right));
    }

    return make_pair(std::move(expr), it);
}

ParseResult comparison(TokenIter const& start_it, TokenIter const& end_it) {
    auto it = start_it;
    static constexpr impl::TokenList<Greater, GreaterOrEq, Less, LessOrEq>
        tok_list;

    ExprPtr expr;
    UNWRAP_AND_ITER(term, expr, it, end_it);
    while (it < end_it && tok_matches_any(tok_list, it)) {

        EBinOp op;
        if (tok_matches<Greater>(it)) {
            op = EBinOp::Greater;
        } else if (tok_matches<GreaterOrEq>(it)) {
            op = EBinOp::GreaterOrEq;
        } else if (tok_matches<Less>(it)) {
            op = EBinOp::Less;
        } else {
            op = EBinOp::LessOrEq;
        }

        it += 1;

        ExprPtr right;
        UNWRAP_AND_ITER(term, right, it, end_it);
        expr = make_unique<Expr_Binary>(std::move(expr), op, std::move(right));
    }

    return make_pair(std::move(expr), it);
}
ParseResult term(TokenIter const& start_it, TokenIter const& end_it) {
    auto it = start_it;
    static constexpr impl::TokenList<Minus, Plus> tok_list;

    ExprPtr expr;
    UNWRAP_AND_ITER(factor, expr, it, end_it);
    while (it < end_it && tok_matches_any(tok_list, it)) {
        EBinOp op = tok_matches<Minus>(it) ? EBinOp::Minus : EBinOp::Plus;
        it += 1;

        ExprPtr right;
        UNWRAP_AND_ITER(factor, right, it, end_it);
        expr = make_unique<Expr_Binary>(std::move(expr), op, std::move(right));
    }

    return make_pair(std::move(expr), it);
}
ParseResult factor(TokenIter const& start_it, TokenIter const& end_it) {
    auto it = start_it;
    static constexpr impl::TokenList<Slash, Star> tok_list;

    ExprPtr expr;
    UNWRAP_AND_ITER(unary, expr, it, end_it);
    while (it < end_it && tok_matches_any(tok_list, it)) {
        EBinOp op = tok_matches<Slash>(it) ? EBinOp::Div : EBinOp::Mul;
        it += 1;

        ExprPtr right;
        UNWRAP_AND_ITER(unary, right, it, end_it);
        expr = make_unique<Expr_Binary>(std::move(expr), op, std::move(right));
    }

    return make_pair(std::move(expr), it);
}
ParseResult unary(TokenIter const& start_it, TokenIter const& end_it) {
    using EUnaryOp = Expr_Unary::EUnaryOperator;
    auto it = start_it;

    TokenVariant tok = *it;

    EUnaryOp unary_op;
    if (holds_alternative<Bang>(tok)) {
        unary_op = EUnaryOp::Bang;
    } else if (holds_alternative<Minus>(tok)) {
        unary_op = EUnaryOp::Minus;
    } else {
        // Just pass thru should be sufficient
        // NOTE: not applying bounds check, cause we haven't moved iterator
        return primary(it, end_it);
    }
    ExprPtr inner_expr;
    it += 1;
    UNWRAP_AND_ITER(unary, inner_expr, it, end_it);
    return make_pair(make_unique<Expr_Unary>(unary_op, std::move(inner_expr)),
                     it);
}
ParseResult primary(TokenIter const& start_it, TokenIter const& end_it) {
    auto it = start_it;

    TokenVariant tok = *it;

    enum class EPrimaryMatchResult { Value, LeftParen, Other };

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
    UNWRAP_AND_ITER(expression, expr, it, end_it);

    // Next one should be right paren
    if (!std::holds_alternative<RightParen>(*it)) {
        FAIL("After parsing expression in primary(), expected a right paren");
    }
    // wrap into grouping
    return make_pair(make_unique<Expr_Grouping>(std::move(expr)), it + 1);
}

} // namespace grammar

void pprint::Visitor_PPrint::visit_unary(Expr_Unary const& unary) const {
    const char op = unary.op == Expr_Unary::EUnaryOperator::Minus ? '-' : '!';
    print("({} ", op);
    unary.inner->accept(*this);
    print(")");
}
void pprint::Visitor_PPrint::visit_literal(Expr_Literal const& literal) const {
    std::visit(
        [](auto&& var) {
            using T = std::decay_t<decltype(var)>;
            using std::is_same_v;
            if constexpr (is_same_v<T, Expr_Literal::Number>) {
                // Is it a whole num?
                if (var.value == std::trunc(var.value)) {
                    print("{:.1f}", var.value);
                } else {
                    print("{}", var.value);
                }
            } else if constexpr (is_same_v<T, Expr_Literal::String>) {
                print("{}", var.value);
            } else if constexpr (is_same_v<T, Expr_Literal::True>) {
                print("true");
            } else if constexpr (is_same_v<T, Expr_Literal::False>) {
                print("false");
            } else if constexpr (is_same_v<T, Expr_Literal::Nil>) {
                print("nil");
            } else {
                std::unreachable();
            }
        },
        literal.inner);
}
void pprint::Visitor_PPrint::visit_binary(Expr_Binary const& binary) const {
    std::string op;
    switch (binary.op) {
    case Expr_Binary::EBinaryOperator::EqEq:
        op = "==";
        break;
    case Expr_Binary::EBinaryOperator::NotEq:
        op = "!=";
        break;
    case Expr_Binary::EBinaryOperator::Less:
        op = "<";
        break;
    case Expr_Binary::EBinaryOperator::LessOrEq:
        op = "<=";
        break;
    case Expr_Binary::EBinaryOperator::Greater:
        op = ">";
        break;
    case Expr_Binary::EBinaryOperator::GreaterOrEq:
        op = ">=";
        break;
    case Expr_Binary::EBinaryOperator::Plus:
        op = "+";
        break;
    case Expr_Binary::EBinaryOperator::Minus:
        op = "-";
        break;
    case Expr_Binary::EBinaryOperator::Mul:
        op = "*";
        break;
    case Expr_Binary::EBinaryOperator::Div:
        op = "/";
        break;
    }
    print("({} ", op);
    binary.left->accept(*this);
    print(" ");
    binary.right->accept(*this);
    print(")");
}
void pprint::Visitor_PPrint::visit_grouping(
    Expr_Grouping const& grouping) const {
    print("(group ");
    grouping.inner->accept(*this);
    print(")");
}

ExprPtr mock_parsed() {
    auto expr = make_unique<Expr_Binary>(
        make_unique<Expr_Unary>(Expr_Unary::EUnaryOperator::Minus,
                                make_unique<Expr_Literal>(123.0)),
        Expr_Binary::EBinaryOperator::Mul,
        make_unique<Expr_Grouping>(make_unique<Expr_Literal>(45.67)));

    return expr;
}

std::expected<ExprPtr, std::string> parse(TokenVec const& tokens) {
    auto result = grammar::expression(tokens.begin(), tokens.end());

    return std::move(result).transform(
        [](auto&& pair) { return std::move(pair.first); });
}
