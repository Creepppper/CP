#pragma once

#include <cstddef>
#include <initializer_list>
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

struct Variable{
    std::string name;
    int kind;
    std::string type;
    int level;
    int address;
};

struct Procedure{
    std::string name;
    std::string type;
    int level;
    int first;
    int last;
    std::vector<Variable> variables;
    bool parameterDeclared;
};

bool readTokens(std::istream& input,std::vector<Token>& tokens,std::string& error);

class Parser{
public:
    explicit Parser(const std::vector<Token>& tokens);
    bool parse();
    const std::vector<std::string>& errors() const;
    const std::vector<Procedure>& procedures() const;

private:
    const std::vector<Token>& tokens;
    std::size_t pos=0;
    std::vector<std::string> messages;
    std::vector<Procedure> table;

    bool accept(int kind);
    bool expect(int kind);
    bool fail(const std::string& expected);
    bool validName(const Token& token);
    void sync(std::initializer_list<int> stops);
    void block(std::size_t procedure);
    void declaration(std::size_t procedure);
    void statementList();
    bool statement();
    bool condition();
    bool expression();
    bool term();
    bool factor();
};
