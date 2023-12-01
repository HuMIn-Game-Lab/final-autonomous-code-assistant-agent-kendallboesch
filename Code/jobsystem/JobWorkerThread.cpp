#include "JobWorkerThread.h"
#include "JobSystem.h"

JobWorkerThread::JobWorkerThread(const char * uniqueName, unsigned long workerJobChannels, JobSystem* jobSystem) :
m_uniqueName(uniqueName),
m_workerJobChannels(workerJobChannels),
m_jobSystem(jobSystem){

}

JobWorkerThread::~JobWorkerThread()
{
       //PC: if we havent already, signal threat that we should exsit as soon as it can (after its cuttent job, if any)
    shutDown();
    //block on the threads main until it has a chance to finish its curr job & exit
    m_thread->join();
    delete m_thread;
    m_thread = nullptr;
}

void JobWorkerThread::workerThreadMain(void *workerThreadObject)
{
    JobWorkerThread* thisWorker = (JobWorkerThread*) workerThreadObject;
    thisWorker->work();
}

void JobWorkerThread::startUp()
{
    m_thread = new std::thread(workerThreadMain, this); 
  // m_thread = new std::thread(&JobWorkerThread::WorkerThreadMain, this);

}

void JobWorkerThread::work()
{
    while(!isStopping())
    {
        m_workerStatusMutex.lock(); 
        unsigned long workerJobChannels = m_workerJobChannels; 
    m_workerStatusMutex.unlock(); 

        Job* job = m_jobSystem->claimJob(m_workerJobChannels);
        if(job)
        {
            job->execute(job->input); 
            m_jobSystem->onJobCompleted(job);   
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1)); 
    }
}

void JobWorkerThread::shutDown()
{
    m_workerStatusMutex.lock(); 
    m_isStopping = true; 
    m_workerStatusMutex.unlock(); 
}

bool JobWorkerThread::isStopping() const
{
  //create new local variable called should close & copied memebr property variable into it 
    m_workerStatusMutex.lock(); 
    bool shouldClose = m_isStopping; 
    m_workerStatusMutex.unlock(); 

    return shouldClose; 
    }

void JobWorkerThread::setWorkerJobChannels(unsigned long workerJobChannels)
{
    m_workerStatusMutex.lock(); 
    m_workerJobChannels = workerJobChannels;
    m_workerStatusMutex.unlock(); 
}



//create trheads & still it to go to worker thread main 
// void JobWorkerThread::startUp()
// {
//     m_thread = new std::thread(WorkerThreadMain, this); 
// }

// void JobWorkerThread::work()
// {
//     while(!IsStopping())
//     {
//         m_workerStatusMutex.lock(); 
//         unsigned long workerJobChannels = m_workerJobChannels; 
//         m_workerStatusMutex.unlock(); 
        
//         Job* job = m_jobSystem->ClaimAJob(m_workerJobChannels);
//         if(job)
//         {
//             job->Execute(); 
//             m_jobSystem->OnJobCompleted(job);   
//         }

//         std::this_thread::sleep_for(std::chrono::microsecond(1)); 
//     }
// }

// bool JobWorkerThread::isStopping() const 
// {
//     //create new local variable called should close & copied memebr property variable into it 
//     m_workerStatusMutex.lock(); 
//     bool shouldClose = m_isStopping; 
//     m_workerStatusMutex.unlock(); 

//     return shouldClose; 
// }

// void JobWorkerThread::SetWorkerJobChannels(unsigned long workerJobChannels)
// {
//     m_workerStatusMutex.lock(); 
//     m_workerJobChannels = workerJobChannels;
//     m_workerStatusMutex.unlock(); 
// }
// void JobWorkerThread::WorkerThreadMain(void* workerThreadObject)
// {
//     JobWorkerthread* thisWorker = (JobWorkerThread*) workerThreadObject;
//     thisWorker->work(); 
// }