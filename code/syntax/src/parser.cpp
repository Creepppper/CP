#include "parser.h"
#include <sstream>

namespace{
const char* names[]={
    "","begin","end","integer","if","then","else","function","read","write",
    "identifier","number","=","<>","<=","<",">=",">","-","*",":=","(",")",";","EOLN","EOF"
};
}

bool readTokens(std::istream& input,std::vector<Token>& tokens,std::string& error){
    std::string line,extra;
    int record=0,sourceLine=1,lastLine=1;
    bool ended=false,lineHasTokens=false;
    while(std::getline(input,line)){
        ++record;
        if(ended){
            error="Record after EOF at line "+std::to_string(record);
            return false;
        }
        std::istringstream row(line);
        Token token;
        if(!(row>>token.text>>token.kind) || (row>>extra) || token.kind<Begin || token.kind>Eof){
            error="Invalid record at line "+std::to_string(record);
            return false;
        }
        if(token.kind!=Identifier && token.kind!=Number && token.text!=names[token.kind]){
            error="Token and kind do not match at line "+std::to_string(record);
            return false;
        }
        if(token.kind==Eoln){
            lastLine=sourceLine++;
            lineHasTokens=false;
        }else if(token.kind==Eof){
            if(lineHasTokens){
                error="Missing EOLN before EOF at line "+std::to_string(record);
                return false;
            }
            token.line=lastLine;
            tokens.push_back(token);
            ended=true;
        }else{
            token.line=sourceLine;
            tokens.push_back(token);
            lineHasTokens=true;
        }
    }
    if(input.bad()){
        error="Cannot read input file";
        return false;
    }
    if(!ended){
        error="Missing EOF";
        return false;
    }
    return true;
}

Parser::Parser(const std::vector<Token>& tokens):tokens(tokens){}

const std::string& Parser::error() const{ return message; }

bool Parser::accept(int kind){
    if(tokens[pos].kind!=kind) return false;
    ++pos;
    return true;
}

bool Parser::expect(int kind){
    if(accept(kind)) return true;
    return fail(names[kind]);
}

bool Parser::fail(const std::string& expected){
    const Token& token=tokens[pos];
    message="***LINE:"+std::to_string(token.line)+" Expected "+expected+", got '"+token.text+"'";
    return false;
}

bool Parser::parse(){ return block() && expect(Eof); }

bool Parser::block(){
    if(!expect(Begin) || !declaration() || !expect(Semicolon)) return false;
    while(tokens[pos].kind==Integer){
        if(!declaration() || !expect(Semicolon)) return false;
    }
    return statementList() && expect(End);
}

bool Parser::declaration(){
    if(!expect(Integer)) return false;
    if(accept(Identifier)) return true;
    if(!accept(Function)) return fail("identifier or function");
    return expect(Identifier) && expect(LeftParen) && expect(Identifier) && expect(RightParen)
        && expect(Semicolon) && block();
}

bool Parser::statementList(){
    if(!statement()) return false;
    while(accept(Semicolon)){
        if(!statement()) return false;
    }
    return true;
}

bool Parser::statement(){
    if(accept(Read) || accept(Write)) return expect(LeftParen) && expect(Identifier) && expect(RightParen);
    if(accept(Identifier)) return expect(Assign) && expression();
    if(accept(If)) return condition() && expect(Then) && statement() && expect(Else) && statement();
    return fail("statement");
}

bool Parser::condition(){
    if(!expression()) return false;
    if(tokens[pos].kind<Equal || tokens[pos].kind>Greater) return fail("relational operator");
    ++pos;
    return expression();
}

bool Parser::expression(){
    if(!term()) return false;
    while(accept(Minus)){
        if(!term()) return false;
    }
    return true;
}

bool Parser::term(){
    if(!factor()) return false;
    while(accept(Star)){
        if(!factor()) return false;
    }
    return true;
}

bool Parser::factor(){
    if(accept(Number)) return true;
    if(!accept(Identifier)) return fail("identifier or number");
    if(accept(LeftParen)) return expression() && expect(RightParen);
    return true;
}
