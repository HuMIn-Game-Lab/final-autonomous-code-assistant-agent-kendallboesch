import os 

import openai
import sys
import json 
import seaborn as sns



# openai.api_base="http://localhost:4891/v1"
# promptFile = 'errors.json'

instruction = sys.argv[1]
openai.api_base=sys.argv[2]
promptFile = sys.argv[3]
model = "NA"
openai.api_key = "not needed for a local LLM"

# print(f'instruction: {instruction}')

if instruction == 'errorsolve':

    file = open(promptFile)
    data =json.load(file)
    resultFormatInstruct = "{\n{\'colNum\' : #}, \n {\'errorMessage\': \'\'},\n{\'file\': \'\'},\n{\'lineNum\': #},\n{\'nextLine\':\'\'},\n{\'previousLine\':\'\'},\n{\'resDescr\': \'\'},\n{\'src\': \'\'},\n{\'srcResolved\': \'\'}\n}\n"
    basePromptInstruct=f"This is the format of an error object:\n {resultFormatInstruct}\n"
    instructionInstruct="For each error object given, provide the resolved C++ code in the \'srcResolved\' member, and provide a description of the fix in the \'resDescr\' member.\n"
    jsonOnlyInstruct = "Return only the updated error object, do not prompt your response.\n"

    resultFormat = "{\n{\'colNum\' : #}, \n {\'errorMessage\': \'\'},\n{\'file\': \'\'},\n{\'lineNum\': #},\n{\'resDescr\': \'\'},\n{\'src\': \'\'},\n{\'srcResolved\': \'\'}\n}\n"
    basePrompt=f"This is the format of an error object:\n {resultFormat}\n"
    instruction="For each error object given, provide the resolved C++ code in the \'srcResolved\' member, and provide a description of the fix in the \'resDescr\' member.\n"
    details="\'srcResolved\' should only contain valid c++ code, unless the error cannot be resolved, then value should be 'need more context' \n Leave the rest of the errorObject input the same. "
    jsonOnly = "Return only the updated error object, do not prompt your response.\n"
    file.close()

    #print(prompt)
    if os.path.exists("Data/convo.json"):
        with open("Data/convo.json", 'r') as json_file:
            write = json.load(json_file)
    else: 
        write = []
        

    for file, errors in data.items():
        # print(f"File: {file}")
        prompt = basePrompt + instruction + jsonOnly

        for error in errors:
            prompt = prompt + str(error)
            # print(f"Error: {error}")
        


        response = openai.Completion.create(
            model=model,
            prompt=prompt,
            max_tokens=750,
            temperature=0.28,
            top_p=0.95,
            n=1,
            echo=True,
            stream=False
        )
        # print(response['choices'][0]['text'])
        text = response['choices'][0]['text']
        LLMresponse = text[len(prompt):]
        print(LLMresponse)
        
        write.append(response)
    with open("Data/convo.json", 'w') as json_file:
        json.dump(write, json_file, indent=4)
    
elif instruction == 'flowgen':
    print("FLOWGEN")
