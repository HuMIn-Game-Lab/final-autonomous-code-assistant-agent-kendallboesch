#include "FlowscriptInterpreter.h"
#include <fstream> 
#include "jobsystem/JobSystem.h" 

/******
 * 
 * 
*/
FlowscriptInterpreter::FlowscriptInterpreter(JobSystem* jobsystem) : syst(jobsystem)
{   
    this->keywords = { "digraph", "subgraph", "label", "shape", "true", "false", "point"};
    std::vector<std::string> jobTypes = syst->getAllJobTypes(); 
    for(int i = 0; i < jobTypes.size(); i++)
    {
        this->keywords.push_back(jobTypes[i]); 
    }
    this->operators = {"=","->"}; 
    this->symbols = {'{', '}', '[', ']', ';'};  
    this->validSpecialChars = "=->[]{};"; 
    this->lineTerminators = "{};"; 
    this->setSyntaxMap();

}
bool FlowscriptInterpreter::lexicalAnalysis(const std::string flowFile)
{
    bool errors = false; 
    std::ifstream file(flowFile); 
    std::vector<std::string> script; 
    char buffer[200]; 
    if(!file.is_open())
    {
        std::string s = flowFile.substr(5); 
        std::cout << " FILE SWITCH: " << s << std::endl; 
        file.open(s); 
    }
    
    if (file.is_open())
    {
        std::cout << "Flowscript File opened successsfully" << std::endl; 
        // Read in flowscript 
        while(!file.eof())
        {
            file.getline(buffer, 200); 
            script.push_back(buffer); 
        }

        for(int lineNum = 0; lineNum < script.size(); lineNum++)
        {
          //  std::string line = removeLeadingWhitespace(script[lineNum]);
            std::pair<std::string, int> p = removeLeadingWhiteSpaces(script[lineNum]);
            if(p.first == "")
            {
                continue; 
            }
            std::string line = p.first; 
            int linePos = p.second; 

            if(line[0] != '{' && line[0] != '}' && !isalpha(line[0]))
            {
                // std::string error =  "Parse Error: [line: ";
                // error.append(line);
                // error.append(", position: 0] \n\tinvalid start character");
                std::cout << "Parse Error: [line: " << lineNum << ", position: 0] \n\tinvalid start character" << std::endl; 
                errors = true; 
                           
            }
            bool tokenFound = false;
            bool eol = false; 
            Token t; 
            while(!eol) // While end of line is not reached
            {
                std::string tokenValue = ""; 
                int position = 0; 
                while(!tokenFound)  // While no token has been found
                {
                   // line = removeLeadingWhitespace(line); 
                    std::pair<std::string, int> p = removeLeadingWhiteSpaces(line); 
                    linePos+=p.second; 
                    line = p.first; 
                    char c = line[position]; 
                    if(isValidChar(c))
                    {
                        if(isValidSpecialChar(c))
                        {
                            if(isValidSymbol(c))
                            {
                                if(c != terminator)
                                {
                                    if(c == blockStart || c == blockEnd)
                                    {
                                        t.tokenType=BLOCK; 
                                    }
                                    else
                                    {
                                        t.tokenType=BRACKET;

                                    }    
                                }
                                else
                                {
                                    t.tokenType=TERMINATOR;
                                }
                                tokenValue+=c; 
                                t.tokenValue=tokenValue; 
                                
                                t.line=lineNum; 
                                t.cStart = linePos;
                                tokenFound=true; 
                            }
                            else
                            {
                                if(c == '=')
                                {
                                    tokenValue+=c;
                                    t.tokenType=OPERATOR;
                                    t.tokenValue = tokenValue; 
                                    t.line=lineNum; 
                                    t.cStart = linePos; 
                                    tokenFound = true; 
                                }
                                else if (c == '-') 
                                {
                                    std::string depCheck = ""; 
                                    depCheck+=c; 
                                    depCheck+=carrot; 
                                    if(depCheck == dependency)
                                    {
                                        tokenValue+=c; 
                                        position++; 
                                        linePos++; 
                                        tokenValue+=line[position]; 

                                        t.tokenType=OPERATOR;
                                        t.tokenValue = tokenValue; 
                                        t.line=lineNum; 
                                        t.cStart = linePos; 
                                        tokenFound = true; 
                                    }
                                    else 
                                    {
                                        std::cout << "Parse Error: [line: " << lineNum << ", position: "
                                         << linePos << "]\n\tExpected dependency operator" << std:: endl; 
                                         errors=true; 
                                    }

                                }
                            }
                        }
                        else    // alpha numeric
                        {
                            if(position == 0 && !isalpha(c))
                            {
                                std::cout << "Parse Error: [line: " << lineNum << ", position: " << linePos 
                                << "]\n\tIdentifiers must begin with alphabetical character" << std::endl; 
                                return false; 
                            }
                            else
                            {
                                while(isalnum(c))
                                {
                                    tokenValue+=c; 
                                    position++; 
                                    linePos++; 
                                    c=line[position]; 
                                }
                                position--; 
                                linePos--; 
                                if(isKeyword(tokenValue) != -1)
                                {
                                    t.tokenType=KEYWORD; 
                                    t.tokenValue=tokenValue; 
                                    t.line=lineNum; 
                                    t.cStart = linePos; 
                                    tokenFound = true; 
                                }
                                else
                                {
                                    // // if not an existing identifier, add to list 
                                    // if(isExistingIdentifier(tokenValue))
                                    // {
                                    //     this->identifiers.push_back(tokenValue);
                                    // }
                                    t.tokenValue = tokenValue;
                                    t.tokenType=IDENTIFIER; 
                                    t.line=lineNum; 
                                    t.cStart = linePos; 
                                    tokenFound = true; 
                                }
                            }

                        }
                    }
       
                    else
                    {
                      //  std::cout << "Error: invalid character: " << std::endl; 
                        errors = true;  
                       break; 
                    }
                }

                //token found 
                auto itr = tokens.find(lineNum); 
                if(itr == tokens.end())
                {
                    std::vector<Token> tks; 
                    this->tokens.insert({lineNum, tks}); 
                }
                itr=tokens.find(lineNum); 
                itr->second.push_back(t); 

               // std::cout << "TOKEN: \n\ttype: " << t.tokenType << "\n\tvalue: " << t.tokenValue << std::endl; 
                

                if(t.tokenType == TERMINATOR || line.size() < position + 1 || (t.tokenType == BLOCK) ||
                    (t.tokenType==IDENTIFIER && (itr->second[itr->second.size()-2].tokenValue == "digraph" || 
                        itr->second[itr->second.size()-2].tokenValue == "subgraph") && 
                        position == line.size() -1))
                {
                    eol = true; 
                }
                else
                {
                    position++; 
                    linePos++;
                    line = line.substr(position); 
                    tokenFound=false; 
                }
            }
            
        }
    }
    else
    {
        std::cout << "File Error: failed to open file '" << flowFile << "'" << std::endl; 
        return false; 
    }
    return true; 
}
bool FlowscriptInterpreter::isValidChar(char c)
{
    return (isalnum(c) || validSpecialChars.find(c) != std::string::npos); 
}
int FlowscriptInterpreter::isKeyword(std::string wordIn)
{
    for(int i = 0; i < keywords.size(); i++)
    {
        if(keywords[i] == wordIn)
        {
            return i; 
        }
    }
    return -1; 
}
bool FlowscriptInterpreter::isValidSpecialChar(char c)
{
    return validSpecialChars.find(c) != std::string::npos; 
}
bool FlowscriptInterpreter::isValidSymbol(char c)
{
    for(int i = 0; i < symbols.size(); i++)
    {
        if (symbols[i] == c)
        {
            return true;
        }
    }
    return false;    
}
bool FlowscriptInterpreter::isExistingIdentifier(std::string idIn)
{
    for(int i = 0; i < identifiers.size(); i++)
    {
        if(identifiers[i] == idIn)
        {
            return true;
        }
    }
    return false; 
}
bool FlowscriptInterpreter::interpret(std::string scriptFile)
{
 

    bool lex = this->lexicalAnalysis(scriptFile); 
    if(lex)
    {
        bool syn = syntacticalAnalysis();
        if(syn)
        {
            this->buildFlow();  
        }
        
    }
    
    //bool flow = this->buildFlow(); 
}
std::string FlowscriptInterpreter::removeLeadingWhitespace(std::string& lineIn)
{
    int startInd; 
    for(int i = 0; i < lineIn.size(); i++)
    {
        if(lineIn[i] != ' ')
        {
            startInd = i; 
            break; 
        }
    }

    return lineIn.substr(startInd); 
}
bool FlowscriptInterpreter::syntacticalAnalysis()
{   
    bool errors = false; 
    bool hasEndBlock = false; 

    std::vector<int> hangingBlocks; 
    for(auto lineItr = tokens.begin(); lineItr!=tokens.end(); lineItr++)
   // for(int lineNum = 0; lineNum < tokens.size(); lineNum++)
    {
        int lineNum = lineItr->first; 
        auto itr = tokens.find(lineNum); 

        if(lineNum ==0)
        {
           // std::cout << "is line 0" << std::endl; 
            if(toLexToken(itr->second) == validSyntax.find(BLOCK_TITLE)->second)
            {
                if(!isExistingIdentifier(itr->second[1].tokenValue))
                {
                    if (itr->second[0].tokenValue == keywords[DIGRAPH])
                    {
                        if (toLexToken(tokens.find(lineNum+1)->second) == validSyntax.find(BLOCK_BOUND)->second)
                        {
                            if(tokens.find(lineNum+1)->second[0].tokenValue[0] == blockStart)
                            {
                                //std::cout << "Found Beginning of script " << std::endl; 
                                programStatements.push_back({BLOCK_TITLE,tokens.find(lineNum)->second}); 
                                programStatements.push_back({BLOCK_BOUND,tokens.find(lineNum+1)->second});
                                lineNum++;                                
                                hangingBlocks.push_back(lineNum);
                            }
                            else 
                            {
                                std::cout << "Syntax Error: [Line: " << lineNum+1<< "]\n\tExpected {" << std::endl; 
                                
                            }
                        }
                        else
                        {
                            // following line is not {
                                std::cout << "Syntax Error: [Line: " << lineNum+1<< "]\n\tExpected {" << std::endl; 
                        }
                    }
                    else
                    {
                        // not digraph
                                std::cout << "Syntax Error: [Line: " << lineNum+1<< "]\n\tExpected keyword 'digraph'" << std::endl; 
                    }

                }
                else
                {
                    // identifier exists  
                    std::cout << "Syntax Error: [Line: " << lineNum+1<< "]\n\tRedefinition of identifier" << std::endl; 

                }
            }
            else
            {
                std::string e = "Syntax Error: [Line: ";
                e.append(std::to_string(lineNum)); 
                e.append("]\n\tExpected digraph definition");

            }
        }
        else // not line 0 
        {
            if (toLexToken(itr->second) == validSyntax.find(BLOCK_TITLE)->second)
            {
                if(!isExistingIdentifier(itr->second[1].tokenValue))
                {
                    // is subgraph 
                    if(itr->second[0].tokenValue == keywords[SUBGRAPH])
                    {
                        if(toLexToken(tokens.find(lineNum+1)->second) == validSyntax.find(BLOCK_BOUND)->second)
                        {
                            if(tokens.find(lineNum+1)->second[0].tokenValue[0] == blockStart)
                            {
                               // std::cout << "found thread definition" << std::endl;
                                programStatements.push_back({BLOCK_TITLE,tokens.find(lineNum)->second}); 
                                programStatements.push_back({BLOCK_BOUND,tokens.find(lineNum+1)->second});
                                lineNum++;
                                hangingBlocks.push_back(lineNum); 
                            }
                            else
                            {
                                std::cout << "Syntax Error: [line: " << lineNum+1<<", position: 0]\n\tExpected open brace" << std::endl;  
                                // wrong bracket 
                                errors = true; 
                            }
                        }
                        else
                        {
                            std::cout << "Syntax Error: [line: " << lineNum+1<<", position: 0]\n\tMissing brace" << std::endl;  
                            // missing bracket
                            errors = true; 

                        }
                    }
                    else
                    {
                        // invalid key word
                        std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[0].cStart << "]\n\tExpected open brace" << std::endl;  
                        errors = true;
                    }

                }
                else
                {
                    // identifier already in use (redefinition) 
                     std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[1].cStart << "]\n\tRedefinition of '" << itr->second[1].tokenValue <<"'" << std::endl;  
                    errors = true;
                }
            }
            else if(toLexToken(itr->second) == validSyntax.find(DEPENDENT_JOB)->second
                || toLexToken(itr->second) == validSyntax.find(UI_JOB)->second)
            {
                if(itr->second[1].tokenValue[0] < itr->second[5].tokenValue[0])
                {
                    if(itr->second[3].tokenValue[0] == operators[ASSIGNMENT][ASSIGNMENT])
                    {
                        if(!isExistingIdentifier(itr->second[0].tokenValue))
                        {
                            if(itr->second[2].tokenValue == keywords[LABEL])
                            {
                                if(isKeyword(itr->second[4].tokenValue) > 6)
                                {
                                 //   std::cout << "FOUND DEPENDENCY JOB" << std::endl; 
                                    programStatements.push_back({DEPENDENT_JOB,tokens.find(lineNum)->second}); 
                                    identifiers.push_back(itr->second[0].tokenValue);
                                    //this->jobs.push_back(itr->second); 
                                    this->jobs.insert({itr->second[0].tokenValue, itr->second}); 

                                }
                                else if (isKeyword(itr->second[4].tokenValue) == -1)
                                {
                                   // std::cout << "FOUND USER INPUT JOB" << std::endl; 
                                    programStatements.push_back({UI_JOB,tokens.find(lineNum)->second});
                                    identifiers.push_back(itr->second[0].tokenValue);
                                    this->jobs.insert({itr->second[0].tokenValue, itr->second}); 
                                }
                                else
                                {
                                    std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[4].cStart << "]\n\tIinvalid use of keyword '" << itr->second[4].tokenValue <<"'" << std::endl;  
                                    errors = true;
                               }
                            }
                            else if (itr->second[2].tokenValue == keywords[SHAPE])
                            {
                                if(itr->second[4].tokenValue == keywords[POINT])
                                {
                                   // std::cout << "END POINT" << std::endl; 
                                    programStatements.push_back({END_POINT,tokens.find(lineNum)->second}); 
                                    identifiers.push_back(itr->second[0].tokenValue); 

                                }
                                else
                                {
                                    // invalid value for shape 
                                    std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[4].cStart << "]\n\tExpected keyword 'point', received '" << itr->second[4].tokenValue <<"'" << std::endl;  
                                    errors = true;
                                }
                            }
                            else
                            {
                                //invalid keyword 
                                 std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[2].cStart << "]\n\tIinvalid use of keyword '" << itr->second[4].tokenValue <<"'" << std::endl;
                                 errors = true; 

                            }
                        }
                        else
                        {
                            // error - redefinition of identifier 
                            std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[0].cStart << "]\n\tRedefinition of '" << itr->second[0].tokenValue <<"'" << std::endl; 
                            errors = true; 

                        }
                    }
                    else
                    {
                        // wrong operator 
                         std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[3].cStart << "]\n\tExpected assignment operator, recieved '" << itr->second[3].tokenValue <<"'" << std::endl;  
                         errors = true;

                    }

                }
                else
                {
                    // wrong order of brackets 
                    std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[1].cStart << "]\n\tInvalid bracket order"<< std::endl;  
                    errors = true;

                }
            }
            else if(toLexToken(itr->second) == validSyntax.find(EXECUTIONAL_DEPENDENCY)->second
                || toLexToken(itr->second) == validSyntax.find(CONDIITONAL_DEPENDENCY)->second)
            {
                if(isExistingIdentifier(itr->second[0].tokenValue))
                {
                    if (itr->second[1].tokenValue == operators[DEPENDENCY])
                    {
                        if (isExistingIdentifier(itr->second[2].tokenValue))
                        {
                            if(itr->second.size() == 4)
                            {
                                //std::cout << "Found EXECUTIONAL DEPENDENCY" << std::endl; 
                                programStatements.push_back({EXECUTIONAL_DEPENDENCY,tokens.find(lineNum)->second}); 

                                auto depItr = dependencies.find(itr->second[2].tokenValue); 
                                if(depItr == dependencies.end())
                                {
                                    dependencies.insert({itr->second[2].tokenValue, {}}); 
                                }
                                depItr = dependencies.find(itr->second[2].tokenValue); 
                                depItr->second.push_back(itr->second[0].tokenValue); 
                                depItr = dependencies.find(itr->second[0].tokenValue);
                                if(depItr == dependencies.end())
                                {
                                    dependencies.insert({itr->second[0].tokenValue, {}}); 
                                }

                                auto orderItr = executionDetails.find(itr->second[0].tokenValue); 
                                if(orderItr == executionDetails.end())
                                {
                                    JobLogic l;
                                    
                                    l.identifier = itr->second[0].tokenValue; 
                                    l.conditional = false; 
                                    l.id_ifTrue = itr->second[2].tokenValue;
                                    l.id_ifFalse = itr->second[2].tokenValue; 
                                    
                                    executionDetails.insert({itr->second[0].tokenValue,l}); 
                                }
                           
                            }
                            else
                            {
                                if(itr->second[3].tokenValue[0] < itr->second[7].tokenValue[0])
                                {
                                    if(itr->second[4].tokenValue == keywords[LABEL])
                                    {
                                        if(itr->second[5].tokenValue == operators[ASSIGNMENT])
                                        {
                                            if (itr->second[6].tokenValue == keywords[TRUE]
                                                || itr->second[6].tokenValue == keywords[FALSE])
                                                {
                                                   // std::cout << "FOUND CONDITIONAL DEPENDENCY" << std::endl; 
                                                    programStatements.push_back({CONDIITONAL_DEPENDENCY,tokens.find(lineNum)->second});
                                                    auto depItr = dependencies.find(itr->second[2].tokenValue); 
                                                    if(depItr == dependencies.end())
                                                    {
                                                        dependencies.insert({itr->second[2].tokenValue, {}}); 
                                                    }
                                                    depItr = dependencies.find(itr->second[2].tokenValue); 
                                                    depItr->second.push_back(itr->second[0].tokenValue);   

                                                    depItr = dependencies.find(itr->second[0].tokenValue);
                                                    if(depItr == dependencies.end())
                                                    {
                                                        dependencies.insert({itr->second[0].tokenValue, {}}); 
                                                    }      


                                                    auto orderItr = executionDetails.find(itr->second[0].tokenValue); 
                                                    if(orderItr == executionDetails.end())
                                                    {
                                                        JobLogic l; 
                                                        l.conditional = true; 
                                                        l.identifier = itr->second[0].tokenValue; 
                                                        if (itr->second[6].tokenValue == keywords[TRUE])
                                                        {
                                                            l.id_ifTrue = itr->second[2].tokenValue;
                                                        }
                                                        else 
                                                        {
                                                            l.id_ifFalse = itr->second[2].tokenValue;
                                                        }

                                                        executionDetails.insert({itr->second[0].tokenValue, l});

                                                    }
                                                    else
                                                    {
                                                        if(itr->second[6].tokenValue == keywords[TRUE])
                                                        {
                                                            orderItr->second.id_ifTrue = itr->second[2].tokenValue;
                                                        }
                                                        else
                                                        {
                                                            orderItr->second.id_ifFalse = itr->second[2].tokenValue;
                                                        }


                                                    }                                             
                                                }
                                                else{
                                                    // invalid dependency definition
                                                    std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[6].cStart << "]\n\texpected true or false" << std::endl;  
                                                    errors = true;

                                                }
                                        }
                                        else{
                                            //invalid operator type
                                            std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[5].cStart << "]\n\tExpected assignment operator" << std::endl; 
                                            errors = true; 
                                        }
                                    }
                                    else
                                    {
                                        // invalid keyword ; expected label
                                        std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[4].cStart << "]\n\tInvalid keyword; Expected 'label'" << std::endl;  
                                        errors = true;

                                    }
                                }
                                else
                                {
                                    // messed up brackets 
                                    std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[3].cStart << "]\n\tInvalid bracket order"<< std::endl; 
                                    errors = true; 
                                }
                            }
                        }
                        else
                        {
                            // 2nd identifier is not found
                            std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[2].cStart << "]\n\tIdentifier not found: '" << itr->second[2].tokenValue <<"'"<< std::endl;  
                            errors = true;

                        }
                    }
                    else
                    {
                        std::cout << "Syntax Error: [line: " << lineNum << ", position: " << itr->second[1].cStart << "]\n\tExpected dependency operator" << std::endl; 
                        errors = true; 
                        std::cout << "NO" << std::endl; 
                    }
                }
               
            }
            else if(toLexToken(itr->second) == validSyntax.find(BLOCK_BOUND)->second)
            {
                if(itr->second[0].tokenValue[0] == blockEnd)
                {
                    if(hangingBlocks.size() != 0)
                    {
                        //std::cout << "FOUND BLOCK END" << std::endl; 
                        programStatements.push_back({BLOCK_BOUND,tokens.find(lineNum)->second});
                        hangingBlocks.pop_back();  
                    }
                }
            }

        }
       

    }
    if(hangingBlocks.size() == 0)
    {
        return !errors;
    }
    return false; 
}

