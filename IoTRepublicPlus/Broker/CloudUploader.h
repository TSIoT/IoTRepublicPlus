#ifndef CloudUploader_H_INCLUDED
#define CloudUploader_H_INCLUDED
#include <iostream>

#include "../Utility/thread.h"
#include "IBroker.h"
#include "../IoT/IoTUtility.h"
#include "../IoT/IoTCommand.h"
#include "../Network/TcpClient.h"
#include "../Utility/thread.h"
#include "../IoT/IoTDevice.h"
#include "../IoT/IoTRule.h"

using namespace std;

class CloudUploader :public IBroker
{
public:
	CloudUploader(string url,string loginId, string loginPw , string name, BrokerType type, string ip, int port);
	~CloudUploader();

	void Start() override;
	void Stop() override;	

private:

	class SubscriptObj
	{
	public:
		string DevId;
		string ComId;
		int ReadFrequency;
		SubscriptObj(string devId, string comId, int readFrequency)
		{
			this->DevId = devId;
			this->ComId = comId;
			this->ReadFrequency = readFrequency;
		}
	};

	TcpClient *client;	
	TSThread listenThread;
	IoTUtility *ioTUtility;	

	std::vector<IoTDevice> allDevices;
	std::vector<SubscriptObj> subscriptObjs;

	string webServerAddress ="";
	string loginId = "";
	string loginPw = "";
	string selfIoTIp = "";
	std::string readBuffer;
	int maxRecvSize=2000;

	static void cloudUploder_main_loop_entry(CloudUploader *Obj);
	void loop();
	void handleRecvData(std::vector<char> *packageBuffer);

	//upload data to cloud server
	static size_t recvWebCallback(char *contents, size_t size, size_t nmemb, void *userp);
	void uploadData(string deviceId, string value);

	//Device list maintain
	void getAllDeviceDescription();
	void saveAllDeviceDescription(IoTPackage *package);	
	int findComponentIndex(string devId, string comId);
	//bool isComponentExists(string devId, string comId);
	IoTPackage* waitIoTPackage(int waitTime);
	string getUploadDeviceId(string iotIp, string comId);

	//rule handle
	void sendSubscriptRule();

	//Broker Manager function
	void brokerRegister();

};

#endif

