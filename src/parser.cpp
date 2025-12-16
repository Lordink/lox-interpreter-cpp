#include "parser.hpp"
#include "lexer.hpp"

using std::expected;
using std::make_unique;
using std::pair;
using std::string;
using std::unexpected;

using namespace grammar;

namespace grammar {

ParseResult expression(TokenVec::const_iterator it) {
    return comparison(it);
}

ParseResult comparison(TokenVec::const_iterator it) {
    return std::unexpected("TODO");
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
