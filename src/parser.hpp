#pragma once
/**
 * Parser for the Lox interpreter
 **/

#include <memory>
#include <print>
#include <string>
#include <variant>
#include <utility>

struct Expr_Grouping;
struct Expr_Literal;
struct Expr_Unary;
struct Expr_Binary;

class Visitor {
  public:
    virtual void visit_literal(Expr_Literal const& literal) const = 0;
    virtual void visit_grouping(Expr_Grouping const& grouping) const = 0;
    virtual void visit_unary(Expr_Unary const& unary) const = 0;
    virtual void visit_binary(Expr_Binary const& binary) const = 0;

    virtual ~Visitor() = default;
};

// Root expression type
struct Expr {
    // Accepting visitor pattern
    virtual void accept(Visitor const& visitor) const = 0;

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

    virtual void accept(Visitor const& visitor) const override {
        visitor.visit_literal(*this);
    }
};

// Expression in () parens
// (simplified language grammar, based on Chapt 5 of crafting-interpreters book)
struct Expr_Grouping : public Expr {
    ExprPtr inner;

    explicit Expr_Grouping(ExprPtr inner) : inner(std::move(inner)) {}

    virtual void accept(Visitor const& visitor) const override {
        visitor.visit_grouping(*this);
    }
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

    virtual void accept(Visitor const& visitor) const override {
        visitor.visit_unary(*this);
    }
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

    virtual void accept(Visitor const& visitor) const override {
        visitor.visit_binary(*this);
    }
};

namespace pprint {
class Visitor_PPrint : public Visitor {
  public:
    virtual void visit_unary(Expr_Unary const& unary) const override {
        const char op =
            unary.op == Expr_Unary::EUnaryOperator::Minus ? '-' : '!';
        std::print("{}", op);
        unary.inner->accept(*this);
    }

    virtual void visit_literal(Expr_Literal const& literal) const override {
        std::visit(
            [](auto&& var) {
                using T = std::decay_t<decltype(var)>;
                using std::is_same_v;
                if constexpr (is_same_v<T, Expr_Literal::Number>) {
                    std::print("{}", var.value);
                } else if constexpr (is_same_v<T, Expr_Literal::String>) {
                    std::print("{}", var.value);
                } else if constexpr (is_same_v<T, Expr_Literal::True>) {
                    std::print("true");
                } else if constexpr (is_same_v<T, Expr_Literal::False>) {
                    std::print("false");
                } else if constexpr (is_same_v<T, Expr_Literal::Nil>) {
                    std::print("nil");
                } else {
                    std::unreachable();
                }
            },
            literal.inner);
    }

    virtual void visit_binary(Expr_Binary const& binary) const override {
        binary.left->accept(*this);
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
        std::print(" {} ", op);
        binary.right->accept(*this);
    }
    virtual void visit_grouping(Expr_Grouping const& grouping) const override {
        std::print("( ");
        grouping.inner->accept(*this);
        std::print(" )");
    }
};
}; // namespace pprint

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

    // auto expr = make_unique<Expr_Unary>(Expr_Unary::EUnaryOperator::Minus,
    //     make_unique<Expr_Literal>(500.19));

    return expr;
}