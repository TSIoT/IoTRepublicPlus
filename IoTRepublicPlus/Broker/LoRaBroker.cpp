
#include "LoRaBroker.h"
#include "../IoT/IoTCommand.h"
#include "../Utility/SystemUtility.h"
#include "../Utility/rs232.h"

//public method
LoRaBroker::LoRaBroker(string name, BrokerType type,string ip,int port,int comNumber,int baudrate) :IBroker(name, type,ip,port)
{
	this->RS232BaudRate = baudrate;
	this->RS232PortNumaber = comNumber;

	this->LoRaDevices.reserve(20);

	this->nowScanning = 0;

	this->clientToManager = new TcpClient(this->ManagerIp, this->ManagerServerPort, this->maxRecvBufferSize);
}

LoRaBroker::~LoRaBroker()
{
	delete this->ioTUtility;
}

void LoRaBroker::Start()
{
	cout << this->Name.c_str() << " Started" << endl;
	bool comportOpened = 0;
	bool serverConnected = 0;
	char mode[] = { '8', 'N', '1', 0 };

	this->clientToManager->Connect();

	if (!RS232_OpenComport(this->RS232PortNumaber, this->RS232BaudRate, mode))
	{
		comportOpened = 1;
	}
	
	serverConnected = this->clientToManager->IsConnected;

	if (comportOpened && serverConnected)
	{
		this->clearComportBuffer();
		this->initLoRaModule();

		this->IsStarted = 1;
		this->ScanAllDevice();
		Thread_create(&this->mainLoopThread, (TSThreadProc)LoRaBroker::server_main_loop_entry, this);
		Thread_run(&this->mainLoopThread);

		this->brokerRegister();		
	}	
}

void LoRaBroker::Stop()
{
	if(this->IsStarted)
	{ 
		RS232_CloseComport(this->RS232PortNumaber);		
		this->clientToManager->Disconnect();
	}	
}

void LoRaBroker::ScanAllDevice()
{	
	bool targetAlive = 0;

	this->nowScanning = 1;
	this->loadLoRaConfigFile();

	IoTPackage *package;

	//check targe device alived. if not, remove the target device in this->LoRaDevices
	for (size_t i = 0; i < this->LoRaDevices.size(); i++)
	{
		//need JoinOK from lora module
		if (addNode(this->LoRaDevices.at(i).LoRaAddress))
		{
			//try this->maxRetryTimes to get end device ack
			targetAlive=this->checkAlive(this->LoRaDevices.at(i).LoRaAddress);

			if (!targetAlive)
			{
				this->LoRaDevices.erase(this->LoRaDevices.begin() + i);
				i--;
			}
		}
		else
		{
			cout << "Add node:" << this->LoRaDevices.at(i).LoRaAddress << " Failed" << endl;
			this->LoRaDevices.erase(this->LoRaDevices.begin() + i);
			i--;
		}
	}		

	//register the alived devices
	for (size_t i = 0; i < this->LoRaDevices.size(); i++)
	{
		string iotIp = IBroker::askIoTIp();
		this->LoRaDevices[i].IoTIp = iotIp;
		
		package = new IoTPackage(IoTPackage::ProtocolVersion, iotIp, IBroker::managerIoTIp, this->LoRaDevices[i].DeviceDescription);
		this->clientToManager->SendData(&package->DataVectorForSending);
		
		delete package;
	}


	this->nowScanning = 0;
}

//private method

//Broker Manager function
void LoRaBroker::brokerRegister()
{
	this->selfIoTIp = IBroker::askIoTIp();
	string selfDescription = "{\"IOTDEV\":{\"DeviceName\":\"LoRa Broker\",\"FunctionGroup\":\"Broker\",\"DeviceID\":\"iotrepublic.broker.lora\",\"Component\":[{\"ID\":\"rescan\",\"Group\":\"1\",\"Name\":\"Re-Scan All\",\"Type\":\"Button\"}]}}";
	std::vector<char> sendData(selfDescription.begin(), selfDescription.end());
	IoTPackage package(this->protocolVersion, this->selfIoTIp, this->managerIoTIp, &sendData);
	this->clientToManager->SendData(&package.DataVectorForSending);
}

void LoRaBroker::rescan_thread_entry(LoRaBroker *serverObj)
{
	serverObj->ScanAllDevice();
}

