#include "lexer.h"
#include <unordered_map>

namespace{
bool letter(char c){ return (c>='a' && c<='z') || (c>='A' && c<='Z'); }
bool digit(char c){ return c>='0' && c<='9'; }

const std::unordered_map<std::string,int> kinds={
    {"begin",1},{"end",2},{"integer",3},{"if",4},{"then",5},
    {"else",6},{"function",7},{"read",8},{"write",9},
    {"=",12},{"<>",13},{"<=",14},{"<",15},{">=",16},{">",17},
    {"-",18},{"*",19},{":=",20},{"(",21},{")",22},{";",23}
};
}

LexResult scan(std::istream& input){
    LexResult result;
    std::string line;
    int lineNo=0;
    while(std::getline(input,line)){
        ++lineNo;
        if(!line.empty() && line.back()=='\r') line.pop_back();
        for(size_t i=0;i<line.size();){
            char c=line[i];
            if(c==' ' || c=='\t'){
                ++i;
                continue;
            }
            size_t start=i++;
            if(letter(c)){
                while(i<line.size() && (letter(line[i]) || digit(line[i]))) ++i;
                std::string word=line.substr(start,i-start);
                auto it=kinds.find(word);
                result.tokens.push_back({word,it==kinds.end()?10:it->second,lineNo});
            }else if(digit(c)){
                while(i<line.size() && digit(line[i])) ++i;
                result.tokens.push_back({line.substr(start,i-start),11,lineNo});
            }else{
                if(i<line.size() && ((c=='<' && (line[i]=='=' || line[i]=='>')) ||
                   (c=='>' && line[i]=='=') || (c==':' && line[i]=='='))) ++i;
                std::string word=line.substr(start,i-start);
                auto it=kinds.find(word);
                if(it!=kinds.end()) result.tokens.push_back({word,it->second,lineNo});
                else result.errors.push_back({c,lineNo});
            }
        }
        result.tokens.push_back({"EOLN",24,lineNo});
    }
    result.tokens.push_back({"EOF",25,lineNo+1});
    return result;
}
