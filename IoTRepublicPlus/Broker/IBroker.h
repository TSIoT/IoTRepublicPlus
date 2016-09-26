#ifndef IBroker_H_INCLUDED
#define IBroker_H_INCLUDED

#include <iostream>
#include "../IoT/IoTUtility.h"
using namespace std;

class IBroker
{
public:	
	enum BrokerType
	{
		BrokerType_Unknown,
		BrokerType_XBeeApiMode,
		BrokerType_CloudBridge,
		BrokerType_LoRa,
		BrokerType_CloudUploader
	};

	string Name;
	BrokerType Type;
	string ManagerIp;
	int ManagerServerPort;

	bool IsStarted;

	IBroker(string name, BrokerType type, string ip, int port);
	
	virtual void Start() = 0;
	virtual void Stop() = 0;
	//virtual void ScanAllDevice() = 0;
	//virtual void StartAutoRescan(int second) = 0;
	//virtual void StopAutoRescan() = 0;
			
protected:
	IoTUtility *ioTUtility=NULL;
	string protocolVersion="IUDP1.0";
	char segmentSymbol = '\0';
	string managerIoTIp = "";

	string askIoTIp();

private:
	void parseReceivedBuffer(string *selfIoTip, std::vector<char> *buffer);//for askIoTIp

};


#endif