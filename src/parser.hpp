#pragma once
/**
 * Parser for the Lox interpreter
 **/

#include <memory>
#include <string>
#include <variant>

// Root expression type
struct Expr {
    virtual ~Expr() = default;
};

using ExprPtr = std::unique_ptr<Expr>;

struct Expr_Literal : public Expr {
    struct Number {
        double value = 0.0;
        explicit Number(const double value) : value(value) {}
    };
    struct String {
        std::string value;
        explicit String(std::string value) : value(std::move(value)) {}
    };
    struct True {};
    struct False {};
    struct Nil {};

    using LiteralVariant = std::variant<Number, String, True, False, Nil>;
    LiteralVariant inner;

    explicit Expr_Literal(LiteralVariant inner) : inner(std::move(inner)) {}
    explicit Expr_Literal(const double num) : inner(Number(num)) {}
    explicit Expr_Literal(std::string s) : inner(String(std::move(s))) {}
};

// Expression in () parens
// (simplified language grammar, based on Chapt 5 of crafting-interpreters book)
struct Expr_Grouping : public Expr {
    ExprPtr inner;

    explicit Expr_Grouping(ExprPtr inner) : inner(std::move(inner)) {}
};

struct Expr_Unary : public Expr {
    enum class EUnaryOperator {
        // -
        Minus,
        // !
        Bang
    };

    EUnaryOperator op;
    ExprPtr inner;

    explicit Expr_Unary(const EUnaryOperator op, ExprPtr inner)
        : op(op), inner(std::move(inner)) {}
};

struct Expr_Binary : public Expr {
    enum class EBinaryOperator {
        // ==
        EqEq,
        // !=
        NotEq,
        // <
        Less,
        // <=
        LessOrEq,
        // >
        Greater,
        // >=
        GreaterOrEq,
        // +
        Plus,
        // -
        Minus,
        // *
        Mul,
        // /
        Div
    };

    ExprPtr left;
    EBinaryOperator op;
    ExprPtr right;

    explicit Expr_Binary(ExprPtr left, const EBinaryOperator op, ExprPtr right)
        : left(std::move(left)), op(op), right(std::move(right)) {}
};

// Just create a mocked expression as if we already parsed something
// Useful to e.g. experiment with pretty-printing
[[nodiscard]]
inline ExprPtr mock_parsed() {
    using std::make_unique;

    auto expr = make_unique<Expr_Binary>(
        make_unique<Expr_Unary>(Expr_Unary::EUnaryOperator::Minus,
                                make_unique<Expr_Literal>(123.0)),
        Expr_Binary::EBinaryOperator::Mul,
        make_unique<Expr_Grouping>(make_unique<Expr_Literal>(45.67)));

    return expr;
}