#include <iostream>
#include <string> 
#include "jobsystem/JobSystem.h"    
#include <array> 
#include <fstream> 
#include <sstream>
#include "FlowscriptInterpreter.h"
#include <regex> 

int main() 
{
    // Create JobSystem
    JobSystem* syst = JobSystem::createOrGet(); 
    // Create Job Worker Threads
   syst->JobSystem::createWorkerThread("t1",0xFFFFFFFF); 
   syst->JobSystem::createWorkerThread("t2",0xFFFFFFFF); 
   syst->JobSystem::createWorkerThread("t3",0xFFFFFFFF); 
   syst->JobSystem::createWorkerThread("t4",0xFFFFFFFF); 
   syst->JobSystem::createWorkerThread("t5",0xFFFFFFFF); 
   syst->JobSystem::createWorkerThread("t6",0xFFFFFFFF); 
   syst->JobSystem::createWorkerThread("t7",0xFFFFFFFF); 

    // Register job type 'compile' 
    syst->registerJobType("compile",[]( json& input)
    {
        std::cout << "in compile job execute" << std::endl; 
        std::array<char, 128> buffer; 
        std::string command = "make "; 

        std::string output = ""; 
        command.append(input["inputData"]); 
        command.append(" 2>&1"); 

        FILE* pipe = popen(command.c_str(), "r"); 
        
        if(!pipe){
            std::cout << "Failed to open pipe" << std::endl; 
            return;}
        while(fgets(buffer.data(), 128, pipe) != NULL)
        {output.append(buffer.data());}

        std::string returnCode = std::to_string(pclose(pipe)); 

        //input["returnCode"] = returnCode; 

        if(returnCode != "0")
        {
            input["output"] = output;
            input["success"]="false"; 
        }
        else
        {
            input["success"] = "true";
        }
        input["output"] = output;
        std::cout << output << std::endl; 

    });
    syst->registerJobType("rest",[](json& input){
        std::cout << "in compile job execute" << std::endl; 
        std::array<char, 128> buffer; 
        std::string command = "make pyrest "; 

        std::string output = ""; 
        command.append(input["inputData"]); 
        std::string extractCommand = command;
        command.append(" 2>&1"); 

        FILE* pipe = popen(command.c_str(), "r"); 
        
        if(!pipe){
            std::cout << "Failed to open pipe" << std::endl; 
            return;}
        while(fgets(buffer.data(), 128, pipe) != NULL)
        {output.append(buffer.data());}

        std::string returnCode = std::to_string(pclose(pipe)); 
        size_t firstLine = output.find('\n');
        std::cout <<"size_t: " << firstLine << std::endl; 
        size_t second;

        //input["returnCode"] = returnCode; 

        if(returnCode != "0")
        {
            input["success"]="false"; 
        }
        else
        {
            input["success"] = "true";
        }
        output=output.substr(firstLine +1);
        second = output.find('\n');
        output=output.substr(0,second);
        std::cout << "UPDATED" << std::endl; 
        input["output"] = output;
        std::cout <<"REST OUTPUT: "<< output << std::endl; 
    }); 
    // register job type 'errorParse'
    syst->registerJobType("errorparse", [](json& input)
    {
        std::regex errorPattern("(.*):(\\d+):(\\d+): (error|warning:)+(.*)");
        std::string unparsedText = input["inputData"]; 
        std::istringstream errorStream(unparsedText); 

        bool reading = true; 
        std::string lineIn; 
        std::unordered_map<std::string, std::vector<Error>> errors; 
        while(getline(errorStream, lineIn))
        {
            Error e; 
            std::smatch matcher; 
            if(std::regex_match(lineIn, matcher, errorPattern))
            {
                e.file = matcher[1]; 
                e.lineNum = std::stoi(matcher[2]); 
                e.colNum = std::stoi(matcher[3]); 
                e.errorMessage = matcher[4]; 
                e.errorMessage.append(matcher[5]); 
                e.srcResolved = "";
                e.resDescr="";

                bool searching = true; 
                while(searching)
                {
                    std::ifstream inFS(e.file);
                    if(inFS.is_open())
                    {
                        int currLine = 0; \
                        std::string line; 
                        while(getline(inFS,line))
                        {
                            currLine++; 
                            if(currLine == e.lineNum - 1)
                            {
                                e.previousLine = line;
                            }
                            else if (currLine == e.lineNum +1)
                            {
                                e.nextLine = line; 
                                inFS.close(); 
                                searching = false;
                            }
                            
                        }
                    }
                }

                // if(getline(errorStream, e.pred))
                // {
                //     if(getline(errorStream, e.suc))
                //     {
                getline(errorStream,e.src); 

                auto error_itr = errors.find(e.file); 
                if(error_itr == errors.end())
                {
                    std::vector<Error> ers; 
                    errors.insert({e.file, ers}); 
                }    
                error_itr = errors.find(e.file); 
                error_itr->second.push_back(e); 
        //     }
                // }
              

            }
        }

        
        bool jsonReadError = false; 
        json toFile = json::object(); 
        std::ifstream prev("errors.json"); 
        if(prev.is_open() && prev.peek() != std::ifstream::traits_type::eof())
        {
            std::ostringstream ss; 
            ss << prev.rdbuf(); 
            std::string existingJson = ss.str(); 

            try
            {
                toFile = json::parse(existingJson); 
                prev.close(); 
                std::ofstream prev("errors.json"); 
                prev.close(); 
                std::cout << "cleared contents of error.json" << std::endl;
            }
            catch (const json::parse_error& e)
            {
                std::cout << "Error reading previously written json" << std::endl; 
                jsonReadError = true; 

            }
        }

        if(!jsonReadError)
        {
            for(const auto& file : errors)
            {
                json errorsArray = json::array(); 

                for(const auto& error : file.second)
                {
                    json errorObj = {
                        {"file", error.file},
                        {"errorMessage", error.errorMessage},
                        {"lineNum", error.lineNum}, 
                        {"colNum", error.colNum},
                        {"src", error.src},
                        {"srcResolved", error.srcResolved},
                        {"resDescr", error.resDescr},
                        {"previousLine",error.previousLine},
                        {"nextLine",error.nextLine}
                    }; 

                    errorsArray.push_back(errorObj); 

                }
                
                toFile[file.first] = errorsArray; 

                std::ofstream of("errors.json", std::ios::app);
                if(of.is_open())
                {
                    of << toFile.dump(2) << std::endl; 
                    of.close(); 
                    std::cout << "errors written to errors.json" << std::endl; 
                    input["output"] = "errors.json";
                }
                else
                {
                    std::cout << "failed to open errors.json" << std::endl; 
                }
            }
        }

    });
    // register job type 'scriptwrite'
    syst->registerJobType("scriptwrite",[](json& input)
    {
        std::string scriptfile = "Code/Flowscript/";
        scriptfile.append(input["inputId"]);
        scriptfile.append(".md"); 

        std::ofstream outFS(scriptfile); 
        std::string data = input["inputData"];
        std::istringstream iss(data);
        std::string line;
        while(std::getline(iss,line))
        {
            outFS << line << std::endl; 
        }
        outFS.close();
        input["success"] = "true";
        input["output"] = scriptfile; 
    });
    // register job type 'coderepair'
    syst->registerJobType("coderepair", [](json& input){
        std::string fixFile = input["inputData"]; 
        std::cout << "in code repair.\nfile: " << fixFile << std::endl; 
       // startInd = fixFile.find('Code/toCompile/');
        // fixFile = fixFile.substr(fixFile.find('\n'));
        std::cout <<"Path Length: " << fixFile.size() << std::endl; 
        std::ifstream inFS(fixFile);
        // std::ifstream inFS("fixedErrors.json");
        json data; 

        if(inFS.is_open())
        {
            try
            {
                inFS >> data; 
            }
            catch (const json::parse_error& e)
            {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl; 
                input["success"] = "false"; 
            }

            // inFS.close(); 
        }
        else 
        {
            std::cerr << "Error opening the file" << std::endl; 
            input["success"]="false"; 
        }

        for (auto& entry : data.items())
        {
            const std::string& fPath = entry.key(); 
            const json& errorArray = entry.value(); 
            std::vector<std::pair<int, std::string>> fixes; 
            for(const auto& error: errorArray)
            {
                std::string strLineNum = error["lineNum"];
                fixes.push_back(std::make_pair(std::stoi(strLineNum),error["srcResolved"])); 
            }

            std::fstream cppfile(fPath, std::ios::in | std::ios::out);
            if(cppfile.is_open())
            {
                int currentLine = 0; 

                for( int i = 0; i < fixes.size(); i++)
                {
                    int lineNum = fixes[i].first; 
                    std::string fix = fixes[i].second;
                    std::string old = "";
                    while(currentLine <= lineNum)
                    {
                        std::getline(cppfile,old);
                        if(currentLine == lineNum)
                        {
                            cppfile << fix <<std::endl; 
                        }
                    }
                }
            input["success"] = true; 
            std::string target = fPath;
            while(target.find("/") != std::string::npos)
            {
                target = target.substr(target.find("/") + 1); 
            }
            input["output"] = target;

            }
            else
            {
                input["success"]="false";
            }


        }


    });

    // Vector for jobs 
    std::vector<Job*> runningJobs; 
    /********
     * Compile Job for demoError.cpp
     * Errors:
     *  - Use of undeclared identifier 
     *  - Expected ';' after return statement 
    *********/
    json compJob {
        {"identifier", "compile"},
        {"inputId","demoerror"},
        {"inputData","demoerror"},
        {"output",""},
        {"success",""}

    };
    /*******
     * Rest job for LLM Calls
     * InputData:
     *  - pyrest: build target for the rest job 
     *  - ARGS
     *      - api endpoint or ip 
     *      - json file of errors to be resolved
     * 
    ********/
    json flowscriptGenerationJob {

        {"identifier","rest"},
        {"inputId", "flowgen"},
        {"inputData", "ARGS=\"flowgen http://localhost:4891/v1 LLMFlowscriptPrompt.txt\""},
        {"output", ""}, 
        {"success", ""}

    };
    json restJob {

        {"identifier","rest"},
        {"inputId", "resterror"},
        {"inputData", "ARGS=\"errorsolve http://localhost:4891/v1 errors.json\""},
        {"output", ""}, 
        {"success", ""}

    };

    syst->loadInput(compJob);
    syst->loadInput(restJob);

    json writeScriptJob 
    {
        {"identifier", "scriptwrite"},
        {"inputId", "script1"},
        {"inputData",""}, 
        {"output",""},
        {"success",""}
    };
    json testCodeFix
    {
        {"identifier", "coderepair"},
        {"inputId","testcodefix"},
        {"inputData", "file: python3 ./Code/RestJob.py errorsolve http://localhost:4891/v1 errors.json\nfixedErrors.json"},
        {"output",""},
        {"success",""}
    }; 

    syst->loadInput(testCodeFix);

   // syst->getAllJobTypes();
// WILL BE IN FINAL SUBMIT - taking out for now
    // Job* scriptGeneration = syst->createJob(flowscriptGenerationJob);
    // syst->queueJob(scriptGeneration); 
    // std::pair<std::string,std::string> scriptGenReturn = syst->finishJob(scriptGeneration);

    //writeScriptJob['inputData'] = scriptGenReturn.first; 
    // Job* scriptWrite = syst->createJob(writeScriptJob); 
    // syst->queueJob(scriptWrite);

    bool validUI = false; 
    std::string flowFile = ""; 
    while(!validUI)
    {
        std::cout << "pick flowscript file:\n\t1 - functioncall.md\n\t2 - codefixcheck.md\n\t3 - restfix.md\n\n"; 
       std::string input;
       std::getline(std::cin, input); 
        int ui = std::stoi(input);
        

    
        
        switch(ui)
        {
            case 1:
                flowFile = "Code/Flowscript/FunctionCall.md";
                validUI = true;
                break;
            case 2: 
                flowFile = "Code/Flowscript/codefixcheck.md";
                validUI = true;
                break;
            case 3: 
                flowFile = "Code/Flowscript/restfix.md";
                validUI = true;
                break;
            default:
                std::cout << "Invalid Choice" << std:: endl; 
    
        };
    }
    FlowscriptInterpreter* interpreter = new FlowscriptInterpreter(syst); 
    // interpreter->interpret(syst->finishJob(scriptWrite).first);
   interpreter->interpret(flowFile);
    // interpreter->interpret("Code/Flowscript/FunctionCall.md");
   //interpreter->interpret("Code/Flowscript/restfix.md");
    return 0;

}