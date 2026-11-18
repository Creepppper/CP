#pragma once

#include <istream>
#include <string>
#include <vector>

struct Token{
    std::string text;
    int kind;
    int line;
};

struct LexError{
    char character;
    int line;
};

struct LexResult{
    std::vector<Token> tokens;
    std::vector<LexError> errors;
};

LexResult scan(std::istream& input);
