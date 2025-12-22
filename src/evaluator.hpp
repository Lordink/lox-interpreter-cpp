#pragma once
/**
 * Evaluator for the Lox interpreter
 **/

#include "parser.hpp"
#include <string>

namespace eval {
using std::monostate;
using std::string;

using Value = std::variant<monostate, bool, double, string>;

class Visitor_Eval : public Visitor {
    virtual void visit_unary(Expr_Unary const& unary) const override;
    virtual void visit_literal(Expr_Literal const& literal) const override;
    virtual void visit_binary(Expr_Binary const& binary) const override;
    virtual void visit_grouping(Expr_Grouping const& grouping) const override;
};
} // namespace eval