#pragma once
#include "Job.h" 
#include <functional>

using CustomExecutor = std::function<void(json& in)>; 

class CustomJob : public Job 
{
    public: 
        CustomJob(json& input) : Job(input){}; 
        ~CustomJob(){}; 
        void setExecute(const CustomExecutor& executor); 
        void execute(json& in) override; 
        void setTypeId() override; 
        
    private: 

    CustomExecutor executor; 
 
     

};