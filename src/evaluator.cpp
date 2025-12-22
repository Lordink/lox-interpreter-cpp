#include "evaluator.h"

namespace eval {
Value Visitor_Eval::visit_literal(Expr_Literal const& literal) const {
    std::visit(
        [this](auto&& var) {
            using T = std::decay_t<decltype(var)>;
            using std::is_same_v;
            if constexpr (is_same_v<T, Expr_Literal::Number>) {
            } else if constexpr (is_same_v<T, Expr_Literal::String>) {
            } else if constexpr (is_same_v<T, Expr_Literal::True>) {
            } else if constexpr (is_same_v<T, Expr_Literal::False>) {
            } else if constexpr (is_same_v<T, Expr_Literal::Nil>) {
            } else {
                std::unreachable();
            }
        },
        literal.inner);

    return monostate();
}
}