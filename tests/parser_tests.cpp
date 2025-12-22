#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <functional>

#include "../src/parser.h"

using std::function;
using std::holds_alternative;

using EUnaryOp = Expr_Unary::EUnaryOperator;
using EBinOp = Expr_Binary::EBinaryOperator;

TEST_CASE("primary() parsing", "[parser]") {
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

TEST_CASE("unary() parsing: -nil", "[parser]") {
    TokenVec toks = {Minus(), Nil()};

    auto res = grammar::unary(toks.begin(), toks.end());
    REQUIRE(res.has_value());
    auto [expr, it] = std::move(res.value());
    CHECK(it == toks.end());

    auto as_unary = dynamic_cast<Expr_Unary*>(expr.get());
    REQUIRE(as_unary != nullptr);
    REQUIRE(as_unary->op == EUnaryOp::Minus);
    auto inner_primary = dynamic_cast<Expr_Literal*>(as_unary->inner.get());
    REQUIRE(inner_primary != nullptr);
    REQUIRE(holds_alternative<Expr_Literal::Nil>(inner_primary->inner));
}

TEST_CASE("unary() parsing: !true", "[parser]") {
    TokenVec toks = {Bang(), True()};

    auto res = grammar::unary(toks.begin(), toks.end());
    REQUIRE(res.has_value());
    auto [expr, it] = std::move(res.value());
    CHECK(it == toks.end());

    auto as_unary = dynamic_cast<Expr_Unary*>(expr.get());
    REQUIRE(as_unary != nullptr);
    REQUIRE(as_unary->op == EUnaryOp::Bang);
    auto inner_primary = dynamic_cast<Expr_Literal*>(as_unary->inner.get());
    REQUIRE(inner_primary != nullptr);
    REQUIRE(holds_alternative<Expr_Literal::True>(inner_primary->inner));
}

TEST_CASE("unary() parsing: -19", "[parser]") {
    TokenVec toks = {Minus(), NumberLiteral("19")};

    auto res = grammar::unary(toks.begin(), toks.end());
    REQUIRE(res.has_value());
    auto [expr, it] = std::move(res.value());
    CHECK(it == toks.end());

    auto as_unary = dynamic_cast<Expr_Unary*>(expr.get());
    REQUIRE(as_unary != nullptr);
    REQUIRE(as_unary->op == EUnaryOp::Minus);
    auto inner_primary = dynamic_cast<Expr_Literal*>(as_unary->inner.get());
    REQUIRE(inner_primary != nullptr);
    REQUIRE(holds_alternative<Expr_Literal::Number>(inner_primary->inner));
    REQUIRE(std::get<Expr_Literal::Number>(inner_primary->inner).value == 19.0);
}

TEST_CASE("unary() parsing: 'Blah bleh'", "[parser]") {
    TokenVec toks = {StringLiteral("Blah bleh")};

    auto res = grammar::unary(toks.begin(), toks.end());
    REQUIRE(res.has_value());
    auto [expr, it] = std::move(res.value());
    CHECK(it == toks.end());

    auto as_literal = dynamic_cast<Expr_Literal*>(expr.get());
    REQUIRE(as_literal != nullptr);
    REQUIRE(holds_alternative<Expr_Literal::String>(as_literal->inner));
    REQUIRE(std::get<Expr_Literal::String>(as_literal->inner).value ==
            "Blah bleh");
}
TEST_CASE("factor() parsing: 5 / 6", "[parser]") {
    TokenVec toks = {NumberLiteral("5"), Slash(), NumberLiteral("6")};

    auto res = grammar::factor(toks.begin(), toks.end());
    REQUIRE(res.has_value());
    auto [expr, it] = std::move(res.value());
    CHECK(it == toks.end());

    auto as_binary = dynamic_cast<Expr_Binary*>(expr.get());
    REQUIRE(as_binary != nullptr);
    REQUIRE(as_binary->op == EBinOp::Div);
    auto left = dynamic_cast<Expr_Literal*>(as_binary->left.get());
    REQUIRE(left != nullptr);
    auto right = dynamic_cast<Expr_Literal*>(as_binary->right.get());
    REQUIRE(right != nullptr);

    REQUIRE(std::get<Expr_Literal::Number>(left->inner).value == 5.0);
    REQUIRE(std::get<Expr_Literal::Number>(right->inner).value == 6.0);
}

TEST_CASE("factor() parsing: -15 * 2.5", "[parser]") {
    TokenVec toks = {Minus(), NumberLiteral("15"), Star(),
                     NumberLiteral("2.5")};

    auto res = grammar::factor(toks.begin(), toks.end());
    REQUIRE(res.has_value());
    auto [expr, it] = std::move(res.value());
    CHECK(it == toks.end());

    auto as_binary = dynamic_cast<Expr_Binary*>(expr.get());
    REQUIRE(as_binary != nullptr);
    REQUIRE(as_binary->op == EBinOp::Mul);
    auto left = dynamic_cast<Expr_Unary*>(as_binary->left.get());
    REQUIRE(left != nullptr);
    auto right = dynamic_cast<Expr_Literal*>(as_binary->right.get());
    REQUIRE(right != nullptr);

    REQUIRE(left->op == EUnaryOp::Minus);
    auto left_inner = dynamic_cast<Expr_Literal*>(left->inner.get());
    REQUIRE(left_inner != nullptr);
    REQUIRE(std::get<Expr_Literal::Number>(left_inner->inner).value == 15.0);
    REQUIRE(std::get<Expr_Literal::Number>(right->inner).value == 2.5);
}

TEST_CASE("term() parsing: 1 - 2", "[parser]") {
    TokenVec toks = {NumberLiteral("1"), Minus(), NumberLiteral("2")};

    auto res = grammar::term(toks.begin(), toks.end());
    REQUIRE(res.has_value());
    auto [expr, it] = std::move(res.value());
    CHECK(it == toks.end());

    auto as_binary = dynamic_cast<Expr_Binary*>(expr.get());
    REQUIRE(as_binary != nullptr);
    REQUIRE(as_binary->op == EBinOp::Minus);
    auto left = dynamic_cast<Expr_Literal*>(as_binary->left.get());
    REQUIRE(left != nullptr);
    auto right = dynamic_cast<Expr_Literal*>(as_binary->right.get());
    REQUIRE(right != nullptr);

    REQUIRE(std::get<Expr_Literal::Number>(left->inner).value == 1.0);
    REQUIRE(std::get<Expr_Literal::Number>(right->inner).value == 2.0);
}

TEST_CASE("comparison() parsing: 9 >= 15", "[parser]") {
    TokenVec toks = {NumberLiteral("9"), GreaterOrEq(), NumberLiteral("15")};

    auto res = grammar::comparison(toks.begin(), toks.end());
    REQUIRE(res.has_value());
    auto [expr, it] = std::move(res.value());
    CHECK(it == toks.end());

    auto as_binary = dynamic_cast<Expr_Binary*>(expr.get());
    REQUIRE(as_binary != nullptr);
    REQUIRE(as_binary->op == EBinOp::GreaterOrEq);
    auto left = dynamic_cast<Expr_Literal*>(as_binary->left.get());
    REQUIRE(left != nullptr);
    auto right = dynamic_cast<Expr_Literal*>(as_binary->right.get());
    REQUIRE(right != nullptr);

    REQUIRE(std::get<Expr_Literal::Number>(left->inner).value == 9);
    REQUIRE(std::get<Expr_Literal::Number>(right->inner).value == 15);
}

TEST_CASE("expression() parsing: 2 + 3", "[parser]") {
    TokenVec toks = {NumberLiteral("2"), Plus(), NumberLiteral("3")};

    auto res = grammar::expression(toks.begin(), toks.end());
    REQUIRE(res.has_value());
    auto [expr, it] = std::move(res.value());
    CHECK(it == toks.end());

    auto as_binary = dynamic_cast<Expr_Binary*>(expr.get());
    REQUIRE(as_binary != nullptr);
    REQUIRE(as_binary->op == EBinOp::Plus);
    auto left = dynamic_cast<Expr_Literal*>(as_binary->left.get());
    REQUIRE(left != nullptr);
    auto right = dynamic_cast<Expr_Literal*>(as_binary->right.get());
    REQUIRE(right != nullptr);

    REQUIRE(std::get<Expr_Literal::Number>(left->inner).value == 2);
    REQUIRE(std::get<Expr_Literal::Number>(right->inner).value == 3);
}