
#include "XBeeBroker_ApiMode.h"
#include "../Utility/SystemUtility.h"
#include "../IoT/IoTCommand.h"

//public method
XBeeBroker_ApiMode::XBeeBroker_ApiMode(string name, BrokerType type,string ip,int port,int comNumber,int baudrate) :IBroker(name, type,ip,port)
{
	this->RS232BaudRate = baudrate;
	this->RS232PortNumaber = comNumber;

	this->xbeeDevices.reserve(20);

	this->nowScanning = 0;

	this->clientToManager = new TcpClient(this->ManagerIp, this->ManagerServerPort, this->maxRecvBufferSize);
}

XBeeBroker_ApiMode::~XBeeBroker_ApiMode()
{
	delete this->ioTUtility;
}

void XBeeBroker_ApiMode::Start()
{
	cout << this->Name.c_str() << " Started" << endl;
	bool comportOpened = 0;
	bool serverConnected = 0;

	this->clientToManager->Connect();

	comportOpened=this->xbeeObj.begin(this->RS232PortNumaber, this->RS232BaudRate);
	serverConnected = this->clientToManager->IsConnected;


	if (comportOpened && serverConnected)
	{
		this->IsStarted = 1;
		this->ScanAllDevice();
		Thread_create(&this->mainLoopThread, (TSThreadProc)XBeeBroker_ApiMode::server_main_loop_entry, this);
		Thread_run(&this->mainLoopThread);


		this->brokerRegister();
		
	}	
}

void XBeeBroker_ApiMode::Stop()
{
	if(this->IsStarted)
	{ 
		this->xbeeObj.end(this->RS232PortNumaber);
		this->clientToManager->Disconnect();
	}	
}

void XBeeBroker_ApiMode::ScanAllDevice()
{
	const string cmdDiscover = "ND";
	const int maxFailTimes = 6;
	const int maxAddressLength = 8;

	RecvResult recvResult;

	int failTimes = 0, xbeeModuleCount=0;	
	unsigned int dh = 0, dl = 0;

	this->nowScanning = 1;

	std::vector<char> recvBuffer;
	std::vector<XBeeAddress64> xbeeAddresses;

	recvBuffer.reserve(50);

	//send scan command
	this->sendAtCommand(cmdDiscover, "");
	while (1)
	{		
		ms_sleep(1000);
		recvResult = this->recvResponse(&recvBuffer, XBeeResponseType_AtCommand);
		if (recvResult == RecvResult_Succeed)
		{
			cout << "Found XBee device" << endl;			
			dh = 0, dl = 0;

			for (int i = 0; i < maxAddressLength / 2; i++)
			{
				dh |= (unsigned char)recvBuffer.at(i + 2);
				if (i < (maxAddressLength / 2) - 1)
				{
					dh = dh << 8;
				}
			}

			for (int i = maxAddressLength / 2; i < maxAddressLength; i++)
			{
				dl |= (unsigned char)recvBuffer.at(i + 2);
				if (i < (maxAddressLength)-1)
				{
					dl = dl << 8;
				}
			}

			XBeeAddress64 address;

			address.setMsb(dh);
			address.setLsb(dl);

			xbeeAddresses.push_back(address);
			recvBuffer.clear();
		}
		else
		{
			failTimes++;			
		}

		//ms_sleep(1000);
		if (failTimes >= maxFailTimes)
			break;
	}

	//start ask acknowledge and description to all device 
	xbeeModuleCount = xbeeAddresses.size();
	
	RecvResult res;
	if (xbeeModuleCount > 0)
	{
		recvBuffer.clear();		
	}
		
	std::vector<char> synData(this->msgSyn.begin(), this->msgSyn.end());
	for (int i = 0; i < xbeeModuleCount; i++)
	{		
		this->sendData(xbeeAddresses.at(i), &synData);		
		res = this->getResponse(&recvBuffer,500);
		
		if (res != RecvResult_Succeed)
		{
			recvBuffer.clear();
			continue;
		}

		string responseMsg(recvBuffer.begin(), recvBuffer.end());
		if (responseMsg == this->msgAck)
		{
			cout << "Found a alive XBee device:"<< std::hex 
				<< xbeeAddresses.at(i).getMsb() << xbeeAddresses.at(i).getLsb() << endl;
			
			this->registerDevice(xbeeAddresses.at(i));
			recvBuffer.clear();
		}
	}


	this->nowScanning = 0;
}

