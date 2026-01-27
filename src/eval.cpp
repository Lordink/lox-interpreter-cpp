#include "eval.h"
#include "parser.h"
#include <expected>
#include <stdexcept>
#include <utility>
#include <variant>

using std::holds_alternative;

namespace eval {
ValueResult Visitor_Eval::visit_literal(Expr_Literal const& literal) const {
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

ValueResult Visitor_Eval::visit_unary(Expr_Unary const& unary) const {
    const ValueResult res_inner_val = unary.inner->accept(*this);
    if (!res_inner_val) {
        return res_inner_val.error();
    }

    const Value inner_val = res_inner_val.value();
    switch (unary.op) {
    case Expr_Unary::EUnaryOperator::Minus:
        // Should never fail
        if (holds_alternative<double>(inner_val)) {
            return -1.0 * std::get<double>(inner_val);
        } else {
            return std::unexpected("Operand must be a number");
        }
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

double binary_value(Value const& val) {
    if (holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? 1.0 : 0.0;
    } else if (holds_alternative<std::monostate>(val)) {
        return 0.0;
    } else {
        return std::get<double>(val);
    }
}

ValueResult Visitor_Eval::visit_binary(Expr_Binary const& binary) const {
    enum class EOperationKind { Arithmetic, StrConcat, Cmp, Relation };
    EOperationKind op_kind;

    const ValueResult res_left_v = binary.left->accept(*this);
    if (!res_left_v) {
        return res_left_v.error();
    }
    const Value left_v = res_left_v.value();
    const ValueResult res_right_v = binary.right->accept(*this);
    if (!res_right_v) {
        return res_right_v.error();
    }
    const Value right_v = res_right_v.value();

    switch (binary.op) {
    case Expr_Binary::EBinaryOperator::Plus:
        // Both are strings
        if (both_values_are<string>(left_v, right_v)) {
            op_kind = EOperationKind::StrConcat;
            break;
        } else if (both_values_are<double>(left_v, right_v)) {
            op_kind = EOperationKind::Arithmetic;
            break;
        } else {
            return std::unexpected(
                "Operands must be two numbers or two strings");
        }
    case Expr_Binary::EBinaryOperator::Minus:
        if (both_values_are<double>(left_v, right_v)) {
            op_kind = EOperationKind::Arithmetic;
            break;
        } else {
            return std::unexpected("Operands must be numbers");
        }
    case Expr_Binary::EBinaryOperator::Mul:
    case Expr_Binary::EBinaryOperator::Div:
        op_kind = EOperationKind::Arithmetic;
        break;
    case Expr_Binary::EBinaryOperator::EqEq:
    case Expr_Binary::EBinaryOperator::NotEq:
        op_kind = EOperationKind::Cmp;
        break;
    default:
        op_kind = EOperationKind::Relation;
    }

    if (op_kind == EOperationKind::Arithmetic) {
        if (!holds_alternative<double>(left_v) ||
            !holds_alternative<double>(right_v)) {
            return std::unexpected("Operands must be numbers.");
        }
        const double left = std::get<double>(left_v);
        const double right = std::get<double>(right_v);

        switch (binary.op) {
        case Expr_Binary::EBinaryOperator::Plus:
            return left + right;
        case Expr_Binary::EBinaryOperator::Minus:
            return left - right;
        case Expr_Binary::EBinaryOperator::Mul:
            return left * right;
        case Expr_Binary::EBinaryOperator::Div:
            return left / right;
        default:
            std::unreachable();
        }
    } else if (op_kind == EOperationKind::StrConcat) {
        const string left = std::get<string>(left_v);
        const string right = std::get<string>(right_v);

        return left + right;
    } else if (op_kind == EOperationKind::Cmp) {
        // They don't hold the same variant type
        if (left_v.index() != right_v.index()) {
            // Diff variants are always NOT equal
            switch (binary.op) {
            case Expr_Binary::EBinaryOperator::EqEq:
                return false;
            case Expr_Binary::EBinaryOperator::NotEq:
                return true;
            default:
                std::unreachable();
            }
        } else {
            // They hold the same variant type
            if (holds_alternative<std::monostate>(left_v)) {
                // monostates are always equal
                switch (binary.op) {
                case Expr_Binary::EBinaryOperator::EqEq:
                    return true;
                case Expr_Binary::EBinaryOperator::NotEq:
                    return false;
                default:
                    std::unreachable();
                }
            } else if (holds_alternative<bool>(left_v)) {
                return compare_values<bool>(binary.op, left_v, right_v);
            } else if (holds_alternative<double>(left_v)) {
                return compare_values<double>(binary.op, left_v, right_v);
            } else if (holds_alternative<string>(left_v)) {
                return compare_values<string>(binary.op, left_v, right_v);
            }
        }

    } else if (op_kind == EOperationKind::Relation) {
        const double left = binary_value(left_v);
        const double right = binary_value(right_v);

        switch (binary.op) {
        case Expr_Binary::EBinaryOperator::EqEq:
            return left == right;
        case Expr_Binary::EBinaryOperator::NotEq:
            return left != right;
        case Expr_Binary::EBinaryOperator::Less:
            return left < right;
        case Expr_Binary::EBinaryOperator::LessOrEq:
            return left <= right;
        case Expr_Binary::EBinaryOperator::Greater:
            return left > right;
        case Expr_Binary::EBinaryOperator::GreaterOrEq:
            return left >= right;
        default:
            std::unreachable();
            break;
        }
    }

    throw std::runtime_error("Unexpected control path");
}
ValueResult Visitor_Eval::visit_grouping(Expr_Grouping const& grouping) const {
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
