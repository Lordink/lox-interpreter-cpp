#include "eval.h"
#include <expected>
#include <stdexcept>
#include <variant>

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
    throw std::logic_error("Unimplemented");
    return std::monostate{};
}
Value Visitor_Eval::visit_binary(Expr_Binary const& binary) const {

    throw std::logic_error("Unimplemented");
    return std::monostate{};
}
Value Visitor_Eval::visit_grouping(Expr_Grouping const& grouping) const {

    throw std::logic_error("Unimplemented");
    return std::monostate{};
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