void XBeeBroker_ApiMode::StartAutoRescan(int second)
{

}

void XBeeBroker_ApiMode::StopAutoRescan()
{

}


//private method

//listener
void XBeeBroker_ApiMode::server_main_loop_entry(XBeeBroker_ApiMode *serverObj)
{
	serverObj->serverLoop();
}

int XBeeBroker_ApiMode::serverLoop()
{
	cout << "XBee broker started" << endl;
	std::vector<char> recvBuffer;
	recvBuffer.reserve(this->maxRecvBufferSize);

	IoTPackage *packageFromManager;
	
	IoTUtility::GetPackageError errorFromManager;
		
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
			//cout << "XBee broker received data" << endl;
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

void XBeeBroker_ApiMode::handleReceivePackage(IoTPackage *package)
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

void XBeeBroker_ApiMode::handleForwardingPackage(IoTPackage *package)
{
	const int recvWaitTime = 500;//response max waiting time from end device(ms)

	RecvResult res;
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
		XBeeAddress64 address = this->xbeeDevices.at(targetXbeeDeviceIndex).address;
		bufferToEndDevice.clear();
		bufferToEndDevice.insert(bufferToEndDevice.begin(), cmdFromManager->ID.begin(), cmdFromManager->ID.end());
		this->sendData(address, &bufferToEndDevice);

		res = this->getResponse(&bufferFromEndDevice, recvWaitTime);
		//res = this->recvData(&bufferFromEndDevice, recvWaitTime);
	}

	//forwarding the response to manager
	if (res == RecvResult_Succeed && bufferFromEndDevice.size() > 0)
	{
		string value(bufferFromEndDevice.begin()+1, bufferFromEndDevice.end()-1); //ignore start-symbol and end-symbol
		cmdToManager = new IoTCommand(IoTCommand::command_t_ReadResponse, cmdFromManager->ID, value);
		string sendDataStr = cmdToManager->sendedData.str();
		std::vector<char> sendData(sendDataStr.begin(), sendDataStr.end());

		IoTPackage responsePackage(package->Ver, package->DesIp, package->SorIp, &sendData);
		this->clientToManager->SendData(&responsePackage.DataVectorForSending);		
	}


	delete cmdFromManager;

}

void XBeeBroker_ApiMode::handleManagerPackage(IoTPackage *package)
{
	cout << "XBee broker got manager package" << endl;
	string commandString(package->DataVector.begin(), package->DataVector.end());
	IoTCommand cmd(commandString);
	if (cmd.ID == "rescan")
	{		
		TSThread rescanThread;
		Thread_create(&rescanThread, (TSThreadProc)XBeeBroker_ApiMode::rescan_thread_entry, this);
		Thread_run(&rescanThread);
	}

}

//Broker Manager function
void XBeeBroker_ApiMode::brokerRegister()
{	
	this->selfIoTIp = this->askIoTIp();
	string selfDescription = "{\"IOTDEV\":{\"DeviceName\":\"XBee Broker\",\"FunctionGroup\":\"Broker\",\"DeviceID\":\"iotrepublic.broker.xbee_api1\",\"Component\":[{\"ID\":\"rescan\",\"Group\":\"1\",\"Name\":\"Re-Scan All\",\"Type\":\"Button\"}]}}";
	std::vector<char> sendData(selfDescription.begin(), selfDescription.end());
	IoTPackage package(this->protocolVersion, this->selfIoTIp, this->managerIoTIp, &sendData);
	this->clientToManager->SendData(&package.DataVectorForSending);
}

void XBeeBroker_ApiMode::rescan_thread_entry(XBeeBroker_ApiMode *serverObj)
{
	serverObj->ScanAllDevice();
}

