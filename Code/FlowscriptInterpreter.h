    #pragma once
#include <iostream> 
#include <string>
#include <vector>
#include <map> 
#include "jobsystem/JobSystem.h"


class FlowscriptInterpreter
{

    
    struct JobLogic
    {
        std::string identifier; 
        std::string id_ifTrue;
        std::string id_ifFalse; 
        bool conditional; 
    };
    enum LexToken
    {
        KEYWORD,
        IDENTIFIER,
        OPERATOR,
        BRACKET,
        BLOCK,
        TERMINATOR
    };
    enum Keyword
    {
        DIGRAPH,
        SUBGRAPH,
        LABEL,
        SHAPE,
        TRUE,
        FALSE,
        POINT,
        NONE
    };
    enum Operator
    {
        ASSIGNMENT,
        DEPENDENCY
    };
    struct Token
    {
        LexToken tokenType;
        std::string tokenValue;   
        int line; 
        int cStart;
        int cEnd; 
    };
    enum SynToken
    {
        BLOCK_TITLE,
        BLOCK_BOUND,
        DEPENDENT_JOB,
        UI_JOB,
        END_POINT,
        EXECUTIONAL_DEPENDENCY,
        CONDIITONAL_DEPENDENCY,
        START_POINT
    };
    public: 
        FlowscriptInterpreter(JobSystem* jobsystem);         
        ~FlowscriptInterpreter(){};        
        bool interpret(std::string flowscriptFile); 
        
         

    private:

    // Private attributes 
        char terminator = ';'; 
        char carrot = '>';
        char blockStart = '{'; 
        char blockEnd = '}'; 
        std::string dependency = "->";
  
        JobSystem* syst; 

        std::vector<std::string> keywords; 
        std::vector<const char*> operators; 
        std::vector<char> symbols; 
        std::string validSpecialChars;
        std::string lineTerminators; 

        std::vector<std::string> identifiers; 
        std::map<int,std::vector<Token>> tokens;
        std::map<SynToken, std::vector<LexToken>> validSyntax;  

       // std::vector<std::vector<Token>> jobs; 
        std::map<std::string, std::vector<Token>> jobs;
        std::vector<Job*> processJobs; 
        ///std::vector<std::pair<std::string, std::vector<std::string>>> dependencies; 
        std::map<std::string, std::vector<std::string>> dependencies;   // childParent
        std::map<std::string, std::vector<std::string>> parentChildren; 
        std::vector<std::pair<SynToken, std::vector<Token>>> getAllDependencies(); 
        std::map<std::string, JobLogic> executionDetails; 
        std::string end_id;
        std::string start_id;
        std::string job0;
    // Private functions
        bool lexicalAnalysis(const std::string flowscriptFile);
        bool syntacticalAnalysis(); 
        std::string removeLeadingWhitespace(std::string& linein);
        std::pair<std::string, int> removeLeadingWhiteSpaces(std::string& lineIn);  
        bool isValidChar(char c);
        bool isValidSpecialChar(char c);
        bool isValidSymbol(char c); 
        int isKeyword(std::string wordIn);
        bool isExistingIdentifier(std::string idIn); 
        void setSyntaxMap(); 
        bool checkScriptBounds(); 
        bool checkIsBlockTitle(std::vector<Token>); 
        std::vector<std::vector<std::string>> executionPaths; 
        bool syntaticalAnalysis(); 

        std::vector<SynToken> toSyntaxTokens(std::vector<Token>); 
        std::vector<LexToken> toLexToken(std::vector<Token>); 
        std::vector<std::string> synErrors; 

        std::vector<std::pair<SynToken, std::vector<Token>>> programStatements; 
        void buildFlow();
        bool checkProcessFlow();
        bool createAllJobs();
        bool getEndID(); 
        bool getStartID();
        std::string getParentJob(); 
        std::vector<Token> getJobTokens(std::string);


}; 