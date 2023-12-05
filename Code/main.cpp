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
    // register 
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


    syst->getAllJobTypes();
// WILL BE IN FINAL SUBMIT - taking out for now
    // Job* scriptGeneration = syst->createJob(flowscriptGenerationJob);
    // syst->queueJob(scriptGeneration); 
    // std::pair<std::string,std::string> scriptGenReturn = syst->finishJob(scriptGeneration);

    //writeScriptJob['inputData'] = scriptGenReturn.first; 
    // Job* scriptWrite = syst->createJob(writeScriptJob); 
    // syst->queueJob(scriptWrite);
    FlowscriptInterpreter* interpreter = new FlowscriptInterpreter(syst); 
    // interpreter->interpret(syst->finishJob(scriptWrite).first);
    interpreter->interpret("Code/Flowscript/FunctionCall.md");




    // // Create the compile Jobs & add them to runningJobs vector
    // // runningJobs.push_back(syst->createJob(compJob1)); 
    // // // runningJobs.push_back(syst->createJob(compJob2)); 
    // // // runningJobs.push_back(syst->createJob(compJob3)); 

    // // // keeps track of whether or not to send a rest job
    // bool needsErrorHandling = false; 

    // // Queue the jobs
    // for(int i = 0; i < runningJobs.size(); i++)
    // {
    //     syst->queueJob(runningJobs[i]); 
    // }
    // // Finish the Jobs
    // for( int j = 0; j < runningJobs.size(); j++)
    // {
    //    std::pair<std::string, std::string> res = syst->finishJob(runningJobs[j]); 
    //    // If compilation was successful 
    //    if(res.second == "true")
    //    {
    //         std::cout << "Successful Compilation:\n" << res.first << std::endl; 
    //    }
    // //    else // Compilation failed 
    // //    {    
    // //         needsErrorHandling = true; 
    // //         // Set unique inputId
    // //         std::string inputId = "errorparse"; 
    // //         inputId.append(std::to_string(j)); 
    // //         // Create errorparse input 
    // //         json parse 
    // //         {
    // //             {"identifier", "errorparse"},
    // //             {"inputId", inputId},
    // //             {"inputData",res.first},
    // //             {"output",""},
    // //             {"success",""}
    // //         };
    // //         // Create, queue, & finish errorparse job
    // //         Job* p = syst->createJob(parse); 
    // //         syst->queueJob(p); 
    // //         syst->finishJob(p);
    // //         std::cout << "We have an issue" << std::endl; 
    // //     }
   
    // }
    // // if at least one of the compile jobs were unsuccessful 
    // if(needsErrorHandling)
    // {   
    //     // Create, queue, and finish a rest job
    //     Job* r = syst->createJob(restJob);
    //     syst->queueJob(r); 
    //     std::pair<std::string, std::string> res = syst->finishJob(r); 

    //     std::cout << "\n\n\nRETURNED FROM REST JOB:\n" << res.first << std:: endl; 
    // }

      // json input for custom compile job 
        // job completes with erros & invokes child job    
    // json compError = {
    //     {"identifier","compile"},
    //     {"inputId", "comperror"},
    //     {"inputData", "demoerror"},
    //     {"output", ""}, 
    //     {"success", ""}
    // }; 
    // json compAuto = {
    //     {"identifier","compile"},
    //     {"inputId", "compAuto"},
    //     {"inputData", "automated"},
    //     {"output", ""}, 
    //     {"success", ""}
    // }; 
    // //json input for custom compile job 
    //     // job completes weithout errors -> no child job created 
    // json compWorking = {
    //     {"identifier","compile"},
    //     {"inputId", "compWorking"},
    //     {"inputData", "demoWorking"},
    //     {"output", ""}, 
    //     {"success", ""}
    // }; 
    //   json testJob = {
    //     {"identifier","test"},
    //     {"inputId", "testJob"},
    //     {"inputData", "test data 123"},
    //     {"output", ""}, 
    //     {"success", ""}
    // }; 

    // if(syst->loadInput(compError))
    // {
    //     std::cout << "Input loaded: " << compError << std::endl; 
    // }
    // if(syst->loadInput(compAuto))
    // {
    //     std::cout << "Input loaded: " << compAuto << std::endl; 
    // }
    // if(syst->loadInput(compWorking))
    // {
    //     std::cout << "Input loaded: " << compWorking << std::endl; 
    // }
    // if(syst->loadInput(testJob))
    // {
    //     std::cout << "Input loaded: " << testJob << std::endl; 
    // }

    // FlowscriptInterpreter* intrp = new FlowscriptInterpreter(syst); 
    // intrp->interpret("Code/Flowscript/FunctionCall.md");
    return 0;

}