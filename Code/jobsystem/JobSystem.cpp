
// {
//     return m_jobsCompleted;    
// }
#include <iostream> 
#include "JobSystem.h" 
#include "JobWorkerThread.h"

JobSystem* JobSystem::s_jobSystem = nullptr; 
//JobFactory* JobSystem::factory = nullptr; 
typedef void (*JobCallback)(Job* completedJob);    
//new type called JObCallBack --> can use that to make all the code i write with function pointers way simpler; 

JobSystem::JobSystem()
{
    m_jobHistory.reserve(256 * 1024); 
    //factoryStartup(); 
}
JobSystem::~JobSystem()
{
    m_workerThreadsMutex.lock(); 
    int numWorkerThreads = (int)m_workerThreads.size(); 

    //First, tell each worker thread to stop picking up jobs 
    for(int i = 0; i < numWorkerThreads; ++i)
    {
        m_workerThreads[i]->shutDown(); 
    }

    while( !m_workerThreads.empty()){
        delete m_workerThreads.back(); 
        m_workerThreads.pop_back(); 
    }
    m_workerThreadsMutex.unlock(); 
}

JobSystem* JobSystem::createOrGet()
{

    if(!s_jobSystem)
    {
        s_jobSystem = new JobSystem(); 
    }
    return s_jobSystem; 
}

void JobSystem::destroy()
{
    if(s_jobSystem)
    {
        delete s_jobSystem; 
        s_jobSystem = nullptr; 
    }
}

void JobSystem::createWorkerThread(const char* uniqueName, unsigned long workerJobChannel)
{
    JobWorkerThread* newWorker = new JobWorkerThread(uniqueName, workerJobChannel, this); 
    m_workerThreadsMutex.lock(); 
    m_workerThreads.push_back(newWorker);
    m_workerThreadsMutex.unlock(); 

    m_workerThreads.back()->startUp(); 
}

void JobSystem::destroyWorkerThread(const char *uniqueName)
{
    m_workerThreadsMutex.lock(); 
    JobWorkerThread *worker = nullptr; 
    std::vector<JobWorkerThread*>::iterator it = m_workerThreads.begin(); 

    for(; it != m_workerThreads.end(); ++it)
    {
        if((*it)->m_uniqueName == uniqueName)
       // if(strcmp((*it)->m_uniqueName, uniqueName) == 0)
        {
            worker = *it; 
            m_workerThreads.erase(it); 
            break;
        }
    }   
    m_workerThreadsMutex.unlock(); 

    if(worker)
    {
        worker->shutDown(); 
        delete worker;
    }
}

