#pragma once // know what this does
#include <mutex>
#include <map>
#include <deque>
#include <vector>
#include <thread>
#include "Job.h"

class JobSystem;

class JobWorkerThread{
    //since we dont actually create, we onlt have a pointer to jobSystem
        // promising job system will be created by end???
    friend class JobSystem;     // if included job system header in here, it would do the same thing
public:

    JobWorkerThread(const char* uniqueName, unsigned long workerJobChannels, JobSystem* jobSystem);
    ~JobWorkerThread();

    void startUp(); // PC: kick off actual thread, which will call Work();
    void work();  // PC: Called in provate thread, blocks forever until stop wotking
    void shutDown(); // PC: signal that work shuld stop at next opportunity

    bool isStopping() const;
    void setWorkerJobChannels(unsigned long workerJobChannels);
    static void workerThreadMain(void* workThreadObject); // start point

private:
    const char *                 m_uniqueName;
    unsigned long                m_workerJobChannels = 0xFFFFFFF;
    bool                         m_isStopping = false;
    JobSystem*                   m_jobSystem = nullptr;
    std::thread*                 m_thread = nullptr;
    mutable std::mutex           m_workerStatusMutex;
};