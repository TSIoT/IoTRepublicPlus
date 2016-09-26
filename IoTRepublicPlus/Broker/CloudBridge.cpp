#include "CloudBridge.h"
#include "../Utility/JsonUtility.h"
#include "../Utility/SystemUtility.h"
#include "../IoT/IoTUtility.h"
#include <sstream>


TSMutex CloudBridge::mutexLock;

CloudBridge::CloudBridge(string targetIp, int port, int maxRecvSize,string name, BrokerType type) :IBroker(name, type, this->managerIoTIp, this->managerPort)
{
	this->maxRecvSize = maxRecvSize;
	this->socketToCloud = new TcpClient(targetIp, port, maxRecvSize);
	
	this->IsLoggeed = 0;

	Mutex_create(&CloudBridge::mutexLock);
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
		else
		{
			errorCode = NetworkError_UnknownError;;
		}
	}	
	
	return errorCode;
}

void CloudBridge::Start()
{
	this->startListener();
}

void CloudBridge::Stop()
{
	//this->socketToCloud->Disconnect();
	if (this->IsLoggeed)
	{
		//this->socketToManager->Disconnect();
		this->stopListener();
	}	
}

void CloudBridge::ScanAllDevice()
{

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

	//json_decref(root);
	//delete root;
	
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
	cout << "Cloud bridge Stop all listener" << endl;
	
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
			//Mutex_lock(&CloudBridge::mutexLock);
			//cout << "Receive form manager, forwarding to Cloud" << endl;			
			this->socketToCloud->SendData(&recvBuffer);
			recvBuffer.clear();
			//Mutex_unlock(&CloudBridge::mutexLock);
		}
		
		//cout << "free" << endl;
	}
}

void CloudBridge::cloudLoop()
{
	cout << "Start cloud listener" << endl;

	IoTUtility ioTUtility(IoTPackage::ProtocolVersion, IoTPackage::SegmentSymbol);

	std::vector<char> recvBuffer;
	std::vector<char> packageBuffer;
	packageBuffer.reserve(this->maxRecvSize);
	recvBuffer.reserve(this->maxRecvSize);

	IoTPackage *package = NULL;
	bool needReconnect = 0;

	while (1)
	{
		this->socketToCloud->RecviveData(&recvBuffer);
		if (recvBuffer.size()>0)
		{
			packageBuffer.insert(packageBuffer.end(), recvBuffer.begin(), recvBuffer.end());
			IoTUtility::GetPackageError error;
			do
			{
				package = ioTUtility.GetCompletedPackage(&packageBuffer, &error);
				if (error == IoTUtility::GetPackageError_NoError)
				{
					needReconnect = this->reconnectToManager(&package->DataVector);
					if (needReconnect)
					{
						
					}
					else
					{
						this->socketToManager->SendData(&package->DataVectorForSending);
					}
				}

				//cout << "Receive form Cloud, forwarding to manager" << endl;
				//this->socketToManager->SendData(&recvBuffer);
				recvBuffer.clear();
				delete package;
			} while (error == IoTUtility::GetPackageError_NoError);
			
		}
	}
}


bool CloudBridge::reconnectToManager(std::vector<char> *dataVector)
{
	bool needToReconnect = 0;
	if (dataVector->size() == 6)
	{
		string content(dataVector->begin(), dataVector->end());
		if (content == "discon")
		{
			needToReconnect = 1;
			cout << "Discon!" << endl;
			//restart the socket path to manager
			//Mutex_lock(&CloudBridge::mutexLock);
			this->socketToManager->Disconnect();
			Thread_stop(&this->managerThread);
			Thread_kill(&this->managerThread);
			Thread_create(&this->managerThread, (TSThreadProc)CloudBridge::managerSocketLoopEntry, this);
			Thread_run(&this->managerThread);
			//Mutex_unlock(&CloudBridge::mutexLock);
			cout << "Discon over" << endl;
		}

	}

	return needToReconnect;
}



