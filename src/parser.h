#pragma once
/**
 * Parser for the Lox interpreter
 **/

#include <expected>
#include <memory>
#include <print>
#include <string>
#include <utility>
#include <variant>

#include "lexer.h"
#include "runtime.h"

struct Expr_Grouping;
struct Expr_Literal;
struct Expr_Unary;
struct Expr_Binary;

template<typename RetVal>
class Visitor {
  public:
    virtual RetVal visit_literal(Expr_Literal const& literal) const = 0;
    virtual RetVal visit_grouping(Expr_Grouping const& grouping) const = 0;
    virtual RetVal visit_unary(Expr_Unary const& unary) const = 0;
    virtual RetVal visit_binary(Expr_Binary const& binary) const = 0;

    virtual ~Visitor() = default;
};

// Root expression type
struct Expr {
    // Visitor that doesn't return anything
    virtual void accept(Visitor<void> const& visitor) const = 0;
    // Visitor returning runtime value
    virtual rt::Value accept(Visitor<rt::Value> const& visitor) const = 0;

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

    virtual void accept(Visitor<void> const& visitor) const override {
        visitor.visit_literal(*this);
    }
    virtual rt::Value accept(Visitor<rt::Value> const& visitor) const override {
        return visitor.visit_literal(*this);
    }
};

// Expression in () parens
// (simplified language grammar, based on Chapt 5 of crafting-interpreters book)
struct Expr_Grouping : public Expr {
    ExprPtr inner;

    explicit Expr_Grouping(ExprPtr inner) : inner(std::move(inner)) {}

    virtual void accept(Visitor<void> const& visitor) const override {
        visitor.visit_grouping(*this);
    }
    virtual rt::Value accept(Visitor<rt::Value> const& visitor) const override {
        return visitor.visit_grouping(*this);
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

    virtual void accept(Visitor<void> const& visitor) const override {
        visitor.visit_unary(*this);
    }
    virtual rt::Value accept(Visitor<rt::Value> const& visitor) const override {
        return visitor.visit_unary(*this);
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

    virtual void accept(Visitor<void> const& visitor) const override {
        visitor.visit_binary(*this);
    }
    virtual rt::Value accept(Visitor<rt::Value> const& visitor) const override {
        return visitor.visit_binary(*this);
    }
};

namespace pprint {
using std::print;

class Visitor_PPrint : public Visitor<void> {
  public:
    virtual void visit_unary(Expr_Unary const& unary) const override;
    virtual void visit_literal(Expr_Literal const& literal) const override;
    virtual void visit_binary(Expr_Binary const& binary) const override;
    virtual void visit_grouping(Expr_Grouping const& grouping) const override;
};
}; // namespace pprint

namespace grammar {
using std::expected;
using std::pair;
using std::string;

using TokenIter = TokenVec::const_iterator;
using ParseResult = expected<pair<ExprPtr, TokenIter>, string>;

template <Token T> bool tok_matches(TokenVec::const_iterator it) {
    return std::holds_alternative<T>(*it);
}

template <Token... Ts>
bool tok_matches_any(impl::TokenList<Ts...> tokens,
                        TokenVec::const_iterator& it) {
    return (tok_matches<Ts>(it) || ...);
}

// "Decorate" a function with an iterator bounds check
// If all g, pass thru the iterators
template<typename F>
ParseResult bounds_check(F&& fn, TokenIter const& start_it, TokenIter const& end_it) {
    if (start_it >= end_it) {
        return std::unexpected("Reached end iterator");
    }

    return fn(start_it, end_it);
}

ParseResult expression(TokenIter const& start_it, TokenIter const& end_it);
ParseResult equality(TokenIter const& start_it, TokenIter const& end_it);
ParseResult comparison(TokenIter const& start_it, TokenIter const& end_it);
ParseResult term(TokenIter const& start_it, TokenIter const& end_it);

ParseResult factor(TokenIter const& start_it, TokenIter const& end_it);
ParseResult unary(TokenIter const& start_it, TokenIter const& end_it);
ParseResult primary(TokenIter const& start_it, TokenIter const& end_it);
} // namespace grammar

// Parse a single expression
[[nodiscard]]
std::expected<ExprPtr, std::string> parse(TokenVec const& tokens);