#ifndef IoTRule_H_INCLUDED
#define IoTRule_H_INCLUDED
#include <iostream>
#include <string>
#include "../Utility/JsonUtility.h"

using namespace std;

class IoTRule
{
public:
	string Type = "";
	string Name="";	
	string ReadFrom = "";
	string ReadID = "";
	string ReadValue = "";
	
	string WriteTo = "";
	string WriteID = "";
	string WriteValue = "";
	string WriteRul = "";

	int ActionFrequency;

	string JsonData;

	IoTRule()
	{

	}

	IoTRule(string jsonContent)
	{		
		json_t *root= JsonUtility::LoadJsonData(jsonContent);
		if (root != NULL)
		{
			this->JsonData = jsonContent;
			this->Type= JsonUtility::GetValueInFirstObject(root, "Type");
			this->Name = JsonUtility::GetValueInFirstObject(root, "Name");
			this->ReadFrom = JsonUtility::GetValueInFirstObject(root, "ReadFrom");
			this->ReadID = JsonUtility::GetValueInFirstObject(root, "ReadID");
			this->ReadValue = JsonUtility::GetValueInFirstObject(root, "ReadValue");
			this->WriteTo = JsonUtility::GetValueInFirstObject(root, "WriteTo");
			this->WriteID = JsonUtility::GetValueInFirstObject(root, "WriteID");
			this->WriteValue = JsonUtility::GetValueInFirstObject(root, "WriteValue");
			this->WriteRul = JsonUtility::GetValueInFirstObject(root, "WriteRul");

			string actionFrequencyStr = JsonUtility::GetValueInFirstObject(root, "ActionFrequency");
			this->ActionFrequency = std::atoi(actionFrequencyStr.c_str());
		}
		
		delete root;
	}



	IoTRule(string Type,string name,
		string readFrom, string readID, string readValue, 
		string writeTo, string writeID, string writeValue, string writeRul,
		int actionFrequency)

	{
		this->Type = Type;
		this->Name = name;
		this->ReadFrom = readFrom;
		this->ReadID = readID;
		this->ReadValue = readValue;
		this->WriteTo = writeTo;
		this->WriteID = writeID;
		this->WriteValue = writeValue;
		this->WriteRul = writeRul;
		this->ActionFrequency = actionFrequency;

		this->Packet();		
	}


	~IoTRule()
	{

	}

	void Packet()
	{
		string ruleTemplate = "{\"IOTRUL\":{\"Type\":\"\",\"Name\":\"\",\"ReadFrom\":\"\",\"ReadID\":\"\",\"ReadValue\":\"\",\"WriteTo\":\"\",\"WriteID\":\"\",\"WriteValue\":\"\",\"WriteRul\":\"\",\"ActionFrequency\":\"\"}}";
		json_t *root = JsonUtility::LoadJsonData(ruleTemplate);

		JsonUtility::SetValueInFirstObject(root, "Type", this->Type);
		JsonUtility::SetValueInFirstObject(root, "Name", this->Name);

		JsonUtility::SetValueInFirstObject(root, "ReadFrom", this->ReadFrom);
		JsonUtility::SetValueInFirstObject(root, "ReadID", this->ReadID);
		JsonUtility::SetValueInFirstObject(root, "ReadValue", this->ReadValue);

		JsonUtility::SetValueInFirstObject(root, "WriteTo", this->WriteTo);
		JsonUtility::SetValueInFirstObject(root, "WriteID", this->WriteID);
		JsonUtility::SetValueInFirstObject(root, "WriteValue", this->WriteValue);
		JsonUtility::SetValueInFirstObject(root, "WriteRul", this->WriteRul);

		JsonUtility::SetValueInFirstObject(root, "ActionFrequency", std::to_string(this->ActionFrequency));		

		this->JsonData = JsonUtility::ExportJsonContent(root);	
		delete root;
	}	
};

#endif