//listener
void LoRaBroker::server_main_loop_entry(LoRaBroker *serverObj)
{
	serverObj->serverLoop();
}

int LoRaBroker::serverLoop()
{
	cout << "LoRa broker started" << endl;
	std::vector<char> recvBuffer;
	recvBuffer.reserve(this->maxRecvBufferSize);

	IoTPackage *packageFromManager=NULL;
	
	IoTUtility::GetPackageError errorFromManager= IoTUtility::GetPackageError_UnknownError;
		
	while (1)
	{
		
		this->clientToManager->RecviveData(&recvBuffer);

		if (recvBuffer.size() > 0 && this->nowScanning)
		{
			//if in scanning process, drop all receive data
			recvBuffer.clear();
		}
		else if(recvBuffer.size() > 0)
		{
			cout << "LoRa broker received data" << endl;
			do
			{
				packageFromManager = this->ioTUtility->GetCompletedPackage(&recvBuffer, &errorFromManager);
				if (packageFromManager != NULL)
				{
					this->handleReceivePackage(packageFromManager);					
					delete packageFromManager;
				}

			}while(errorFromManager== IoTUtility::GetPackageError_NoError);			
		}
		
	}

	return 0;
}

void LoRaBroker::handleReceivePackage(IoTPackage *package)
{
	if (package->DesIp == this->selfIoTIp)
	{
		//the package is for this broker
		this->handleManagerPackage(package);
	}
	else
	{
		//the package need forwarding to end device
		this->handleForwardingPackage(package);
	}
}

void LoRaBroker::handleForwardingPackage(IoTPackage *package)
{
	const int recvWaitTime = 500;//response max waiting time from end device(ms)

	RecvResult res=RecvResult_Failed;
	std::vector<char> bufferToEndDevice;
	std::vector<char> bufferFromEndDevice;
	IoTCommand *cmdFromManager;
	IoTCommand *cmdToManager;
	int targetXbeeDeviceIndex = -1;
	bufferToEndDevice.reserve(20);
	bufferFromEndDevice.reserve(20);

	string rawCommandData(package->DataVector.begin(), package->DataVector.end());

	cmdFromManager = new IoTCommand(rawCommandData);

	cout << "Comand ID:" << cmdFromManager->ID << endl;
	targetXbeeDeviceIndex = this->findDeviceIndex(package->DesIp);

	if (targetXbeeDeviceIndex >= 0)
	{
		//send request id to end device
		string address = this->LoRaDevices.at(targetXbeeDeviceIndex).LoRaAddress;
		bufferToEndDevice.clear();
		bufferToEndDevice.insert(bufferToEndDevice.begin(), cmdFromManager->ID.begin(), cmdFromManager->ID.end());
		this->sendData(address, &bufferToEndDevice);
		ms_sleep(this->loraResponseWaitTimeout);
		res = this->getResponse(address, &bufferFromEndDevice);
		//res = this->recvData(address,&bufferFromEndDevice);
	}

	//forwarding the response to manager
	if (res == RecvResult_Succeed && bufferFromEndDevice.size() > 0)
	{
		//string value(bufferFromEndDevice.begin() + 1, bufferFromEndDevice.end() - 1); //ignore start-symbol and end-symbol
		string value(bufferFromEndDevice.begin() , bufferFromEndDevice.end()); //ignore start-symbol and end-symbol
		
		cmdToManager = new IoTCommand(IoTCommand::command_t_ReadResponse, cmdFromManager->ID, value);
		string sendDataStr = cmdToManager->sendedData.str();
		std::vector<char> sendData(sendDataStr.begin(), sendDataStr.end());

		IoTPackage responsePackage(package->Ver, package->DesIp, package->SorIp, &sendData);
		this->clientToManager->SendData(&responsePackage.DataVectorForSending);
	}

	this->clearComportBuffer();

	delete cmdFromManager;

}

void LoRaBroker::handleManagerPackage(IoTPackage *package)
{
	cout << "LoRa broker got manager package" << endl;
	
	string commandString(package->DataVector.begin(), package->DataVector.end());
	IoTCommand cmd(commandString);
	if (cmd.ID == "rescan")
	{
		TSThread rescanThread;
		Thread_create(&rescanThread, (TSThreadProc)LoRaBroker::rescan_thread_entry, this);
		Thread_run(&rescanThread);
	}
	
}


