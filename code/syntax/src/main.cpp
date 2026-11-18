#include "parser.h"
#include <fstream>
#include <iostream>

int main(int argc,char* argv[]){
    if(argc!=2){
        std::cerr<<"Usage: mini_parser <token file>\n";
        return 1;
    }
    std::ifstream input(argv[1]);
    if(!input){
        std::cerr<<"Cannot open: "<<argv[1]<<'\n';
        return 1;
    }
    std::vector<Token> tokens;
    std::string error;
    if(!readTokens(input,tokens,error)){
        std::cerr<<"Input error: "<<error<<'\n';
        return 1;
    }
    Parser parser(tokens);
    if(!parser.parse()){
        std::cerr<<parser.error()<<'\n';
        return 1;
    }
    std::cout<<"Syntax analysis succeeded.\n";
    return 0;
}
