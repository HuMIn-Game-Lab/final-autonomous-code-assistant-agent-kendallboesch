#pragma once // What is this? --> prevents circular includes
#include <mutex>
#include <map>
#include <deque>
#include <vector>
#include <thread>

#include "nlohmann/json.hpp"

using json = nlohmann::json; 

struct Error
{
    std::string file; 
    std::string errorMessage; 
    int lineNum;
    int colNum;
    std::string src;
    std::string srcResolved;
    std::string resDescr;  
    std::string pred; 
    std::string suc; 
};

class Job{
    friend class JobSystem; //what is friend class
    friend class JobWorkerThread;

public:
    //What are job channels
    // Inside firsy parenthesis, were defineng parameters for constructor with defaulr values
    Job(json& jobInpput ): input(jobInpput)
    {
        m_jobChannels = 0xFFFFFFFF;
        static int s_nextJobID = 0; 
        m_jobType = -1; 
        m_jobID = s_nextJobID++; 

       typeIdentifier = input["identifier"];

       //childJobType = input["childJobtype"]; 
    }
    std::map<std::string, std::vector<Error> > errorMap; 

    virtual ~Job() {};
    virtual void jobCompleteCallback() {};
    int getUniqueID() const {return m_jobID; }
    virtual void execute(json& jobInput) = 0; 
    virtual void setTypeId() = 0; 

    json input; 
    std::string typeIdentifier;
    std::string childJobType; 
    int childId = -1; 
    Job* dependency = nullptr; 
    std::string output; 
    bool success; 
 

private:
    int m_jobID;
    int m_jobType; 
    unsigned long m_jobChannels;

    


};