//utility
void LoRaBroker::initLoRaModule()
{
	string initCmd1[] = {"LoraSystemMode inNormal\r","LoraMode MASTER\r"};
	int cmdCount = sizeof(initCmd1) / sizeof(initCmd1[0]);

	for (int i = 0; i < cmdCount; i++)
	{
		this->sendAtCommand(initCmd1[i]);		
	}
}

void LoRaBroker::sendAtCommand(string cmd)
{	
	std::vector<char> recvBuffer;
	recvBuffer.reserve(50);
	unsigned char *cmdToSend = (unsigned char*)cmd.c_str();

	RS232_SendBuf(this->RS232PortNumaber, cmdToSend, cmd.size());
	this->recvAtResponse(&recvBuffer,200);	
	string response(recvBuffer.begin(), recvBuffer.end());
	
	string temp(cmd.begin(), cmd.end() - 1); //remove the '\r' for display
	if (response == "OK")
	{				
		cout << "CMD:["<< temp << "] Success" << endl;		
	}
	else
	{
		cout << "CMD:[" << temp << "] Failed!!" << endl;
		cout << response << endl;
	}
}

LoRaBroker::RecvResult LoRaBroker::recvAtResponse(std::vector<char> *dataBuffer, unsigned long long msTimeout)
{
	RecvResult res = RecvResult_Failed;

	bool isStartRecv = 0, isEndRecv = 0;
	int num = 0;

	unsigned long long waitTime = 0;
	unsigned long long startTime = get_millis();
	unsigned char *recvBuffe=new unsigned char[this->maxRecvBufferSize];	
	
	memset(recvBuffe,0, this->maxRecvBufferSize);

	while (1)
	{
		//check if timeout
		waitTime = get_millis() - startTime;
		if (waitTime > msTimeout)
		{
			res = RecvResult_Timeout;
			break;
		}

		num=RS232_PollComport(this->RS232PortNumaber, recvBuffe, this->maxRecvBufferSize);
		if (num>0)
		{
			if (isStartRecv == 0)
			{
				isStartRecv = 1;
				startTime = get_millis();
				msTimeout = 500;
			}
				

			for (int i = 0; i < num; i++)
			{
				dataBuffer->push_back(recvBuffe[i]);
			}			
		}
	}

	return res;
}

void LoRaBroker::sendData(string address,std::vector<char> *data)
{
	int index = 0,encodedSize=0,sendSIze=0;
	string sendCmd = this->cmdSendData+" "+address+" ";

	//add start code and end code
	std::vector<char> sendBuffer(data->size()+2);
	std::vector<char> encodedBuffer(this->maxRecvBufferSize);	
	
	sendBuffer.at(index++) = this->startCode;

	for (size_t i = 0; i < data->size(); i++)
	{
		sendBuffer.at(index++) = data->at(i);
	}

	sendBuffer.at(index++) = this->endCode;
	
	//encode with base64
	encodedSize=Base64encode(&encodedBuffer.at(0), &sendBuffer.at(0), sendBuffer.size());	

	//decode test
	//char testBuf[10] = { 0 };
	//Base64decode(testBuf, &encodedBuffer.at(0));


	encodedBuffer.insert(encodedBuffer.begin(), sendCmd.begin(), sendCmd.end());		

	sendSIze = encodedSize + sendCmd.size();
	encodedBuffer.at((sendSIze)) = this->cmdEndSymbol.at(0);
	sendSIze++;

	RS232_SendBuf(this->RS232PortNumaber,(unsigned char*)&encodedBuffer.at(0), sendSIze);

	this->clearComportBuffer();


}

