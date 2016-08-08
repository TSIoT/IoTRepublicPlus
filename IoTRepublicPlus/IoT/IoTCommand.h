#ifndef IoTCommand_H_INCLUDED
#define IoTCommand_H_INCLUDED

#include <iostream>
#include "../Utility/JsonUtility.h"

using namespace std;

class IoTCommand
{
public:
	enum command_t
	{
		command_t_None = 0,
		command_t_ReadRequest = 1,
		command_t_ReadResponse = 2,
		command_t_Write = 3,
		command_t_Management = 4,
	};

	command_t CmdType;
	string ID;
	string Value;
	stringstream sendedData;

	IoTCommand(string content)
	{
		json_t *root;
		root=JsonUtility::LoadJsonData(content);
		string jsonRootName = JsonUtility::GetFirstKeyName(root);

		if (jsonRootName == this->rootName)
		{
			this->ID = JsonUtility::GetValueInFirstObject(root,"ID");
			this->Value = JsonUtility::GetValueInFirstObject(root, "Value");
			string type= JsonUtility::GetValueInFirstObject(root, "Type");
			if (type == "Req")
			{
				this->CmdType = command_t_ReadRequest;
			}
			else if (type == "Res")
			{
				this->CmdType = command_t_ReadResponse;
			}
			else if (type == "Wri")
			{
				this->CmdType = command_t_Write;
			}
			else if (type == "Man")
			{
				this->CmdType = command_t_Management;
			}
		}

		sendedData << content;
	}

	~IoTCommand()
	{
		this->sendedData.clear();
		this->sendedData.str(std::string());
	}

	void Packect()
	{
		string templete = "{\"IOTCMD\":{\"Type\":\"None\",\"ID\":\"0\",\"Value\":\"0\"}}";
		json_t *root;
		root = JsonUtility::LoadJsonData(templete);
		if (root)
		{
			string typeStr = "";
			switch (this->CmdType)
			{
			case IoTCommand::command_t_None:
				typeStr = "None";
				break;

			case IoTCommand::command_t_ReadRequest:
				typeStr = "Req";
				break;

			case IoTCommand::command_t_ReadResponse:
				typeStr = "Res";
				break;

			case IoTCommand::command_t_Write:
				typeStr = "Wri";
				break;

			case IoTCommand::command_t_Management:
				typeStr = "Man";
				break;

			default:
				break;
			}
			//cout << "Packect" << endl;
			JsonUtility::SetValueInFirstObject(root,"Type", typeStr);
			JsonUtility::SetValueInFirstObject(root, "ID", this->ID);
			JsonUtility::SetValueInFirstObject(root, "Value", this->Value);

			sendedData.clear();
			sendedData.str(std::string());
			sendedData << JsonUtility::ExportJsonContent(root);
		}
	}

private:
	string rootName = "IOTCMD";
};
#endif
