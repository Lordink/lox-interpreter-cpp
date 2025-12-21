#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <print>

#include "lexer.hpp"
#include "parser.hpp"

using std::string;
using std::cin;
using std::println;

string read_file_contents(const string& filename);

constexpr int ERR_RETURN_CODE = 65;


int main(const int argc, char *argv[]) {
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;


    const string command = argv[1];

    if (command == "tokenize" || command == "parse") {
        if (argc < 3) {
            println(stderr, "Usage: ./your_program tokenize <filename>");
            return 1;
        }
        string file_contents = read_file_contents(argv[2]);
        size_t num_errors = 0;

        const auto tokens = lex(file_contents, num_errors);
        const bool is_tokenizing = command == "tokenize";
        for (const auto& exp_tok : tokens) {
            if (exp_tok.has_value()) {
                if (is_tokenizing) {
                    print_token_variant(*exp_tok);
                }
            } else {
                println(stderr, "{}", exp_tok.error());
            }
        }

        if (num_errors > 0) {
            return ERR_RETURN_CODE;
        }

        if (command == "parse") {
            auto opt_token_vec = lift(tokens);
            assert(opt_token_vec.has_value());

            auto opt_parsed = parse(opt_token_vec.value());
            if (!opt_parsed.has_value()) {
                // Line 1 hardcoded, as we parse a single expression for now
                println(stderr, "[line 1] Error at '{}': Expect expression.", opt_parsed.error());
                return ERR_RETURN_CODE;
            }
            auto parsed = std::move(opt_parsed.value());

            pprint::Visitor_PPrint pprinter;
            parsed->accept(pprinter);
            // Just newline
            println("");
        }

    } else {
        println(stderr, "Unknown command: {}", command);
        return 1;
    }

    return 0;
}

[[nodiscard]]
string read_file_contents(const string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        println(stderr, "Error reading file: {}", filename);
        std::exit(1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
}