std::pair<std::string, int> FlowscriptInterpreter::removeLeadingWhiteSpaces(std::string& in)
{

    int startInd; 
    if(in != "")
    {
        for(int i = 0; i < in.size(); i++)
        {
            if(in[i] != ' ')
            {
                startInd = i; 
                break; 
            }
        }
    }
    else{
        return std::make_pair("",0);
    }
    if(in.size() - startInd == 0)
    {
        return std::make_pair("",startInd); 
    }

   // return {startInd,std::string::substr(startInd) };  
    return std::make_pair(in.substr(startInd),startInd); 
}

void FlowscriptInterpreter::setSyntaxMap()
{
    validSyntax.insert({BLOCK_TITLE, 
                                    {KEYWORD, IDENTIFIER}});
    validSyntax.insert({DEPENDENT_JOB, 
                                    {IDENTIFIER, BRACKET, KEYWORD, OPERATOR, KEYWORD, BRACKET, TERMINATOR}});
    validSyntax.insert({UI_JOB,
                                    {IDENTIFIER, BRACKET, KEYWORD, OPERATOR, IDENTIFIER, BRACKET, TERMINATOR}});
    validSyntax.insert({END_POINT,
                                    {IDENTIFIER, BRACKET, KEYWORD, OPERATOR, KEYWORD, BRACKET, TERMINATOR}}); 
    validSyntax.insert({EXECUTIONAL_DEPENDENCY, 
                                    {IDENTIFIER, OPERATOR, IDENTIFIER, TERMINATOR}}); 
    validSyntax.insert({CONDIITONAL_DEPENDENCY, 
                                    {IDENTIFIER, OPERATOR, IDENTIFIER, BRACKET, KEYWORD, OPERATOR, KEYWORD, BRACKET, TERMINATOR }});
    validSyntax.insert({BLOCK_BOUND,
                                    {BLOCK}});
}

