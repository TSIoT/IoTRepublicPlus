#ifndef IoTDevice_H_INCLUDED
#define IoTDevice_H_INCLUDED

#include <iostream>
#include <vector>
#include <string>
#include "IoTComponent.h"
#include "../Utility/JsonUtility.h"
using namespace std;

class IoTDevice
{
public:
	string IoTIP;
	string DeviceID;
	string DeviceName;
	string FunctionGroup;

	std::vector<IoTComponent> Component;

	IoTDevice();
	~IoTDevice();

	void SetJSDescription(string desc)
	{
		json_t *jsObj = JsonUtility::LoadJsonData(desc);
		this->DeviceID = JsonUtility::GetValueInFirstObject(jsObj, "DeviceID");
		this->DeviceName = JsonUtility::GetValueInFirstObject(jsObj, "DeviceName");
		this->FunctionGroup = JsonUtility::GetValueInFirstObject(jsObj, "FunctionGroup");
		string arrayName = "Component";
		int comCount = JsonUtility::GetArrayLengthInFirstObject(jsObj, arrayName);
		IoTComponent *com;
		for (int i = 0; i < comCount; i++)
		{
			com = new IoTComponent();
			com->ID = JsonUtility::GetArrayValueInFirstObject(jsObj, arrayName, i, "ID");
			com->Name = JsonUtility::GetArrayValueInFirstObject(jsObj, arrayName, i, "Name");
			com->Group = JsonUtility::GetArrayValueInFirstObject(jsObj, arrayName, i, "Group");
			string type = JsonUtility::GetArrayValueInFirstObject(jsObj, arrayName, i, "Type");

			if (type == "Button")
			{
				com->Type = IoTComponent::ComType::Button;
			}
			else if (type == "Content")
			{
				com->Type = IoTComponent::ComType::Content;
			}
			else
			{
				com->Type = IoTComponent::ComType::Unknown;
			}

			if (com->Type == IoTComponent::ComType::Content)
			{
				string str_rdFrequency = JsonUtility::GetArrayValueInFirstObject(jsObj, arrayName, i, "ReadFrequency");
				com->ReadFrequency = std::stoi(str_rdFrequency);
			}
			this->Component.push_back(*com);
		}
	}
private:

};
#endif

