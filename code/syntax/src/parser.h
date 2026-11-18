#pragma once

#include <cstddef>
#include <istream>
#include <string>
#include <vector>

enum Kind{
    Begin=1,End,Integer,If,Then,Else,Function,Read,Write,Identifier,Number,
    Equal,NotEqual,LessEqual,Less,GreaterEqual,Greater,Minus,Star,Assign,
    LeftParen,RightParen,Semicolon,Eoln,Eof
};

struct Token{
    std::string text;
    int kind;
    int line;
};

bool readTokens(std::istream& input,std::vector<Token>& tokens,std::string& error);

class Parser{
public:
    explicit Parser(const std::vector<Token>& tokens);
    bool parse();
    const std::string& error() const;

private:
    const std::vector<Token>& tokens;
    std::size_t pos=0;
    std::string message;

    bool accept(int kind);
    bool expect(int kind);
    bool fail(const std::string& expected);
    bool block();
    bool declaration();
    bool statementList();
    bool statement();
    bool condition();
    bool expression();
    bool term();
    bool factor();
};