//utility
void XBeeBroker_ApiMode::sendAtCommand(string cmd, string value)
{
	AtCommandRequest request = AtCommandRequest();
	unsigned char cmdChar[2];

	cmdChar[0] = cmd.at(0);
	cmdChar[1] = cmd.at(1);

	if (value.size() > 0)
	{
		request.setCommandValue((unsigned char*)value.c_str());
		request.setCommandValueLength((unsigned char)value.size());
	}

	request.setCommand(cmdChar);
	this->xbeeObj.send(request);
}

XBeeBroker_ApiMode::RecvResult XBeeBroker_ApiMode::recvResponse(std::vector<char> *recvBuffer, XBeeResponseType expectType)
{
	//receive a expect response type
	RecvResult result= RecvResult_Failed;

	AtCommandResponse response = AtCommandResponse();
	ZBRxResponse rx_response = ZBRxResponse();
	TxStatusResponse txStatus = TxStatusResponse();

	int len = 0;

	if (this->xbeeObj.readPacket(this->xbeePackageWaitTimeout))
	{
		// got a response!
		// should be an AT command response
		if (expectType == XBeeResponseType_AtCommand && 
			this->xbeeObj.getResponse().getApiId() == XBeeResponseType_AtCommand)
		{
			this->xbeeObj.getResponse().getAtCommandResponse(response);
			if (response.isOk())
			{
				result = RecvResult_Succeed;
				
				if (response.getValueLength() > 0)
				{
					len = response.getValueLength();
					for (int i = 0; i < len; i++)
					{
						recvBuffer->push_back(response.getValue()[i]);
					}
				}			
			}
			else
			{
				cout << "AT Command return error code:" << response.getStatus() << endl;

			}
		}
		else if (expectType == XBeeResponseType_RxStatus && 
			this->xbeeObj.getResponse().getApiId() == XBeeResponseType_RxStatus)
		{
			this->xbeeObj.getResponse().getZBRxResponse(rx_response);
			if (rx_response.isAvailable())
			{
				result = RecvResult_Succeed;
				len = rx_response.getDataLength();
				unsigned char *readArray = rx_response.getData();
				for (int i = 0; i < len; i++)
				{					
					recvBuffer->push_back(readArray[i]);
				}
			}
			else
			{
				cout << "Recv return error code:" << rx_response.getErrorCode() << endl;				
			}
		}
		else if (expectType == XBeeResponseType_TxStatus && 
			this->xbeeObj.getResponse().getApiId() == XBeeResponseType_TxStatus)//ZB_TX_STATUS_RESPONSE
		{
			this->xbeeObj.getResponse().getZBTxStatusResponse(txStatus);
			if (txStatus.getStatus() == SUCCESS)
			{
				result = RecvResult_Succeed;				
			}
			else
			{
				cout << "TX Error:" << txStatus.getStatus() << endl;			
			}
		}
		else
		{
			int id = this->xbeeObj.getResponse().getApiId();
			cout <<"Expect:"<< XBeeResponseType_TxStatus <<", but got:"<< id << endl;
		}
	}
	else
	{
		result = RecvResult_Timeout;
		cout << "Recv timeout" << endl;		
	}


	return result;
}

