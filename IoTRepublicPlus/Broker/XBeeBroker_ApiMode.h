#ifndef XBeeBroker_ApiMode_H_INCLUDED
#define XBeeBroker_ApiMode_H_INCLUDED

#include <iostream>
#include <vector>
#include "IBroker.h"
#include "XBee/XBeeApiLibrary.h"
#include "../IoT/IoTUtility.h"
#include "../Network/TcpClient.h"
#include "../Utility/thread.h"

using namespace std;

//XBee broker use the api mode 2
class XBeeBroker_ApiMode:public IBroker
{
public:	
	XBeeBroker_ApiMode(string name, BrokerType type, string ip, int port, int comNumber, int baudrate);
	~XBeeBroker_ApiMode();

	void Start();
	void Stop();
	void ScanAllDevice();
	void StartAutoRescan(int second);
	void StopAutoRescan();

private:
	enum XBeeResponseType
	{
		XBeeResponseType_Unknown=0x00,
		XBeeResponseType_AtCommand = 0x88,
		XBeeResponseType_TxStatus=0x8b,
		XBeeResponseType_RxStatus=0x90
	};

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
		XBeeAddress64 address;
	};
		
	string msgSyn = "SYN";
	string msgAck = "ACK";
	string msgDes = "DES";
	string selfIoTIp = "";

	char startCode = 0x02;
	char endCode = 0x03;

	int xbeePackageWaitTimeout = 500; 
	int maxRecvBufferSize = 2000;
	int RS232PortNumaber;
	int RS232BaudRate;

	bool nowScanning;

	XBee xbeeObj;
	TcpClient *clientToManager;
	TSThread mainLoopThread;

	std::vector<DeviceInfo> xbeeDevices;
	
	//listener
	static void server_main_loop_entry(XBeeBroker_ApiMode *serverObj);
	int serverLoop();
	void handleReceivePackage(IoTPackage *package);
	void handleForwardingPackage(IoTPackage *package);
	void handleManagerPackage(IoTPackage *package);

	//Broker Manager function
	void brokerRegister();
	static void rescan_thread_entry(XBeeBroker_ApiMode *serverObj);
	
	//utility
	void sendAtCommand(string cmd,string value);
	RecvResult recvResponse(vector<char> *recvBuffer, XBeeResponseType expectType);//receive a expect response type
	RecvResult getResponse(std::vector<char> *recvBuffer, int timeOut);//in common using, only for IoTRepublic rule(a request command bring two reponse: tx_response and rx_response)
	void sendData(XBeeAddress64 xbee_mac_addr, std::vector<char> *data);//send data to XBee
	RecvResult recvData(std::vector<char> *dataBuffer, int msTimeout); //keep wait data from xbee for msTimeout
	
	int findStartCodeIndex(std::vector<char> *buffer);
	int findEndCodeIndex(std::vector<char> *buffer);
	unsigned int getCheckSum(std::vector<char> *buffer);

	//XBee network maintain
	void registerDevice(XBeeAddress64 addr);
	std::vector<char> askDeviceDescription(XBeeAddress64 addr);
	
	//XBee device list maintain
	//bool isDeviceExists(XBeeAddress64 addr);
	int findDeviceIndex(XBeeAddress64 addr);
	int findDeviceIndex(string iotIp);
	void addNewDevice(XBeeAddress64 addr, string IoTIp);
};

#endif