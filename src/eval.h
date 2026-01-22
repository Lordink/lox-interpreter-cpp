#pragma once
/**
 * Evaluator for the Lox interpreter
 **/

#include "parser.h"
#include "runtime.h"
#include <expected>
#include <string>

namespace eval {
using std::monostate;
using std::string;

using rt::Value;

class Visitor_Eval : public Visitor<Value> {
    virtual Value visit_unary(Expr_Unary const& unary) const override;
    virtual Value visit_literal(Expr_Literal const& literal) const override;
    virtual Value visit_binary(Expr_Binary const& binary) const override;
    virtual Value visit_grouping(Expr_Grouping const& grouping) const override;
};

[[nodiscard]]
std::expected<Value, string> evaluate(ExprPtr ast);
} // namespace eval