XBeeBroker_ApiMode::RecvResult XBeeBroker_ApiMode::getResponse(std::vector<char> *recvBuffer,  int timeOut)
{
	//in common using, only for IoTRepublic rule(a request command bring two reponse: tx_response and rx_response)
	RecvResult result = RecvResult_Failed;
	XBeeResponseType responseType = XBeeResponseType_Unknown;

	ZBRxResponse rxResponse = ZBRxResponse();
	TxStatusResponse txResponse = TxStatusResponse();

	int len = 0;
	int responseCount = 0;
	unsigned long long startTime = get_millis();
	unsigned long long currentTime=0;

	while (1)
	{
		currentTime = get_millis();
		if (currentTime - startTime > timeOut)
		{
			cout << "getResponse timeout!!" << endl;
			result = RecvResult_Timeout;
			break;
		}

		if (this->xbeeObj.readPacket(this->xbeePackageWaitTimeout))
		{
			// got a response!
			// should be an AT command response
			if (this->xbeeObj.getResponse().getApiId() == XBeeResponseType_RxStatus)
			{
				responseCount++;
				responseType = XBeeResponseType_RxStatus;
				this->xbeeObj.getResponse().getZBRxResponse(rxResponse);
				if (rxResponse.isAvailable())
				{
					result = RecvResult_Succeed;
					len = rxResponse.getDataLength();
					unsigned char *readArray = rxResponse.getData();
					for (int i = 0; i < len; i++)
					{
						recvBuffer->push_back(readArray[i]);
					}
				}
				else
				{
					cout << "Recv return error code:" << rxResponse.getErrorCode() << endl;
				}
			}
			else if (this->xbeeObj.getResponse().getApiId() == XBeeResponseType_TxStatus)//ZB_TX_STATUS_RESPONSE
			{
				responseCount++;
				responseType = XBeeResponseType_TxStatus;
				this->xbeeObj.getResponse().getZBTxStatusResponse(txResponse);
				if (txResponse.getStatus() == SUCCESS)
				{
					result = RecvResult_Succeed;
				}
				else
				{
					cout << "TX Error:" << txResponse.getStatus() << endl;
				}
			}
			else
			{
				int id = this->xbeeObj.getResponse().getApiId();
				cout << "Unknown response:" << id << endl;
			}
		}

		if ( responseCount==2)
		{
			break;
		}		
	}


	return result;
}

void XBeeBroker_ApiMode::sendData(XBeeAddress64 xbee_mac_addr, std::vector<char> *data)
{	
	int send_buf_len = data->size() + 2; //add start code and end code
	std::vector<unsigned char> sendBuff;

	sendBuff.reserve(send_buf_len);
	sendBuff.push_back(0x02);
	sendBuff.insert(sendBuff.end(), data->begin(), data->end());
	sendBuff.push_back(0x03);

	ZBTxRequest zbTx;

	zbTx = ZBTxRequest(xbee_mac_addr, (uint8_t*)&sendBuff.at(0), send_buf_len);
	this->xbeeObj.send(zbTx);
	
	//RS232_SendBuf(COM_XBee_API, (unsigned char*)send_buf, len + 2);
}

XBeeBroker_ApiMode::RecvResult XBeeBroker_ApiMode::recvData(std::vector<char> *dataBuffer,int msTimeout)
{
	RecvResult res = RecvResult_Failed;

	bool isStartRecv = 0,isEndRecv=0;

	unsigned long long waitTime = 0;
	unsigned long long startTime = get_millis();

	while (1)
	{
		//check if timeout
		waitTime = get_millis() - startTime;
		if (waitTime > msTimeout)
		{
			res = RecvResult_Timeout;
			break;
		}

		//recv Rx data from xbee device
		res=this->recvResponse(dataBuffer, XBeeResponseType_RxStatus);
		if (res==RecvResult_Succeed)
		{					
			//check if have start code and end code
			if (this->findStartCodeIndex(dataBuffer) >= 0 && this->findEndCodeIndex(dataBuffer) > 0)
			{
				isStartRecv = 1;
				isEndRecv = 1;
				break;
			}
		}
		
	}


	if (isStartRecv == isEndRecv == 1)//remove the start_code and end_code
	{	
		/*
		string value(dataBuffer->begin()+1, dataBuffer->end() - 1);
		dataBuffer->clear();
		dataBuffer->insert(dataBuffer->begin(), value.begin(),value.end());
		*/

		dataBuffer->erase(dataBuffer->begin());
		dataBuffer->erase(dataBuffer->end()-1);
	}

	return res;
}

int XBeeBroker_ApiMode::findStartCodeIndex(std::vector<char> *buffer)
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

int XBeeBroker_ApiMode::findEndCodeIndex(std::vector<char> *buffer)
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

unsigned int XBeeBroker_ApiMode::getCheckSum(std::vector<char> *buffer)
{
	unsigned int sum = 0;
	int size = buffer->size();
	for (int i = 0; i < size; i++)
	{
		sum += (unsigned char)buffer->at(i);
	}


	return sum;
}