void JobSystem::queueJob(Job* job)
{
    m_jobHistoryMutex.lock(); 
     m_jobsQueuedMutex.lock(); 

    m_jobHistory.emplace_back(JobHistoryEntry(job->m_jobType, JOB_STATUS_QUEUED)); 

    m_jobHistoryMutex.unlock(); 
    m_jobsQueued.push_back(job); 
    m_jobsQueuedMutex.unlock(); 

   // std::cout << "Queued: " << GetJobID(job) << std::endl;
}
JobStatus JobSystem::getJobStatus(int jobID) const{
    m_jobHistoryMutex.lock();

    JobStatus jobStatus = JOB_STATUS_NEVER_SEEN;
    if (jobID < (int) m_jobHistory.size()){
        jobStatus = (JobStatus)(m_jobHistory[jobID].m_jobStatus);
    }

    m_jobHistoryMutex.unlock();
    return jobStatus;
}
int JobSystem::getJobID(Job* job) 
{
    return job->m_jobID;
}
bool JobSystem::isJobComplete(int jobID) const{
    return (getJobStatus(jobID)) == (JOB_STATUS_COMPLETED);
}
void JobSystem::finishCompletedJobs()
{
    std::deque<Job*> jobsCompleted; 

    m_jobsCompletedMutex.lock(); 

    jobsCompleted.swap(m_jobsCompleted); 
    m_jobsCompletedMutex.unlock(); 

    for(Job* job : jobsCompleted)
    {
        job->jobCompleteCallback(); 
        m_jobHistoryMutex.lock();
        m_jobHistory[job->m_jobID].m_jobStatus = JOB_STATUS_RETIRED; 
        m_jobHistoryMutex.unlock(); 
        delete job; 
    }
}
std::pair<std::string, std::string> JobSystem::finishJob(Job* job){
    int jobID = job->getUniqueID(); 
    while(!isJobComplete(jobID)){
        JobStatus jobStatus = getJobStatus(jobID);
        if((jobStatus == JOB_STATUS_NEVER_SEEN) || (jobStatus == JOB_STATUS_RETIRED)){
            std::cout << "Error: Waiting for job (# " << jobID << ") - no such job in JobSystem" << std::endl;
            std::string returnVal = "Error: Waiting for job (# ";
            returnVal.append(std::to_string(jobID));
            returnVal.append(") - no such job in JobSystem"); 
           // return "Error: Waiting for job (# " + jobID + ") - no such job in JobSystem"; 
           return std::make_pair(returnVal,"false");  
        }
    }
    m_jobsCompletedMutex.lock();
    Job* thisCompletedJob = nullptr;
    for(auto jcIter = m_jobsCompleted.begin(); jcIter != m_jobsCompleted.end(); ++jcIter){
        Job* someCompletedJob = *jcIter;
        if(someCompletedJob->m_jobID == jobID){
            thisCompletedJob = someCompletedJob;
            m_jobsCompleted.erase(jcIter);
            break;
        }
    }
    if(thisCompletedJob->input["success"] == "true")
    {
        thisCompletedJob->success = true;
    }
    else
    {
        thisCompletedJob->success = false; 
    }
    m_jobsCompletedMutex.unlock(); 
    if(thisCompletedJob == nullptr)
    {
        std::cout << "ERROR: Job # " << jobID << " was status completed but not found" << std::endl;
        std::string returnVal="ERROR: Job # ";
        returnVal.append(std::to_string(jobID));
        returnVal.append(" was status completed but not found");

           return std::make_pair(returnVal,thisCompletedJob->input["success"]);  
    }

    // if(thisCompletedJob->input["hasDependencies"] == "true")
    // {
    //     bool childJobTypeExists = isTypeInSystem(thisCompletedJob->input["childJobType"]);
    //     if(childJobTypeExists)
    //     {
    //         json childInput = {
    //             {"identifier", thisCompletedJob->input["childJobType"]},
    //             {"inputData", thisCompletedJob->input["passToChild"]},
    //             {"output", ""},
    //             {"hasDependencies", "false"},
    //             {"childJobType", ""},
    //             {"passToChild", ""}
    //         };

    //         thisCompletedJob->dependency = createJob(childInput); 
    //         this->queueJob(thisCompletedJob->dependency); 
    //     }
    //     else
    //     {
    //         std::cout << "Child job type not found in system" << std::endl; 
    //     }
    // }
    
    thisCompletedJob->jobCompleteCallback();

    m_jobHistoryMutex.lock();
    m_jobHistory[thisCompletedJob->m_jobID].m_jobStatus = JOB_STATUS_RETIRED;
    m_jobHistoryMutex.unlock();

    // if(thisCompletedJob->dependency!=nullptr)
    // {
    //     this->finishJob(thisCompletedJob->dependency->getUniqueID()); 
    // }

    std::string output = thisCompletedJob->input["output"];
    std::string stat = thisCompletedJob->input["success"]; 
    delete thisCompletedJob; 
    return std::make_pair(output, stat);  
 
}
void JobSystem::onJobCompleted(Job* jobJustExecuted )
{
    totalJobs++;
    m_jobsCompletedMutex.lock(); 
    m_jobsRunningMutex.lock(); 

    std::deque<Job*>::iterator runningJobItr = m_jobsRunning.begin(); 
    for(; runningJobItr!= m_jobsRunning.end(); ++runningJobItr)
    {
        if(jobJustExecuted == * runningJobItr)
        {
            m_jobHistoryMutex.lock(); 
            m_jobsRunning.erase(runningJobItr); 
            m_jobsCompleted.push_back(jobJustExecuted); 
            m_jobHistory[jobJustExecuted->m_jobID].m_jobStatus = JOB_STATUS_COMPLETED; 
            m_jobHistoryMutex.unlock(); 

            break; 
        }
    }
    m_jobsRunningMutex.unlock(); 
    m_jobsCompletedMutex.unlock(); 
}

Job* JobSystem::claimJob(unsigned long workerJobChannels)
{
    m_jobsQueuedMutex.lock(); 
    m_jobsRunningMutex.lock(); 

    Job* claimedJob = nullptr; 
    std::deque<Job *>::iterator queuedJobItr = m_jobsQueued.begin(); 

    for(; queuedJobItr != m_jobsQueued.end(); ++queuedJobItr)
    {
        Job* queuedJob = *queuedJobItr; 

        if((queuedJob->m_jobChannels & workerJobChannels) != 0)
        {
            claimedJob = queuedJob; 

            m_jobHistoryMutex.lock(); 

            m_jobsQueued.erase(queuedJobItr); 
            m_jobsRunning.push_back(claimedJob);
            m_jobHistory[claimedJob->m_jobID].m_jobStatus = JOB_STATUS_RUNNING;

            //TODO: Check 
            m_jobHistoryMutex.unlock(); 
            break; 
        }
    }
    m_jobsRunningMutex.unlock(); 
    m_jobsQueuedMutex.unlock(); 

    return claimedJob;
}

std::vector<std::string> JobSystem::getAllJobTypes()
{
    std::vector<std::string> types; 
    m_jobTypesMutex.lock();  
    std::cout << "Job Types Available: " << std::endl; 
    for(int i = 0; i < m_jobTypes.size(); i++)
    {
        std::cout << "\t -> " << m_jobTypes[i] << std::endl; 
        types.push_back(m_jobTypes[i]);
    }
    m_jobTypesMutex.unlock(); 
    return types; 
}

