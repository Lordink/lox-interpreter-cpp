#include "eval.h"
#include <expected>
#include <stdexcept>
#include <variant>

using std::holds_alternative;

namespace eval {
Value Visitor_Eval::visit_literal(Expr_Literal const& literal) const {
    return std::visit(
        [this](auto&& var) -> Value {
            using T = std::decay_t<decltype(var)>;
            using std::is_same_v;

            if constexpr (is_same_v<T, Expr_Literal::Number>) {
                return var.value;
            } else if constexpr (is_same_v<T, Expr_Literal::String>) {
                return var.value;
            } else if constexpr (is_same_v<T, Expr_Literal::True>) {
                return true;
            } else if constexpr (is_same_v<T, Expr_Literal::False>) {
                return false;
            } else if constexpr (is_same_v<T, Expr_Literal::Nil>) {
                return std::monostate{};
            } else {
                std::unreachable();
                return std::monostate{};
            }
        },
        literal.inner);
}

Value Visitor_Eval::visit_unary(Expr_Unary const& unary) const {
    const Value inner_val = unary.inner->accept(*this);
    switch (unary.op) {
    case Expr_Unary::EUnaryOperator::Minus:
        // Should never fail
        return -1.0 * std::get<double>(inner_val);
    case Expr_Unary::EUnaryOperator::Bang:
        if (holds_alternative<bool>(inner_val)) {
            return !std::get<bool>(inner_val);
        } else if (holds_alternative<std::monostate>(inner_val)) {
            return true;
        } else if (holds_alternative<double>(inner_val)) {
            // Negating any number is false-ey, incl 0
            return false;
        } else {
            throw std::runtime_error("Unexpected unary type");
        }
    }
}
Value Visitor_Eval::visit_binary(Expr_Binary const& binary) const {

    throw std::logic_error("Unimplemented");
    return std::monostate{};
}
Value Visitor_Eval::visit_grouping(Expr_Grouping const& grouping) const {
    return grouping.inner->accept(*this);
}

std::expected<Value, string> evaluate(ExprPtr ast) {
    if (ast == nullptr) {
        return std::unexpected("Input AST is nil");
    }
    Visitor_Eval eval_visitor;
    // TODO this assumes no failures are possible inside evaluation code
    const auto value = ast->accept(eval_visitor);

    return value;
}

} // namespace eval
