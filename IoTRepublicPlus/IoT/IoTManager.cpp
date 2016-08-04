#include "IoTManager.h"
#include <sstream>


//public method
IoTManager::IoTManager(int port, int maxReceiveBuffer, int maxClient) :TcpServer(port, maxReceiveBuffer, maxClient)
{		
	this->ioTUtility = new IoTUtility(this->currentVersion,IoTPackage::SegmentSymbol);
	//this->jsonUtility = new JsonUtility();

	//this->registed_devices = new std::vector<IoTDeviceInfo>(this->maxClient);
	this->registed_devices = new std::vector<IoTDeviceInfo>();
	this->registed_devices->reserve(this->maxClient);

	this->packageBuffer = new PackageBuffer[this->maxClient];
	
	for (int i = 0; i < this->maxClient; i++)
	{
		this->packageBuffer[i].buffer = new char[this->maxReceiveBuffer];
		this->packageBuffer[i].receiveCount = 0;
	}
}

IoTManager::~IoTManager()
{
	for (int i = 0; i < this->maxClient; i++)
	{
		delete[] this->packageBuffer[i].buffer;		
	}
	delete[] this->packageBuffer;
	//delete this->jsonUtility;
	delete this->ioTUtility;
	
}

void IoTManager::StartManager()
{
	this->StartServer();
}

void IoTManager::StopManager()
{
	this->StopServer();
}

//private method
void IoTManager::sendIoTPackage(IoTPackage *package, int socketIndex)
{
	if (package->DataForSending == NULL)
	{
		package->Packet();
	}

	this->SendDataToExistsConnection(socketIndex, package->DataForSending, package->SendLength);
}

void IoTManager::handlePackage(int socketIndex)
{
	//PackageBuffer *packageBuffer = &this->packageBuffer[socketIndex];
	IoTPackage *recvPackage = NULL;

	IoTUtility::GetPackageError error= IoTUtility::GetPackageError_UnknownError;

	do
	{
		//recvPackage = this->ioTUtility->GetCompletedPackage(packageBuffer->buffer, &packageBuffer->receiveCount, &error);
		recvPackage = this->ioTUtility->GetCompletedPackage(this->packageBuffer[socketIndex].buffer, &this->packageBuffer[socketIndex].receiveCount, &error);

		if (recvPackage == NULL)
		{
			//cout << "There is no Vaild package" << endl;		
			if (error == IoTUtility::GetPackageError_lengthIsNotLongEnough ||
				error == IoTUtility::GetPackageError_HeaderNotCompleted ||
				error == IoTUtility::GetPackageError_PackageNotCompleted)
			{
				//do not need to handle, mabye just because the package is not completed yet
				//cout << "Data not completed yet" << endl;
			}
			else if (error == IoTUtility::GetPackageError_StartWordError ||
				error == IoTUtility::GetPackageError_ChecksumError)
			{
				//data error, need to clear the buffer
				std::fill_n(this->packageBuffer[socketIndex].buffer, this->maxReceiveBuffer, 0);
				this->packageBuffer[socketIndex].receiveCount = 0;
				cout << "Received data error, clear the buffer" << endl;
			}
			else
			{
				cout << "Unknown error" << error << endl;
			}
		}
		else
		{
			//cout << "Got a vaild package" << endl;
			//printAllChar(recvPackage->Data, recvPackage->DataLength);
			if (recvPackage->SorIp == "0" && recvPackage->DesIp == "0")
			{
				cout << "Handle a ip-request package" << endl;

				IoTPackage *responsePackage = new IoTPackage(this->currentVersion, this->ServerIoTIP,
					this->getNewIoTIP(), "0", 1);
				this->sendIoTPackage(responsePackage, socketIndex);
				delete responsePackage;

			}
			else if (recvPackage->DesIp == this->ServerIoTIP)
			{
				cout << "Handle a manager package" << endl;
				this->handleManagerPackage(socketIndex, recvPackage);
			}
			else
			{
				cout << "Handle a forwarding package" << endl;
				int targetIndex = this->findDeviceIndexByIoTIp(recvPackage->DesIp);
				if (targetIndex >= 0)
				{
					int targetSocketIndex = this->registed_devices->at(targetIndex).SocketIndex;
					this->SendDataToExistsConnection(targetSocketIndex, recvPackage->DataForSending, recvPackage->SendLength);
				}
			}

			delete recvPackage;
			
			//this->ioTUtility.FreeIoTPackage(package);
		}
	} while (recvPackage!=NULL);	
}