void JobSystem::registerJobType(const std::string identifier, std::function<void(json&)> executor)
{   
    executors[identifier] = executor; 
    m_jobTypesMutex.lock(); 
    m_jobTypes.push_back(identifier); 
    m_jobTypesMutex.unlock(); 


}

int JobSystem::createJobInstance(json& input)
{
    try
    {
       // Job* job; 
        std::string typeIdentifier = input["identifier"];
        bool jobExists = false; 
        jobExists = isTypeInSystem(typeIdentifier); 

        if(jobExists)
        {
            
                CustomJob* cJob = new CustomJob(input); 
                
                auto itr = executors.find(typeIdentifier); 
                if(itr != executors.end())
                {
                    cJob->setExecute(itr->second);
                    cJob->setTypeId(); 
                    queueJob(cJob);
                    return cJob->getUniqueID(); 
                }
        
        }
        else
        {
            std::cout << "Error: Job type does not exist in this system: " << typeIdentifier << std::endl; 
            return -1; 
        }

    }
    catch(json::parse_error e)
    {
        std::cout << "Error: Invalid JSON Object passed" << std::endl; 
        return -1; 
    }
    return -1; 
}


Job* JobSystem::createJob(json& input)
{
    try
    {
       // Job* job; 
        std::string typeIdentifier = input["identifier"];
        bool jobExists = false; 
        jobExists = isTypeInSystem(typeIdentifier); 

        if(jobExists)
        {
            
                CustomJob* cJob = new CustomJob(input); 
                
                auto itr = executors.find(typeIdentifier); 
                if(itr != executors.end())
                {
                    cJob->setExecute(itr->second);
                    cJob->setTypeId(); 
                   // queueJob(cJob);
                    return cJob; 
                }
        
        }
        else
        {
            std::cout << "Error: Job type does not exist in this system: " << typeIdentifier << std::endl; 
            return nullptr; 
        }

    }
    catch(json::parse_error e)
    {
        std::cout << "Error: Invalid JSON Object passed" << std::endl; 
        return nullptr; 
    }
    return nullptr;; 
}


bool JobSystem::isTypeInSystem(std::string typeIn)
{
    bool exists = false; 
    m_jobTypesMutex.lock(); 
    for(int i = 0; i < m_jobTypes.size(); i++)
    {
        if(m_jobTypes[i] == typeIn)
        {
            exists = true; 
            break; 
        }
    }
    m_jobTypesMutex.unlock(); 
    return exists; 

}
void JobSystem::destroyJob(int jobId)
{
    JobStatus stat = getJobStatus(jobId); 
    if(stat == JOB_STATUS_NEVER_SEEN)
    {
        std::cout << "No such job to delete" << std::endl; 
    }
    else if ( stat == JOB_STATUS_QUEUED)
    {
        m_jobsQueuedMutex.lock(); 
        auto itr = m_jobsQueued.begin(); 
        for(; itr != m_jobsQueued.end(); itr++)
        {
            Job* queuedJob = *itr; 

            if(this->getJobID(queuedJob) == jobId)
            {
                m_jobsQueued.erase(itr);
                break; 
            }
        }
        m_jobsQueuedMutex.unlock(); 
        std::cout << "Job deleted" << std::endl; 
    }

    else if(stat == JOB_STATUS_RUNNING)
    {
        m_jobsRunningMutex.lock(); 
        auto itr = m_jobsRunning.begin(); 
        for(; itr != m_jobsRunning.end(); itr++)
        {
            Job* runningJob = *itr; 

            if(this->getJobID(runningJob) == jobId)
            {
                m_jobsCompleted.erase(itr);  
                break; 
            }
        }
        m_jobsRunningMutex.unlock();    
    }
    
}
bool JobSystem::loadInput(json& jobInput)
{
    std::string type = jobInput["identifier"]; 
    std::string inputID = jobInput["inputId"]; 

    if(isTypeInSystem(type))
    {

        jobInputs.push_back({inputID, jobInput});
        // auto itr = jobInputs.find(type); 
        // if(itr == jobInputs.end())
        // {
        //     jobInputs.insert({type,{}});
        // }
        // itr = jobInputs.find(type);
        // itr->second.push_back({inputID, jobInput});

        return true;
    }
    return false; 
}
json& JobSystem::getJobInputFromID(std::string inId)
{
    for(int i = 0; i < jobInputs.size(); i++)
    {
        if(jobInputs[i].first == inId)
        {
            return jobInputs[i].second; 
        }
    }
      json jobInputStructure {

        {"identifier",""},
        {"inputId", ""},
        {"inputData", ""},
        {"output", ""}, 
        {"success", ""}

    };

    return jobInputStructure; 
}
// Job* JobSystem::createJobInstance(json& in)
// {

// }