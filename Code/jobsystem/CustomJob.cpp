#include "CustomJob.h"
#include <iostream> 

void CustomJob::setExecute(const CustomExecutor &executor)
{
  this->executor = executor;
}

void CustomJob::execute(json& input)
{
    executor(input); 
}
void CustomJob::setTypeId()
{
  try
  {
    std::string type = this->input["identifier"]; 
    if(type != "")
    {
      this->typeIdentifier = type; 
    }
    else 
    {
      this->typeIdentifier = "UNKcustom"; 
    }

  }
  catch(const std::exception& e)
  {
    std::cout << "unable to read job Type" << std::endl; 
    this->typeIdentifier = "UNKcustom"; 
  }
  

}
// void CustomJob::setChildJobType()
// {
//   try
//   {
//     std::string childType = input["childJobType"];
//     if(childType != "" && childType != "na")
//     {
//       childJobType = childType; 
//     }
//     else 
//     {
//       childJobType = "na"; 
//     }
//   }
//   catch(const std::exception& e)
//   {
//     std::cout << "could not read child job type " << std::endl; 
//     childJobType="UNK"; 
//   }
  
// }
// void CustomJob::setHasDependencies()
// {
//   try
//   {
//     std::string hasDep = input["hasDependencies"];
//     if(hasDep == "true")
//     {
//       hasDependencies = true; 
//     }
//     else if (hasDep == "false" || hasDep == "")
//     {
//       hasDependencies = false; 
//     }  
//   }
//   catch(const std::exception& e)
//   {
//     std::cout << "Unable to read hasDependencies" << std::endl; 
//     hasDependencies = false; 
//   }
  
// }

// void CustomJob::setGenericJobInfo()
// {
//   setTypeId(); 
//   setChildJobType(); 
//   setHasDependencies(); 
// }