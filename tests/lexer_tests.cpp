#include <catch2/catch_test_macros.hpp>
#include "../src/lexer.hpp"

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