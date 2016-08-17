#include "CloudBridge.h"
#include "../Utility/JsonUtility.h"
#include "../Utility/SystemUtility.h"
#include <sstream>

CloudBridge::CloudBridge(string targetIp, int port, int maxRecvSize)
{
	this->maxRecvSize = maxRecvSize;
	this->socketToCloud = new TcpClient(targetIp, port, maxRecvSize);
	
	this->IsLoggeed = 0;
}


CloudBridge::~CloudBridge()
{
	//delete this->socketToCloud;
	cout << "Cloud bridge end" << endl;
}

NetworkError CloudBridge::Login(string id, string password)
{
	NetworkError errorCode = NetworkError_UnknownError;
	errorCode = this->socketToCloud->Connect();

	if (errorCode == NetworkError_NoError)//tcp connect success
	{
		LoginError loginError = this->checkLoginInfo(id,password);
		if (loginError == LoginError_NoError)
		{
			errorCode = NetworkError_NoError;
			this->IsLoggeed = 1;
			this->startListener();
		}
		else if (loginError==LoginError_IdError)
		{
			cout << "Id is not exists" << endl;
			errorCode = NetworkError_LoginError;
		}
		else if (loginError == LoginError_PasswordError)
		{
			cout << "password is incorrect" << endl;
			errorCode = NetworkError_LoginError;
		}		
	}	
	
	return errorCode;
}

void CloudBridge::Logout()
{
	//this->socketToCloud->Disconnect();
	if (this->IsLoggeed)
	{
		//this->socketToManager->Disconnect();
		this->stopListener();
	}	
}

//private

CloudBridge::LoginError CloudBridge::checkLoginInfo(string id, string password)
{
	LoginError errorCode = LoginError_UnknownError;
	
	string templete = "{\"REGISTER\":{\"TYPE\":\"MANAGER\",\"ID\":\"\",\"PW\":\"\"}}";
	std::vector<char> receivedData;
	std::stringstream recvResult;

	//create a json style data for register
	json_t *root = JsonUtility::LoadJsonData(templete);
	JsonUtility::SetValueInFirstObject(root, "ID", id);
	JsonUtility::SetValueInFirstObject(root, "PW", password);
	string newContent = JsonUtility::ExportJsonContent(root);

	this->socketToCloud->SendData(newContent);//send register json file

	//wait cloud server response
	unsigned long long startTime = get_millis();
	unsigned long long currentTime = 0;
	while (receivedData.size() <= 0 && this->socketToCloud->IsConnected)
	{
		this->socketToCloud->RecviveData(&receivedData);
		currentTime = get_millis();

		if ((currentTime - startTime)>1000)
		{
			cout << "Receive data timeout" << endl;
			errorCode = LoginError_Timeout;
			break;
		}
	}

	for (int i = 0; i < (int)receivedData.size(); i++)
	{
		recvResult << receivedData.at(i);
	}


	if (recvResult.str() == "Success")
	{
		errorCode = LoginError_NoError;		
	}
	else
	{
		cout << "login failed" << endl;
		errorCode = LoginError_UnknownError;		
	}
	
	return errorCode;
}

void CloudBridge::startListener()
{
	Thread_create(&this->cloudThread, (TSThreadProc)CloudBridge::cloudSocketLoopEntry, this);
	Thread_run(&this->cloudThread);

	Thread_create(&this->managerThread, (TSThreadProc)CloudBridge::managerSocketLoopEntry, this);
	Thread_run(&this->managerThread);
}

void CloudBridge::stopListener()
{
	cout << "Stop all listener" << endl;
	
	Thread_stop(&this->cloudThread);
	Thread_kill(&this->cloudThread);

	Thread_stop(&this->managerThread);
	Thread_kill(&this->managerThread);
	
}

void CloudBridge::managerSocketLoopEntry(CloudBridge *clientObj)
{
	clientObj->managerLoop();
}

void CloudBridge::cloudSocketLoopEntry(CloudBridge *clientObj)
{
	clientObj->cloudLoop();
}

void CloudBridge::managerLoop()
{
	cout << "Start manager listener" << endl;	
	this->socketToManager = new TcpClient(this->managerIp,this->managerPort , this->maxRecvSize);
	this->socketToManager->Connect();

	std::vector<char> recvBuffer;
	recvBuffer.reserve(this->maxRecvSize);
	
	while (1)
	{
		this->socketToManager->RecviveData(&recvBuffer);
		if (recvBuffer.size()>0)
		{
			cout << "Receive form manager, forwarding to Cloud" << endl;
			

			this->socketToCloud->SendData(&recvBuffer);
			recvBuffer.clear();
		}
	}
}

void CloudBridge::cloudLoop()
{
	cout << "Start cloud listener" << endl;
	std::vector<char> recvBuffer;
	recvBuffer.reserve(this->maxRecvSize);

	while (1)
	{
		this->socketToCloud->RecviveData(&recvBuffer);
		if (recvBuffer.size()>0)
		{
			cout << "Receive form Cloud, forwarding to manager" << endl;
			this->socketToManager->SendData(&recvBuffer);
			recvBuffer.clear();
		}
	}
}



