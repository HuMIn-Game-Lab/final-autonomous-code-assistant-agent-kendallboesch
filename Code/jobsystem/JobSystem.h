
#pragma once
#include <vector> 
#include <mutex>
#include <deque>
#include <thread>
#include <unordered_map> 
#include <functional> 
#include "Job.h"
#include "CustomJob.h"
#include "nlohmann/json.hpp"
const int JOB_TYPE_ANY = -1;

class JobWorkerThread; 

enum JobStatus{
    JOB_STATUS_NEVER_SEEN,  // job created but not put anywhjre
    JOB_STATUS_QUEUED,  // waiting to be run
    JOB_STATUS_RUNNING,
    JOB_STATUS_COMPLETED,
    JOB_STATUS_RETIRED,     // main thread has processed & done all its clean up & deleted the job 
    NUM_JOB_STATUSES
}; 

struct JobHistoryEntry
{
    JobHistoryEntry(int jobType, JobStatus jobStatus)
        : m_jobType(jobType), 
         m_jobStatus(jobStatus){}

        int m_jobType = -1; 
        JobStatus m_jobStatus = JOB_STATUS_NEVER_SEEN; 
}; 

class Job; 
class JobSystem 
{
    friend class JobWorkerThread; 

    public: 
    JobSystem(); 
    ~JobSystem(); 

    static JobSystem* createOrGet(); 
    static void destroy(); 
    int totalJobs = 0; 


    void createWorkerThread(const char* unioqueName, unsigned long workerJobChannels = 0xFFFFFFFF); 
    void queueJob(Job* job); 

    //Status queries 
    JobStatus getJobStatus(int jobID) const;
    bool isJobComplete(int jobID) const;
   // void finishJob(int jobID); 
   std::pair<std::string, std::string> finishJob(Job* toFinish); 
    int getJobID(Job* job);
    void registerJobType(const std::string, std::function<void(json&)>); 
    int createJobInstance(json&); 
    bool loadInput(json&); 
    Job* createJob(json&); 
    std::vector<std::string> getAllJobTypes(); 
    void destroyJob(int jobId); 
    std::vector<std::pair<std::string,json&>> jobInputs; 
    json& getJobInputFromID(std::string);


    private: 

    void destroyWorkerThread(const char* uniqueName); 
    Job* claimJob(unsigned long workerJobFlags); 
    void onJobCompleted(Job* jobJustExecuted); 
    void finishCompletedJobs(); 
    static JobSystem* s_jobSystem; 
    std::vector<std::string>        m_jobTypes = {"custom"};  
    bool isTypeInSystem(std::string); 


    std::vector<JobWorkerThread*>   m_workerThreads; 
    mutable std::mutex              m_workerThreadsMutex; 
    std::deque< Job*>               m_jobsQueued; 
    std::deque<Job*>                m_jobsRunning;
    std::deque<Job*>                m_jobsCompleted; 
    mutable std::mutex              m_jobsQueuedMutex; 
    mutable std::mutex              m_jobsRunningMutex; 
    mutable std::mutex              m_jobsCompletedMutex; 

    std::vector<JobHistoryEntry>    m_jobHistory; 
    mutable int                     m_jobHistoryLowestActiveIndex = 0; 
    mutable std::mutex              m_jobHistoryMutex; 

    mutable std::mutex              m_jobTypesMutex; 

    std::unordered_map<std::string, std::function<void(json&)>> executors; 
};