std::vector<FlowscriptInterpreter::LexToken> FlowscriptInterpreter::toLexToken(std::vector<Token> asStruct )
{
    std::vector<FlowscriptInterpreter::LexToken> tks; 
    for(int i = 0; i < asStruct.size(); i++)
    {
        tks.push_back(asStruct[i].tokenType);
    }

    return tks; 
}



bool FlowscriptInterpreter::getEndID()
{
   for(int i = 0; i < programStatements.size(); i++)
   {
        if(programStatements[i].first == END_POINT)
        {
            this->end_id = programStatements[i].second[0].tokenValue; 
            return true;
        }
   } 
   return false; 
}

void FlowscriptInterpreter::buildFlow()
{
    //std::vector<Token> jobToks;
    // = getJobTokens(getParentJob());
    getEndID();
    std::string jobid;
    std::vector<Token> jobToks; 

    jobToks = jobs.find(getParentJob())->second;
    std::string inputId = jobToks[4].tokenValue; //get job input 

    json jin = syst->getJobInputFromID(inputId); 
    if(jin["identifier"] != "")
    {
        Job* job = syst->createJob(jin); 
        bool completed = false; 
        syst->queueJob(job); 
        std::pair<std::string, std::string> p = syst->finishJob(job); 
        jobid = jobToks[0].tokenValue;
          auto itr = executionDetails.find(jobid); 
        while(!completed)
        {
            
            Job* j; 
            itr = executionDetails.find(jobid); 
            jobToks = jobs.find(jobid)->second; 

            if (p.second == keywords[TRUE])
            {
               // std::cout << "TRUE" << std::endl; 
                jobid=itr->second.id_ifTrue; 
            }
            else if(p.second == keywords[FALSE])
            {
               // std::cout << "FASLE" << std::endl; 
                jobid = itr->second.id_ifFalse; 
            }
            else
            {
                jobid=itr->second.id_ifTrue; 
            }
            if(jobid == this->end_id)
            {
               // std::cout << "COMPLETED" << std::endl; 
                completed = true; 
                break; 
            }
            jobToks = jobs.find(jobid)->second; 

            if(isKeyword(jobs.find(jobid)->second[4].tokenValue)
                && jobs.find(jobid)->second[4].tokenValue != keywords[POINT])
            {
                json in {
                {"identifier",jobToks[4].tokenValue},
                {"inputId", ""},
                {"inputData", p.first},
                {"output", ""}, 
                {"success", ""}
                };
                j= syst->createJob(in);
                syst->queueJob(j);
            }
            else if(jobToks[4].tokenValue == keywords[POINT])
            {
                //std::cout << "COMPLETED" << std::endl; 
                completed = true; 
               // break; 
            }
            else //UI job
            {
                json in = syst->getJobInputFromID(jobToks[4].tokenValue); 
                j = syst->createJob(in);
                syst->queueJob(j);
            }
         // std::cout <<"finishing" << std::endl;
            std::pair<std::string, std::string> p = syst->finishJob(j);
         // std::cout <<"cal;led" << std::endl; 
        }
    }
    
    
}

std::string FlowscriptInterpreter::getParentJob()
{
    auto itr = dependencies.begin(); 
    while(itr != dependencies.end())
    {
        if (itr->second.size() == 0)
        {
            return itr->first; 
        }
        itr++; 
    }

    std::cout << "Could not find parent job" << std::endl; 
    return ""; 

}