LoRaBroker::RecvResult LoRaBroker::recvData(string address, std::vector<char> *data)
{
	int n = 0;
	const int tempBufSize = 2000;
	unsigned long long waitTime = 0;
	unsigned long long startTime = get_millis();

	cout << "Recv data from:" << address << endl;

	RecvResult res = RecvResult_Failed;	
	string recvCmd = this->cmdSendData + " " + address + " "+ this->enqCode +this->cmdEndSymbol;
	
	std::vector<char> sendCmdBuf(recvCmd.begin(), recvCmd.end());	
	std::vector<char> recvBuffer;

	recvBuffer.reserve(this->maxRecvBufferSize);

	RS232_SendBuf(this->RS232PortNumaber, (unsigned char*)&sendCmdBuf.at(0), sendCmdBuf.size());
	this->clearComportBuffer();
	
	unsigned char tempBuffer[tempBufSize] = { 0 };
	while (1)
	{
		//check if timeout
		waitTime = get_millis() - startTime;
		if (waitTime > this->loraResponseWaitTimeout*2)
		{
			cout << "Timeout" << endl;
			res = RecvResult_Timeout;
			break;
		}

		n = RS232_PollComport(this->RS232PortNumaber, tempBuffer, data->capacity());
		if (n > 0)
		{
			cout << "Got Data:";
			for (int i = 0; i < n; i++)
			{
				//data->push_back(tempBuffer[i]);
				recvBuffer.push_back(tempBuffer[i]);
				cout << tempBuffer[i];
			}
			cout << endl;

			ms_sleep(100);

			//make sure the data are all received
			n = RS232_PollComport(this->RS232PortNumaber, tempBuffer, data->capacity());
			for (int i = 0; i < n; i++)
			{
				//data->push_back(tempBuffer[i]);
				recvBuffer.push_back(tempBuffer[i]);
				cout << tempBuffer[i];
			}

			res = RecvResult_Succeed;
			break;
		}
	}

	//base64 decode
	if (res == RecvResult_Succeed)
	{
		memset(tempBuffer,0, tempBufSize);
		n=Base64decode((char*)tempBuffer, &recvBuffer.at(0));
		data->reserve(n);
		for (int i = 0; i < n; i++)
		{
			data->push_back(tempBuffer[i]);
		}

	}

	return res;
}

LoRaBroker::RecvResult LoRaBroker::getResponse(string address, std::vector<char> *recvBuffer)
{
	//in common using, only for IoTRepublic rule
	//the different between this and recvData function
	//is this function will check the start-symbol and end-symbol

	RecvResult result = RecvResult_Failed;	
	result=this->recvData(address, recvBuffer);
	int indexOfStartCode = -1, indexOfEndCode=-1;

	if (result == RecvResult_Succeed)
	{
		indexOfStartCode = this->findStartCodeIndex(recvBuffer);
		indexOfEndCode = this->findEndCodeIndex(recvBuffer);

		if (indexOfStartCode == -1 || indexOfEndCode == -1)
		{
			result = RecvResult_Failed;
			recvBuffer->clear();
		}
		else
		{
			recvBuffer->erase(recvBuffer->begin()+ indexOfStartCode);
			indexOfEndCode--;
			recvBuffer->erase(recvBuffer->begin() + indexOfEndCode);
		}
	}

	return result;
}

void LoRaBroker::loadLoRaConfigFile()
{
	int deviceCount = 0;

	vector<char> loraConfigFile;

	loraConfigFile.reserve(this->maxConfigFileSize);
	ReadTextFileToVector("../Debug/LoRa.config", &loraConfigFile);

	std::string source(loraConfigFile.begin(), loraConfigFile.end());
	
	vector<string> splitResultVector;
	splitResultVector.reserve(1000);
	size_t begin_pos = 0, end_pos = source.find(this->configFileSplitSymbol); // locate the first delimiter in string
	while (end_pos != string::npos) 
	{
		splitResultVector.push_back(source.substr(begin_pos, end_pos - begin_pos)); // extract the sub-string before current delimiter
		begin_pos = end_pos + this->configFileSplitSymbol.size();
		end_pos = source.find(this->configFileSplitSymbol, begin_pos);  // locate the next delimiter in string
	}
	splitResultVector.push_back(source.substr(begin_pos, end_pos - begin_pos));  // extract the last sub-string	

	deviceCount = splitResultVector.size() / 2;

	for (size_t i = 0; i < splitResultVector.size(); i+=2)
	{
		DeviceInfo dev;
		dev.LoRaAddress = splitResultVector.at(i);
		dev.DeviceDescription = splitResultVector.at(i+1);

		this->LoRaDevices.push_back(dev);
	}

}

