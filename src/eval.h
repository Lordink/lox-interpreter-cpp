#pragma once
/**
 * Evaluator for the Lox interpreter
 **/

#include "parser.h"
#include "runtime.h"
#include <expected>
#include <string>
#include <variant>

namespace eval {
using std::expected;
using std::monostate;
using std::string;

using rt::Value;

class Visitor_Eval : public Visitor<ValueResult> {
    virtual ValueResult visit_unary(Expr_Unary const& unary) const override;
    virtual ValueResult
    visit_literal(Expr_Literal const& literal) const override;
    virtual ValueResult visit_binary(Expr_Binary const& binary) const override;
    virtual ValueResult
    visit_grouping(Expr_Grouping const& grouping) const override;
};

template <typename T>
[[nodiscard]]
bool compare_values(Expr_Binary::EBinaryOperator op, Value const& left,
                    Value const& right) {
    switch (op) {
    case Expr_Binary::EBinaryOperator::EqEq:
        return std::get<T>(left) == std::get<T>(right);
    case Expr_Binary::EBinaryOperator::NotEq:
        return std::get<T>(left) != std::get<T>(right);
    default:
        std::unreachable();
    }
}

template <typename T>
[[nodiscard]]
bool both_values_are(Value const& left, Value const& right) {
    return std::holds_alternative<T>(left) && std::holds_alternative<T>(right);
}

std::expected<Value, string> evaluate(ExprPtr ast);
} // namespace eval
