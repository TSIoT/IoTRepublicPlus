#include "IBroker.h"
#include "../Network/TcpClient.h"

//public
IBroker::IBroker(string name, BrokerType type, string ip, int port)
{
	this->Name = name;
	this->Type = type;
	this->IsStarted = 0;
	this->ManagerIp = ip;
	this->ManagerServerPort = port;

	this->ioTUtility = new IoTUtility(this->protocolVersion, this->segmentSymbol);
}


//protected
string IBroker::askIoTIp()
{	
	string iotIp = "";
	IoTPackage package(this->protocolVersion, "0", "0", NULL);
	TcpClient client(this->ManagerIp, this->ManagerServerPort, 1000);
	
	std::vector<char>recvBuffer;
	recvBuffer.reserve(1000);

	client.Connect();
	if (client.IsConnected)
	{
		client.SendData(&package.DataVectorForSending);
		while (1)
		{
			client.RecviveData(&recvBuffer);
			if (recvBuffer.size() > 0)
			{
				this->parseReceivedBuffer(&iotIp, &recvBuffer);				
				if (iotIp.size() > 0 && this->managerIoTIp.size() > 0)
					break;
			}
			
		}
	}
	client.Disconnect();
	return iotIp;
}

//private
void IBroker::parseReceivedBuffer(string *selfIoTip, std::vector<char> *buffer)
{
	IoTUtility::GetPackageError error = IoTUtility::GetPackageError_UnknownError;
	IoTPackage *recvPackage = this->ioTUtility->GetCompletedPackage(buffer, &error);

	if (recvPackage != NULL)
	{
		//get new IoT Ip
		*selfIoTip = recvPackage->DesIp;

		//get manager IoT Ip
		if (this->managerIoTIp.size() == 0)
			this->managerIoTIp = recvPackage->SorIp;

		delete recvPackage;		
	}
}