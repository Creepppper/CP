#include "lexer.h"
#include <filesystem>
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
    namespace fs=std::filesystem;
    fs::path dir="output", name=fs::path(argv[1]).stem();
    fs::path dyd=dir/(name.string()+".dyd"), err=dir/(name.string()+".err");
    std::error_code ec;
    fs::create_directories(dir,ec);
    if(ec){
        std::cerr<<"Cannot create output directory: "<<ec.message()<<'\n';
        return 1;
    }
    if(fs::equivalent(argv[1],dyd,ec) || fs::equivalent(argv[1],err,ec)){
        std::cerr<<"Output file is the source file\n";
        return 1;
    }
    std::ofstream tokens(dyd), errors(err);
    if(!tokens || !errors){
        std::cerr<<"Cannot open output files\n";
        return 1;
    }
    for(const auto& token:result.tokens) tokens<<token.text<<' '<<token.kind<<'\n';
    for(const auto& error:result.errors) errors<<"***LINE:"<<error.line<<" Illegal character: "<<error.character<<'\n';
    tokens.close();
    errors.close();
    if(!tokens || !errors){
        std::cerr<<"Cannot write output files\n";
        return 1;
    }
    std::cout<<"Tokens: "<<dyd.string()<<"\nErrors: "<<err.string()<<'\n';
    return result.errors.empty()?0:1;
}
