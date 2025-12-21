#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <functional>

#include "../src/parser.hpp"

using std::function;
using std::holds_alternative;

TEST_CASE("primary() parsing literals", "[parser]") {
    using LiteralVariant = Expr_Literal::LiteralVariant;

    auto [tok, checking_lambda] =
        GENERATE(table<TokenVariant, function<bool(LiteralVariant const&)>>({
            {Nil(),
             [](auto const& var) {
                 return holds_alternative<Expr_Literal::Nil>(var);
             }},
            {True(),
             [](auto const& var) {
                 return holds_alternative<Expr_Literal::True>(var);
             }},
            {False(),
             [](auto const& var) {
                 return holds_alternative<Expr_Literal::False>(var);
             }},
            {StringLiteral("Guh"),
             [](auto const& var) {
                 // Ensure it's the right variant and its context is as expected
                 if (!holds_alternative<Expr_Literal::String>(var)) {
                     return false;
                 }
                 return std::get<Expr_Literal::String>(var).value == "Guh";
             }},
            {NumberLiteral("900.102"),
             [](auto const& var) {
                 // Ensure it's the right variant and its context is as expected
                 if (!holds_alternative<Expr_Literal::Number>(var)) {
                     return false;
                 }
                 return std::get<Expr_Literal::Number>(var).value == 900.102;
             }},
        }));

    TokenVec toks = {tok};

    auto res = grammar::primary(toks.begin(), toks.end());

    REQUIRE(res.has_value());
    auto [expr, it] = std::move(res.value());
    CHECK(it == toks.end());

    auto as_literal = dynamic_cast<Expr_Literal*>(expr.get());
    REQUIRE(as_literal != nullptr);
    REQUIRE(checking_lambda(as_literal->inner));
}