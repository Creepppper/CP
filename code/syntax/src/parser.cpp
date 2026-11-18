#include "parser.h"
#include <algorithm>
#include <sstream>

namespace{
const char* names[]={
    "","begin","end","integer","if","then","else","function","read","write",
    "identifier","number","=","<>","<=","<",">=",">","-","*",":=","(",")",";","EOLN","EOF"
};
const std::size_t noProcedure=static_cast<std::size_t>(-1);
bool statementStart(int kind){ return kind==Read || kind==Write || kind==Identifier || kind==If; }
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

const std::vector<std::string>& Parser::errors() const{ return messages; }

const std::vector<Procedure>& Parser::procedures() const{ return table; }

bool Parser::accept(int kind){
    if(tokens[pos].kind!=kind) return false;
    if(kind!=Eof) ++pos;
    return true;
}

bool Parser::expect(int kind){
    if(accept(kind)) return true;
    return fail(names[kind]);
}

bool Parser::fail(const std::string& expected){
    const Token& token=tokens[pos];
    messages.push_back("***LINE:"+std::to_string(token.line)+" Expected "+expected+", got '"+token.text+"'");
    return false;
}

bool Parser::validName(const Token& token){
    if(token.text.size()<=16) return true;
    messages.push_back("***LINE:"+std::to_string(token.line)+" Symbol table error: name '"+token.text+"' exceeds 16 characters");
    return false;
}

void Parser::sync(std::initializer_list<int> stops){
    while(tokens[pos].kind!=Eof && std::find(stops.begin(),stops.end(),tokens[pos].kind)==stops.end()) ++pos;
}

bool Parser::parse(){
    table.push_back({"<main>","-",0,-1,-1,{},false});
    block(0);
    if(!expect(Eof)) sync({Eof});
    int address=0;
    for(auto& procedure:table){
        if(!procedure.variables.empty()) procedure.first=address;
        for(auto& variable:procedure.variables) variable.address=address++;
        if(!procedure.variables.empty()) procedure.last=address-1;
    }
    return messages.empty();
}

void Parser::block(std::size_t procedure){
    if(!expect(Begin) && tokens[pos].kind==Eof) return;
    if(tokens[pos].kind!=Integer) fail("integer declaration");
    while(tokens[pos].kind==Integer){
        declaration(procedure);
        if(accept(Semicolon)) continue;
        fail(";");
        sync({Semicolon,Integer,Read,Write,Identifier,If,End,Else});
        accept(Semicolon);
    }
    statementList();
    expect(End);
}

void Parser::declaration(std::size_t procedure){
    accept(Integer);
    if(tokens[pos].kind==Identifier){
        const Token& name=tokens[pos++];
        if(validName(name) && procedure!=noProcedure){
            Procedure& current=table[procedure];
            if(!current.parameterDeclared && !current.variables.empty() && current.variables[0].kind==1 && current.variables[0].name==name.text) current.parameterDeclared=true;
            else current.variables.push_back({name.text,0,"integer",current.level,-1});
        }
        return;
    }
    if(!accept(Function)){
        fail("identifier or function");
        return;
    }
    const Token* name=nullptr;
    const Token* parameter=nullptr;
    if(tokens[pos].kind==Identifier) name=&tokens[pos++];
    else fail("identifier");
    bool nameValid=name && validName(*name);
    if(!expect(LeftParen) && tokens[pos].kind!=Identifier && tokens[pos].kind!=RightParen) return;
    if(tokens[pos].kind==Identifier) parameter=&tokens[pos++];
    else fail("identifier");
    bool parameterValid=parameter && validName(*parameter);
    if(!expect(RightParen) && tokens[pos].kind!=Semicolon && tokens[pos].kind!=Begin) return;
    if(!expect(Semicolon) && tokens[pos].kind!=Begin) return;
    std::size_t child=noProcedure;
    if(procedure!=noProcedure && nameValid && parameterValid){
        child=table.size();
        int level=table[procedure].level+1;
        table.push_back({name->text,"integer",level,-1,-1,{},false});
        table[child].variables.push_back({parameter->text,1,"integer",level,-1});
    }
    block(child);
}

void Parser::statementList(){
    bool needStatement=true;
    while(true){
        int kind=tokens[pos].kind;
        if(statementStart(kind)){
            if(!needStatement) fail(";");
            statement();
            needStatement=false;
        }else if(kind==Semicolon){
            if(needStatement) fail("statement");
            ++pos;
            needStatement=true;
        }else if(kind==End || kind==Else || kind==Eof){
            if(needStatement) fail("statement");
            return;
        }else{
            fail(needStatement?"statement":"; or end");
            sync({Semicolon,End,Else,Read,Write,Identifier,If});
        }
    }
}

bool Parser::statement(){
    if(accept(Read) || accept(Write)){
        bool left=expect(LeftParen);
        if(!left && tokens[pos].kind!=Identifier) return false;
        bool name=expect(Identifier);
        if(!name && tokens[pos].kind!=RightParen) return false;
        bool right=expect(RightParen);
        return left && name && right;
    }
    if(accept(Identifier)) return expect(Assign) && expression();
    if(accept(If)){
        bool cond=condition();
        if(!cond) sync({Then,Semicolon,Else,End});
        bool then=expect(Then);
        bool yes=statement();
        if(!yes) sync({Semicolon,Else,End});
        if(tokens[pos].kind==Semicolon && tokens[pos+1].kind==Else){
            fail("else");
            ++pos;
        }
        if(!expect(Else)) return false;
        bool no=statement();
        return cond && then && yes && no;
    }
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
