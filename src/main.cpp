#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <print>

#include "lexer.hpp"

using std::string;
using std::cin;
using std::println;

string read_file_contents(const string& filename);

constexpr int LEXICAL_ERR_RETURN_CODE = 65;

int main(const int argc, char *argv[]) {
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 3) {
        println(stderr, "Usage: ./your_program tokenize <filename>");
        return 1;
    }

    const string command = argv[1];

    if (command == "tokenize") {
        string file_contents = read_file_contents(argv[2]);
        size_t num_errors = 0;

        const auto tokens = lex(file_contents, num_errors);
        for (const auto& exp_tok : tokens) {
            if (exp_tok.has_value()) {
                print_token_variant(*exp_tok);
            } else {
                println(stderr, "{}", exp_tok.error());
            }
        }

        if (num_errors > 0) {
            return LEXICAL_ERR_RETURN_CODE;
        }
    } else if (command == "parse") {
        // TODO extract tokenize (above) into its own fn, so we can run it before parse()

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