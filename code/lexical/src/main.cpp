#include "lexer.h"
#include <fstream>
#include <iostream>

int main(int argc,char* argv[]){
    if(argc!=2){
        std::cerr<<"Usage: mini_lexer <source file>\n";
        return 1;
    }
    std::ifstream input(argv[1]);
    if(!input){
        std::cerr<<"Cannot open: "<<argv[1]<<'\n';
        return 1;
    }
    LexResult result=scan(input);
    if(input.bad()){
        std::cerr<<"Cannot read: "<<argv[1]<<'\n';
        return 1;
    }
    for(const auto& token:result.tokens) std::cout<<token.text<<' '<<token.kind<<'\n';
    for(const auto& error:result.errors) std::cerr<<"***LINE:"<<error.line<<" Illegal character: "<<error.character<<'\n';
    return result.errors.empty()?0:1;
}
