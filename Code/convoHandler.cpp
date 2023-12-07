#include <iostream>
#include <fstream>
#include "jobsystem/nlohmann/json.hpp"

using json = nlohmann::json;

int main() {
    std::string sep = "\n---------------------------\n";

    std::ifstream file("Data/convo.json");
    json json_data;
    file >> json_data;
    json extractedData; 
    std::ofstream asTxt("Data/extractedCompletionData.txt");
    for (const auto& entry : json_data) {
        std::string finish_reason = entry["choices"][0]["finish_reason"];
        std::string text = entry["choices"][0]["text"];
        int created = entry["created"];
        std::string model = entry["model"];
        int completion_tokens = entry["usage"]["completion_tokens"];
        int prompt_tokens = entry["usage"]["prompt_tokens"];
        int total_tokens = entry["usage"]["total_tokens"];

        json completionData 
        {
            {"finish_reason", finish_reason},
            {"text",text},
            {"created", created},
            {"model", model},
            {"completion_tokens", completion_tokens},
            {"prompt_tokens",prompt_tokens},
            {"total_tokens", total_tokens}
        };
        asTxt << sep << "creation: " << created << "\nmodel: " << model << "\nfinish_reason: " << finish_reason <<"\nprompt_tokens: " << prompt_tokens
                << "\ncompletion_tokens: " << completion_tokens << "\ntext: " << text; 

        extractedData.push_back(completionData);
    }
    std::ofstream output_file("Data/extractedCompletionData.json");
    output_file << extractedData.dump(4);

    return 0;
}