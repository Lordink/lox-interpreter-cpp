#include "../src/lexer.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Lexing empty string", "[lexer]") {
    const std::string in = "";
    size_t num_errs = 0;

    const auto out = lex(in, num_errs);
    CHECK( num_errs == 0 );
    REQUIRE( out.size() == 1 );

    const auto& elem = out[0];
    REQUIRE(elem.has_value());

    const auto value = elem.value();
    REQUIRE(std::holds_alternative<EndOfFile>(value));

}

TEST_CASE("Lexing a string literal", "[lexer]") {
    const std::string in = "\"some string\"";
    size_t num_errs = 0;
    const auto out = lex(in, num_errs);
    CHECK( num_errs == 0 );
    REQUIRE( out.size() == 2 );

    const auto& elem_1 = out[0];
    const auto& elem_2 = out[1];
    REQUIRE(elem_1.has_value());
    REQUIRE(elem_2.has_value());

    const auto elem_1_val = elem_1.value();
    REQUIRE(std::holds_alternative<StringLiteral>(elem_1_val));
    const auto literal = std::get<StringLiteral>(elem_1_val);
    REQUIRE(literal.literal == "some string");

    REQUIRE(std::holds_alternative<EndOfFile>(elem_2.value()));
}

TEST_CASE("/() division and parens", "[lexer]") {
    const std::string in = "/()";
    size_t num_errs = 0;
    const auto out = lex(in, num_errs);

    CHECK( num_errs == 0 );
    REQUIRE( out.size() == 4 );

    REQUIRE(std::holds_alternative<Slash>(out[0].value()));
    REQUIRE(std::holds_alternative<LeftParen>(out[1].value()));
    REQUIRE(std::holds_alternative<RightParen>(out[2].value()));

    REQUIRE(std::holds_alternative<EndOfFile>(out[3].value()));
}

TEST_CASE("={===} assignment and figure brackets", "[lexer]") {
    const std::string in = "={===}";
    size_t num_errs = 0;
    const auto out = lex(in, num_errs);

    CHECK( num_errs == 0 );
    REQUIRE( out.size() == 6 );

    REQUIRE(std::holds_alternative<Assign>(out[0].value()));
    REQUIRE(std::holds_alternative<LeftBrace>(out[1].value()));
    REQUIRE(std::holds_alternative<Equals>(out[2].value()));
    REQUIRE(std::holds_alternative<Assign>(out[3].value()));
    REQUIRE(std::holds_alternative<RightBrace>(out[4].value()));

    REQUIRE(std::holds_alternative<EndOfFile>(out[5].value()));
}
