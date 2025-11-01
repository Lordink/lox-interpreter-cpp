#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <format>
#include <vector>

#include "lexer.hpp"

using std::string;
using std::cout;
using std::cerr;
using std::cin;
using std::endl;
using std::format;
using std::vector;

string read_file_contents(const string& filename);

int main(const int argc, char *argv[]) {
    // Disable output buffering
    cout << std::unitbuf;
    cerr << std::unitbuf;

    if (argc < 3) {
        cerr << "Usage: ./your_program tokenize <filename>" << endl;
        return 1;
    }

    const string command = argv[1];

    if (command == "tokenize") {
        string file_contents = read_file_contents(argv[2]);

        const auto tokens = lex(file_contents);
        for (const auto& tok : tokens) {
            cout << format("{}\n", tok);
        }

    } else {
        cerr << "Unknown command: " << command << endl;
        return 1;
    }

    return 0;
}

[[nodiscard]]
string read_file_contents(const string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        cerr << format("Error reading file: {}\n", filename);
        std::exit(1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
}