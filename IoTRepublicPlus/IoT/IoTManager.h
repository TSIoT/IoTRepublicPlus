#ifndef IoTManager_H_INCLUDED
#define IoTManager_H_INCLUDED

#include <iostream>
#include <vector>

#include "../Network/TcpServer.h"
#include "../Utility/JsonUtility.h"
#include "../Utility/SystemUtility.h"
#include "IoTUtility.h"
#include "IoTCommand.h"

using namespace std;

class  IoTManager:public TcpServer
{
public:
	 IoTManager(int port, int maxReceiveBuffer, int maxClient);
	~ IoTManager();

	void StartManager();
	void StopManager();

private:
	string ServerIoTIP="Master";
	string currentVersion = "IUDP1.0";
	string prefix = "DEV.";
	int ip_count = 0;

	//definiction(the classes only used in this class)
	class IoTDeviceInfo
	{
	public:
		string IoTIp;
		string FunctionGroup;
		string DeviceID;
		string DeviceDescription;
		int SocketIndex;
		int proxied;
		IoTDeviceInfo()
		{
			this->SocketIndex = -1;
			this->proxied = -1;
		}
	};

	class PackageBuffer
	{
	public:
		//char *buffer;
		//int receiveCount;
		std::vector<char> CharVector;

		PackageBuffer()
		{
			//this->buffer = NULL;
			//this->receiveCount = 0;
		}
	};

	//class instance
	IoTUtility *ioTUtility;
	//JsonUtility *jsonUtility;

	//members
	std::vector<IoTDeviceInfo> *registed_devices;
	PackageBuffer *packageBuffer;

	//methods
	void sendIoTPackage(IoTPackage *package, int socketIndex);
	void handlePackage(int socketIndex);
	void handleManagerPackage(int socketIndex, IoTPackage *package);
	void commandHandler(int socketIndex, IoTCommand *cmd, IoTPackage *package);

	string getNewIoTIP();

	//device info
	void addNewDevice(string iotip, json_t *root, int socketIndex);
	int findDeviceIndexByDeviceId(string id);
	int findDeviceIndexByIoTIp(string iotIp);
	std::vector<char> encodeAllRegistedDevices();

	//override TcpServer class event
	//void Event_ReceivedData(int socketIndex, char *buffer, int dataLength);
	void Event_ReceivedData(int socketIndex, std::vector<char> *buffer, int dataLength);
};



#endif
