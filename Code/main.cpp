#include <iostream>
#include <string> 
#include "jobsystem/JobSystem.h"    
#include <array> 
#include <fstream> 
#include <sstream>
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
    // register job type 'errorParse'
    syst->registerJobType("errorParse", [](json& input)
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
                getline(errorStream,e.src); 

                auto error_itr = errors.find(e.file); 
                if(error_itr == errors.end())
                {
                    std::vector<Error> ers; 
                    errors.insert({e.file, ers}); 
                }    
                error_itr = errors.find(e.file); 
                error_itr->second.push_back(e); 

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
                        {"resDescr", error.resDescr}
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
                }
                else
                {
                    std::cout << "failed to open errors.json" << std::endl; 
                }
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
    json compJob1 {
        {"identifier", "compile"},
        {"inputId","demoerror"},
        {"inputData","demoerror"},
        {"output",""},
        {"success",""}

    };
    /*******
     * Compile job for typeError.cpp
     * Errors: 
     *  - Use of undeclared identifier 
     *  - Error assigining 'char' from incompatible type 'std::string'
    ********/
    json compJob2 
    {
        {"identifier", "compile"},
        {"inputId", "typeerror"},
        {"inputData","typeerror"}, 
        {"output",""},
        {"success",""}
    };
    /*******
     * Compile job for syntaxerror.json
     * Errors: 
     *  - Expected expression
    *******/
    json compJob3 
    {
        {"identifier", "compile"},
        {"inputId", "syntaxerror"},
        {"inputData","syntaxerror"}, 
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
    json restJob {

        {"identifier","compile"},
        {"inputId", "rest1"},
        {"inputData", "pyrest ARGS=\"http://localhost:4891/v1 errors.json\""},
        {"output", ""}, 
        {"success", ""}

    };

    // Create the compile Jobs & add them to runningJobs vector
    runningJobs.push_back(syst->createJob(compJob1)); 
    runningJobs.push_back(syst->createJob(compJob2)); 
    runningJobs.push_back(syst->createJob(compJob3)); 

    // keeps track of whether or not to send a rest job
    bool needsErrorHandling = false; 

    // Queue the jobs
    for(int i = 0; i < runningJobs.size(); i++)
    {
        syst->queueJob(runningJobs[i]); 
    }
    // Finish the Jobs
    for( int j = 0; j < runningJobs.size(); j++)
    {
       std::pair<std::string, std::string> res = syst->finishJob(runningJobs[j]); 
       // If compilation was successful 
       if(res.second == "true")
       {
            std::cout << "Successful Compilation:\n" << res.first << std::endl; 
       }
       else // Compilation failed 
       {    
            needsErrorHandling = true; 
            // Set unique inputId
            std::string inputId = "errorparse"; 
            inputId.append(std::to_string(j)); 
            // Create errorparse input 
            json parse 
            {
                {"identifier", "errorParse"},
                {"inputId", inputId},
                {"inputData",res.first},
                {"output",""},
                {"success",""}
            };
            // Create, queue, & finish errorparse job
            Job* p = syst->createJob(parse); 
            syst->queueJob(p); 
            syst->finishJob(p);
        }
    }
    // if at least one of the compile jobs were unsuccessful 
    if(needsErrorHandling)
    {   
        // Create, queue, and finish a rest job
        Job* r = syst->createJob(restJob);
        syst->queueJob(r); 
        std::pair<std::string, std::string> res = syst->finishJob(r); 

        std::cout << "\n\n\nRETURNED FROM REST JOB:\n" << res.first << std:: endl; 
    }
    
    return 0;

}