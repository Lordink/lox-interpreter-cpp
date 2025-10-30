#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <format>

using std::string;
using std::cout;
using std::cerr;
using std::cin;
using std::endl;
using std::format;

string read_file_contents(const string& filename);

int main(const int argc, char *argv[]) {
    // Disable output buffering
    cout << std::unitbuf;
    cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    cerr << "Logs from your program will appear here!" << endl;

    if (argc < 3) {
        cerr << "Usage: ./your_program tokenize <filename>" << endl;
        return 1;
    }

    const string command = argv[1];

    if (command == "tokenize") {
        string file_contents = read_file_contents(argv[2]);
        
        if (!file_contents.empty()) {
            cerr << "Scanner not implemented" << endl;
            return 1;
        }
        // TODO Placeholder, replace this line when implementing the scanner
        cout << "EOF  null" << std::endl;

    } else {
        cerr << "Unknown command: " << command << endl;
        return 1;
    }

    return 0;
}

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
