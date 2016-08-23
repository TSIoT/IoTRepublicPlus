#include <iostream>
#include <stdlib.h>
#include "IoT/IoTManager.h"
#include "IoT/BrocastServer.h"
#include "Utility/SystemUtility.h"

#include "Utility/JsonUtility.h"


#include "Broker/CloudBridge.h"
#include "Broker/IBroker.h"
#include "Broker/XBeeBroker_ApiMode.h"
#include "Broker/BrokerController.h"


using namespace std;

//#define ActiveBrocastServer
#define ActiveIoTManager

#define ActiveBroker
#define ActiveXBeeBroker_api
//#define ActiveCloudBridge


int main()
{	
	/*
	JsonUtility json;
	//json.jsonTest();
	//json.ArrayTest();
	//json.CommandTest();
	//json.DumpTest();
	PAUSE;
	*/
	
	/*
	TcpClient client("104.199.183.220", 6210, 2000);
	//TcpClient client("192.168.156.199", 6210, 2000);
	client.Connect();
	PAUSE;
	client.Disconnect();
	*/

	int managerPort = 6210;
	int brocastPort = 6215;
	int maxReceiveBuffer = 2000;
	int maxClientOfManager = 500;

#ifdef ActiveBrocastServer
	BrocastServer brocastServer(brocastPort, maxReceiveBuffer);
	brocastServer.StartServer();	
	ms_sleep(100);
#endif

#ifdef ActiveIoTManager
	IoTManager manager(managerPort, maxReceiveBuffer, maxClientOfManager);
	manager.StartManager();
	ms_sleep(100);
#endif

#ifdef ActiveBroker
	//Broker
	string managerIp("127.0.0.1");	
	BrokerController brokerController;
	
	#ifdef ActiveXBeeBroker_api
		//XBee broker
		int comNumber = 28;
		int baudRate = 9600;
		IBroker *xbeeBroker = new XBeeBroker_ApiMode("XBeeApi", IBroker::BrokerType_XBeeApiMode, 
			managerIp, managerPort, 
			comNumber, baudRate);			
		brokerController.AddBroker(xbeeBroker);
	#endif
	
	#ifdef ActiveCloudBridge
		int cloudServerPort = 6210;
		string cloudServerIp = "104.199.183.220";
		CloudBridge cloudBroker(cloudServerIp, cloudServerPort, maxReceiveBuffer);
		NetworkError error = cloudBroker.Login("DDR", "AAA");
		cout << "Cloud Broker login result:" << error << endl;
	#endif

		
	brokerController.StartAllBroker();

#endif


	PAUSE;


#ifdef ActiveCloudBridge
	cloudBroker.Logout();
#endif

#ifdef ActiveIoTManager
	manager.StopManager();	
#endif

#ifdef ActiveBroker
	brokerController.StopAllBroker();
#endif
	
	return 0;
}
