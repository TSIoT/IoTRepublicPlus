#ifndef LoRaBroker_H_INCLUDED
#define LoRaBroker_H_INCLUDED

#include <iostream>
#include <vector>
#include "IBroker.h"
#include "../IoT/IoTUtility.h"
#include "../Network/TcpClient.h"
#include "../Utility/thread.h"

using namespace std;

//XBee broker use the api mode 2
class LoRaBroker:public IBroker
{
public:	
	LoRaBroker(string name, BrokerType type, string ip, int port, int comNumber, int baudrate);
	~LoRaBroker();

	void Start();
	void Stop();
	void ScanAllDevice();

private:
	
	enum RecvResult
	{
		RecvResult_Succeed,
		RecvResult_Timeout,
		RecvResult_Failed
	};

	class DeviceInfo
	{
	public:
		string IoTIp;
		string LoRaAddress;
		string DeviceDescription;
	};
			
	string cmdAddNode = "LoraJoinNode";
	string cmdRemoveNode = "LoraLeaveNode";
	string cmdSendData = "LoraNodeData";

	string cmdEndSymbol = "\r";

	string msgSyn = "SYN";
	string msgAck = "ACK";
	string selfIoTIp = "";
	string configFileSplitSymbol = "@";

	char startCode = 0x02;
	char endCode = 0x03;
	char enqCode = 'S';

	int loraResponseWaitTimeout = 5000; 
	int maxRecvBufferSize = 2000;
	int maxConfigFileSize = 50000;
	int maxRetryTimes = 3;
	int RS232PortNumaber;
	int RS232BaudRate;

	bool nowScanning;

	TcpClient *clientToManager;
	TSThread mainLoopThread;

	std::vector<DeviceInfo> LoRaDevices;


	//listener
	static void server_main_loop_entry(LoRaBroker *serverObj);
	int serverLoop();
	void handleReceivePackage(IoTPackage *package);
	void handleForwardingPackage(IoTPackage *package);
	void handleManagerPackage(IoTPackage *package);

	//Broker Manager function
	void brokerRegister();
	static void rescan_thread_entry(LoRaBroker *serverObj);

	//utility
	void LoRaBroker::initLoRaModule();
	void sendAtCommand(string cmd);
	RecvResult recvAtResponse(std::vector<char> *dataBuffer, unsigned long long msTimeout); //keep wait data from xbee for msTimeout

	void sendData(string addres, std::vector<char> *data);
	RecvResult recvData(string addres, std::vector<char> *data);	
	RecvResult getResponse(string address,std::vector<char> *recvBuffer);

	void loadLoRaConfigFile();

	void clearComportBuffer();

	int findStartCodeIndex(std::vector<char> *buffer);
	int findEndCodeIndex(std::vector<char> *buffer);
		
	//XBee network maintain
	bool addNode(string address);
	bool removeNode(string address);
	void clearEndNodeBuffer(string address);
	bool checkAlive(string address);
	int findDeviceIndex(string addr);


	/*	
	void handleReceivePackage(IoTPackage *package);
	void handleForwardingPackage(IoTPackage *package);
	void handleManagerPackage(IoTPackage *package);

	
	static void rescan_thread_entry(LoRaBroker *serverObj);
	
	//utility
	
	RecvResult recvResponse(vector<char> *recvBuffer, XBeeResponseType expectType);//receive a expect response type
	RecvResult getResponse(std::vector<char> *recvBuffer, int timeOut);//in common using, only for IoTRepublic rule(a request command bring two reponse: tx_response and rx_response)
	void sendData(XBeeAddress64 xbee_mac_addr, std::vector<char> *data);//send data to XBee
	
	
	int findStartCodeIndex(std::vector<char> *buffer);
	int findEndCodeIndex(std::vector<char> *buffer);
	unsigned int getCheckSum(std::vector<char> *buffer);
	

	
	void registerDevice(XBeeAddress64 addr);
	std::vector<char> askDeviceDescription(XBeeAddress64 addr);		
	int findDeviceIndex(XBeeAddress64 addr);
	int findDeviceIndex(string iotIp);
	void addNewDevice(XBeeAddress64 addr, string IoTIp);
	*/

};

#endif