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

const std::vector<Procedure>& Parser::procedures() const{ return table; }

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

bool Parser::validName(const Token& token){
    if(token.text.size()<=16) return true;
    message="***LINE:"+std::to_string(token.line)+" Symbol table error: name '"+token.text+"' exceeds 16 characters";
    return false;
}

bool Parser::parse(){
    table.push_back({"<main>","-",0,-1,-1,{},false});
    if(!block(0) || !expect(Eof)) return false;
    int address=0;
    for(auto& procedure:table){
        if(!procedure.variables.empty()) procedure.first=address;
        for(auto& variable:procedure.variables) variable.address=address++;
        if(!procedure.variables.empty()) procedure.last=address-1;
    }
    return true;
}

bool Parser::block(std::size_t procedure){
    if(!expect(Begin) || !declaration(procedure) || !expect(Semicolon)) return false;
    while(tokens[pos].kind==Integer){
        if(!declaration(procedure) || !expect(Semicolon)) return false;
    }
    return statementList() && expect(End);
}

bool Parser::declaration(std::size_t procedure){
    if(!expect(Integer)) return false;
    if(tokens[pos].kind==Identifier){
        const Token& name=tokens[pos++];
        if(!validName(name)) return false;
        Procedure& current=table[procedure];
        if(!current.parameterDeclared && !current.variables.empty() && current.variables[0].kind==1 && current.variables[0].name==name.text) current.parameterDeclared=true;
        else current.variables.push_back({name.text,0,"integer",current.level,-1});
        return true;
    }
    if(!accept(Function)) return fail("identifier or function");
    const Token& name=tokens[pos];
    if(!expect(Identifier) || !validName(name) || !expect(LeftParen)) return false;
    const Token& parameter=tokens[pos];
    if(!expect(Identifier) || !validName(parameter) || !expect(RightParen) || !expect(Semicolon)) return false;
    std::size_t child=table.size();
    int level=table[procedure].level+1;
    table.push_back({name.text,"integer",level,-1,-1,{},false});
    table[child].variables.push_back({parameter.text,1,"integer",level,-1});
    return block(child);
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