void LoRaBroker::clearComportBuffer()
{
	const int bufSize = 200;
	unsigned long long startTime = get_millis();
	unsigned char readBuff[bufSize];
	int n = 0;
	while (get_millis() - startTime < 100)
	{
		n = RS232_PollComport(this->RS232PortNumaber, readBuff, bufSize);
		if (n > 0)
		{
			startTime = get_millis();
		}
	}
	//this->RS232PortNumaber
}

int LoRaBroker::findStartCodeIndex(std::vector<char> *buffer)
{
	int startCodeIndex = -1;
	int size = buffer->size();
	for (int i = 0; i < size; i++)
	{
		if (buffer->at(i) == this->startCode)
		{
			startCodeIndex = i;
			break;
		}
	}

	return startCodeIndex;
}

int LoRaBroker::findEndCodeIndex(std::vector<char> *buffer)
{
	int endCodeIndex = -1;

	int size = buffer->size();
	for (int i = 0; i < size; i++)
	{
		if (buffer->at(i) == this->endCode)
		{
			endCodeIndex = i;
			break;
		}
	}

	return endCodeIndex;
}


//LoRa network maintain
bool LoRaBroker::addNode(string address)
{
	bool isSucceed = 0;
	string cmd;
	std::vector<char> recvBuffer;
	recvBuffer.reserve(this->maxRecvBufferSize);

	for (int i = 0; i < this->maxRetryTimes; i++)
	{
		cmd = this->cmdAddNode + " " + address + this->cmdEndSymbol;

		this->sendAtCommand(cmd);
		this->recvAtResponse(&recvBuffer, this->loraResponseWaitTimeout);

		string result(recvBuffer.begin(), recvBuffer.end());
		if (result == "JoinOK")
		{
			cout << "Ready to send data to[" << address << "]" << endl;
			isSucceed = 1;
			break;
		}
		recvBuffer.clear();
	}

	return isSucceed;
}

bool LoRaBroker::removeNode(string address)
{
	bool isSucceed = 0;
	string cmd;

	std::vector<char> recvBuffer;
	recvBuffer.reserve(this->maxRecvBufferSize);

	for (int i = 0; i < this->maxRetryTimes; i++)
	{
		cmd = this->cmdRemoveNode + " " + address + this->cmdEndSymbol;
		this->sendAtCommand(cmd);
		this->recvAtResponse(&recvBuffer, this->loraResponseWaitTimeout);

		string result2(recvBuffer.begin(), recvBuffer.end());
		if (result2 == "LeaveOK")
		{
			cout << "Leave OK!" << endl;
			isSucceed = 1;
			break;
		}
		recvBuffer.clear();
	}

	return isSucceed;
}

void LoRaBroker::clearEndNodeBuffer(string address)
{
	cout << "Clear " << address << endl;
	string recvCmd = this->cmdSendData + " " + address + " " + this->enqCode + this->cmdEndSymbol;
	std::vector<char> sendCmdBuf(recvCmd.begin(), recvCmd.end());

	RS232_SendBuf(this->RS232PortNumaber, (unsigned char*)&sendCmdBuf.at(0), sendCmdBuf.size());
	ms_sleep(this->loraResponseWaitTimeout);
	this->clearComportBuffer();
	cout << "Cleared!" << endl;
}

bool LoRaBroker::checkAlive(string address)
{
	bool alive = 0;
	RecvResult recvResult = RecvResult_Failed;

	std::vector<char> recvBuffer;	
	recvBuffer.reserve(this->maxRecvBufferSize);

	for (int i = 0; i < this->maxRetryTimes; i++)
	{
		this->clearEndNodeBuffer(address);
		std::vector<char> sendedCmd(this->msgSyn.begin(), this->msgSyn.end());
		this->sendData(address, &sendedCmd);
		recvResult = this->recvData(address, &recvBuffer);

		if (recvResult == RecvResult_Succeed && memcmp("ACK", &recvBuffer.at(0), 3) == 0)
		{
			break;
		}
		else
		{
			recvResult = RecvResult_Failed;
		}
	}

	if (recvResult == RecvResult_Succeed)
		alive = 1;

	return alive;
}

int LoRaBroker::findDeviceIndex(string addr)
{
	int index = -1;
	int deviceCount = this->LoRaDevices.size();

	for (int i = 0; i < deviceCount; i++)
	{
		if (this->LoRaDevices.at(i).IoTIp==addr)
		{
			index = i;
			break;
		}
	}

	return index;
}
