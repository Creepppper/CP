#include "parser.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs=std::filesystem;

bool resolve(const fs::path& path,fs::path& result){
    std::error_code ec;
    result=fs::weakly_canonical(path,ec);
    if(!ec) return true;
    std::cerr<<"Cannot resolve path: "<<path.string()<<'\n';
    return false;
}

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
    bool ok=parser.parse();
    fs::path source=argv[1], dir="output", stem=source.stem();
    std::array<fs::path,3> paths={dir/(stem.string()+".var"),dir/(stem.string()+".pro"),dir/(stem.string()+".err")};
    std::error_code ec;
    fs::create_directories(dir,ec);
    if(ec){
        std::cerr<<"Cannot create output directory: "<<ec.message()<<'\n';
        return 1;
    }
    fs::path inputPath,lexicalError;
    std::array<fs::path,3> resolved;
    if(!resolve(source,inputPath) || !resolve(source.parent_path()/(stem.string()+".err"),lexicalError)) return 1;
    for(std::size_t i=0;i<paths.size();++i){
        if(!resolve(paths[i],resolved[i])) return 1;
        if(resolved[i]==inputPath || resolved[i]==lexicalError){
            std::cerr<<"Output would overwrite the input or its error file; run from the syntax directory.\n";
            return 1;
        }
        for(std::size_t j=0;j<i;++j){
            if(resolved[i]==resolved[j]){
                std::cerr<<"Output files resolve to the same path.\n";
                return 1;
            }
        }
    }
    std::ofstream variables(paths[0]),procedures(paths[1]),errors(paths[2]);
    if(!variables || !procedures || !errors){
        std::cerr<<"Cannot open output files.\n";
        return 1;
    }
    for(const auto& p:parser.procedures()){
        for(const auto& v:p.variables) variables<<v.name<<' '<<p.name<<' '<<v.kind<<' '<<v.type<<' '<<v.level<<' '<<v.address<<'\n';
        procedures<<p.name<<' '<<p.type<<' '<<p.level<<' '<<p.first<<' '<<p.last<<'\n';
    }
    for(const auto& error:parser.errors()) errors<<error<<'\n';
    variables.close();
    procedures.close();
    errors.close();
    if(!variables || !procedures || !errors){
        std::cerr<<"Cannot write output files.\n";
        return 1;
    }
    if(ok){
        std::cout<<"Syntax analysis succeeded.\nVariables (vname vproc vkind vtype vlev vadr):\n";
        for(const auto& p:parser.procedures()) for(const auto& v:p.variables) std::cout<<v.name<<' '<<p.name<<' '<<v.kind<<' '<<v.type<<' '<<v.level<<' '<<v.address<<'\n';
        std::cout<<"Procedures (pname ptype plev fadr ladr):\n";
        for(const auto& p:parser.procedures()) std::cout<<p.name<<' '<<p.type<<' '<<p.level<<' '<<p.first<<' '<<p.last<<'\n';
    }else{
        for(const auto& error:parser.errors()) std::cerr<<error<<'\n';
        std::cerr<<"Analysis completed with errors; symbol tables may be incomplete.\n";
    }
    for(const auto& path:paths) std::cout<<"Output: "<<path.string()<<'\n';
    return ok?0:1;
}