void IoTManager::handleManagerPackage(int socketIndex,IoTPackage *package)
{	
	string rootName = std::string();
	string command(package->Data);
	command = command + "\0";
	//json_error_t error;
	//json_t *root= json_loads(command.c_str(), 0, &error);
	json_t *root = JsonUtility::LoadJsonData(command);

	if (root != NULL)
	{
		rootName = JsonUtility::GetFirstKeyName(root);
		if (rootName == "IOTDEV")
		{
			cout << "A Register package include device info" << endl;
			this->addNewDevice(package->SorIp,root,socketIndex);
		}
		else if (rootName == "IOTCMD")
		{
			cout << "A Command package" << endl;
			IoTCommand cmd(package->Data);
			this->commandHandler(socketIndex, &cmd, package);
			//this->commandHandler(socketIndex,);
		}
	}		
}

void IoTManager::commandHandler(int socketIndex,IoTCommand *cmd, IoTPackage *package)
{
	if (cmd->ID == "Dis_All")
	{

	}
	else if (cmd->ID == "Dis_NPx")
	{
		std::vector<char> buffer=this->encodeAllRegistedDevices();
		IoTPackage *sendPackage = new IoTPackage(this->currentVersion, this->ServerIoTIP, package->SorIp, &buffer[0], buffer.size());

		this->SendDataToExistsConnection(socketIndex, sendPackage->DataForSending, sendPackage->SendLength);
		delete sendPackage;
	}
	else if (cmd->ID == "Del_Dev")
	{

	}
	else if (cmd->ID == "Dis_Pxd")
	{

	}	
	else if (cmd->ID == "Prx_Add")
	{

	}
	else if (cmd->ID == "Prx_Rmv")
	{

	}
	else
	{
		cout << "Unknown command type(this should not happened)" << endl;
	}
}

string IoTManager::getNewIoTIP()
{
	stringstream newIp;
	newIp << this->prefix;
	newIp << ip_count;
	this->ip_count++;


	return newIp.str();
}

//device info
void IoTManager::addNewDevice(string iotip, json_t *root, int socketIndex)
{
	IoTDeviceInfo *newDevice = new IoTDeviceInfo();
	newDevice->IoTIp = iotip;
	newDevice->DeviceID = JsonUtility::GetValueInFirstObject(root, "DeviceID");
	newDevice->FunctionGroup = JsonUtility::GetValueInFirstObject(root, "FunctionGroup");
	newDevice->DeviceDescription = JsonUtility::ExportJsonContent(root);
	newDevice->SocketIndex = socketIndex;
	
	//Check if the device id is already exists	
	int deviceIndex = this->findDeviceIndexByDeviceId(newDevice->DeviceID);
	if (deviceIndex >= 0) //if device is already exists
	{
		cout << newDevice->DeviceID << "is already exists!!update the exists device infomation" << endl;
		this->registed_devices->erase(this->registed_devices->begin()+deviceIndex);	
	}
	this->registed_devices->push_back(*newDevice);	
}

int IoTManager::findDeviceIndexByDeviceId(string id)
{
	int index = -1;
	int deviceSize = this->registed_devices->size();

	for (int i = 0; i < deviceSize; i++)
	{
		if (this->registed_devices->at(i).DeviceID == id)
		{
			index = i;
			break;
		}
	}

	return index;
}

int IoTManager::findDeviceIndexByIoTIp(string iotIp)
{
	int index = -1;
	int deviceSize = this->registed_devices->size();

	for (int i = 0; i < deviceSize; i++)
	{
		if (this->registed_devices->at(i).IoTIp == iotIp)
		{
			index = i;
			break;
		}
	}

	return index;
}

std::vector<char> IoTManager::encodeAllRegistedDevices()
{
	int reserveSize = 500;
	int deviceCount = this->registed_devices->size();
	//char *devices = NULL;
	std::vector<char> temp;
	temp.reserve(reserveSize*deviceCount);
	
	for (int i = 0; i < deviceCount; i++)
	{
		IoTDeviceInfo *devInfo = &this->registed_devices->at(i);
		//devInfo->IoTIp
		//IoTIP
		for (int j = 0; j < devInfo->IoTIp.length(); j++)
		{
			temp.push_back(devInfo->IoTIp.c_str()[j]);
		}
		temp.push_back(IoTPackage::SegmentSymbol);

		//Device description
		for (int j = 0; j < devInfo->DeviceDescription.length(); j++)
		{
			temp.push_back(devInfo->DeviceDescription.c_str()[j]);
		}
		temp.push_back(IoTPackage::SegmentSymbol);
	}
	return temp;		
}



//Event handler
void IoTManager::Event_ReceivedData(int socketIndex, char *buffer, int dataLength)
{	
	cout << "Manager received data("<< dataLength << ")" << endl;	
	charcat(this->packageBuffer[socketIndex].buffer, buffer,this->packageBuffer[socketIndex].receiveCount, dataLength);
	this->packageBuffer[socketIndex].receiveCount += dataLength;
	this->handlePackage(socketIndex);
}
