#include <fstream>
#include <iostream>
#include <print>
#include <sstream>
#include <string>

#include "eval.h"
#include "lexer.h"
#include "parser.h"
#include "runtime.h"

using std::println;
using std::string;

string read_file_contents(const string& filename);

constexpr int INTERP_ERR_RETURN_CODE = 65;
constexpr int RUNTIME_ERR_RETURN_CODE = 70;

int main(const int argc, char* argv[]) {
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    const string command = argv[1];

    if (command == "tokenize" || command == "parse" || command == "evaluate") {
        if (argc < 3) {
            println(stderr, "Usage: ./your_program {} <filename>", command);
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
            return INTERP_ERR_RETURN_CODE;
        } else if (command == "tokenize") {
            // We are done
            return 0;
        }

        // Parsing
        auto opt_token_vec = lift(tokens);
        assert(opt_token_vec.has_value());

        auto opt_parsed = parse(opt_token_vec.value());
        if (!opt_parsed.has_value()) {
            // Line 1 hardcoded, as we parse a single expression for now
            println(stderr, "[line 1] Error at '{}': Expect expression.",
                    opt_parsed.error());
            return INTERP_ERR_RETURN_CODE;
        }
        auto parsed = std::move(opt_parsed.value());

        if (command == "parse") {
            pprint::Visitor_PPrint pprinter;
            parsed->accept(pprinter);
            // Just newline
            println("");
            return 0;
        }

        // Eval
        if (command == "evaluate") {
            auto value = eval::evaluate(std::move(parsed));
            if (value.has_value()) {
                // TODO print value
                rt::print_value(value.value());
                return 0;
            } else {
                println(stderr, "{}\n[line 1]", value.error());
                return RUNTIME_ERR_RETURN_CODE;
            }
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