//XBee network maintain
void XBeeBroker_ApiMode::registerDevice(XBeeAddress64 addr)
{	
	std::vector<char>deviceDescription = this->askDeviceDescription(addr);
	string iotIp = this->askIoTIp();

	if (deviceDescription.size() > 0 && iotIp.size()>0)
	{
		cout << "Got device description and IoT Ip" << endl;	
		this->addNewDevice(addr, iotIp);
		IoTPackage package(this->protocolVersion, iotIp, this->managerIoTIp, &deviceDescription);
		this->clientToManager->SendData(&package.DataVectorForSending);
	}
	else if(deviceDescription.size()==0)
	{
		cout << "Cannot get device description" << endl;
	}
	else if (iotIp.size() == 0)
	{
		cout << "Cannot get IoT ip" << endl;
	}
	else
	{
		cout << "Register unknown error" << endl;
	}
}

std::vector<char> XBeeBroker_ApiMode::askDeviceDescription(XBeeAddress64 addr)
{
	RecvResult resultTx = RecvResult_Failed;
	RecvResult	resultRx = RecvResult_Failed;
	std::vector<char> devPackBuf;	
	std::vector<char> sendCommand(this->msgDes.begin(), this->msgDes.end());

	const int waitDescriptionTime = 1000;
	const int maxReTryTimes = 3;
	int tryTimes = 0;
	bool keepTry = 1;
	unsigned int sum = 0;

	devPackBuf.reserve(1000);
	
	do
	{
		this->sendData(addr, &sendCommand);
		resultTx = this->recvResponse(&devPackBuf, XBeeResponseType_TxStatus);
		if (resultTx == RecvResult_Succeed)
		{
			resultRx = this->recvData(&devPackBuf, waitDescriptionTime);
		}
		else
		{
			devPackBuf.clear();
			cout << "Register failed with send command failed" << endl;
			break;
		}

		if (resultRx == RecvResult_Succeed)
		{
			sum |= (unsigned char)devPackBuf.at(devPackBuf.size() - 2);
			sum = sum << 8;
			sum |= (unsigned char)devPackBuf.at(devPackBuf.size() - 1);

			devPackBuf.erase(devPackBuf.end() - 2, devPackBuf.end());// remove the check sum bytes
			
			if (sum == this->getCheckSum(&devPackBuf))
			{
				cout << "Check sum passed" << endl;
				break;
			}
			else
			{
				devPackBuf.clear();
				tryTimes++;

				cout << "Check sum failed" << endl;
				cout << "Try " << tryTimes << " times all failed. give up this device" << endl;
			}
		}
		else
		{
			devPackBuf.clear();
			cout << "Register failed with receive command failed" << resultRx << endl;
		}
	} while (tryTimes<maxReTryTimes);

	return devPackBuf;
}


//XBee device list maintain
/*
bool XBeeBroker_ApiMode::isDeviceExists(XBeeAddress64 addr)
{
	bool exists = 0;
	int deviceCount = this->xbeeDevices.size();

	for (int i = 0; i < deviceCount; i++)
	{
		if (this->xbeeDevices.at(i).address.isEqual(addr))
		{
			exists = 1;
			break;
		}
	}

	return exists;
}
*/
int XBeeBroker_ApiMode::findDeviceIndex(XBeeAddress64 addr)
{
	int index = -1;
	int deviceCount = this->xbeeDevices.size();

	for (int i = 0; i < deviceCount; i++)
	{
		if (this->xbeeDevices.at(i).address.isEqual(addr))
		{
			index = i;
			break;
		}
	}

	return index;
}

int XBeeBroker_ApiMode::findDeviceIndex(string iotIp)
{
	int index = -1;
	int deviceCount = this->xbeeDevices.size();

	for (int i = 0; i < deviceCount; i++)
	{
		if (this->xbeeDevices.at(i).IoTIp== iotIp)
		{
			index = i;
			break;
		}
	}

	return index;
}

void XBeeBroker_ApiMode::addNewDevice(XBeeAddress64 addr, string IoTIp)
{
	DeviceInfo device;
	device.address = addr;
	device.IoTIp = IoTIp;

	int index = this->findDeviceIndex(addr);
	if (index >= 0)
	{
		cout << "The device is exists, updata old one" << endl;
		this->xbeeDevices.at(index) = device;
	}
	else
	{
		cout << "New device added" << endl;
		this->xbeeDevices.push_back(device);
	}
}