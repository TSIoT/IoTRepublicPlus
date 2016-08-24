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

//#define BrocastServer_
#define IoTManager_

#define Broker
#define XBeeBroker_api
//#define CloudBridge_


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

#ifdef BrocastServer_
	BrocastServer brocastServer(brocastPort, maxReceiveBuffer);
	brocastServer.StartServer();	
	ms_sleep(100);
#endif

#ifdef IoTManager_
	IoTManager manager(managerPort, maxReceiveBuffer, maxClientOfManager);
	manager.StartManager();
	ms_sleep(100);
#endif

#ifdef Broker
	//Broker
	string managerIp("127.0.0.1");	
	BrokerController brokerController;
	
	#ifdef XBeeBroker_api
		//XBee broker
		int comNumber = 28;
		int baudRate = 9600;
		IBroker *xbeeBroker = new XBeeBroker_ApiMode("XBeeApi", IBroker::BrokerType_XBeeApiMode, 
			managerIp, managerPort, 
			comNumber, baudRate);			
		brokerController.AddBroker(xbeeBroker);
	#endif
	
	#ifdef CloudBridge
		int cloudServerPort = 6210;
		string cloudServerIp = "104.199.183.220";
		CloudBridge cloudBroker(cloudServerIp, cloudServerPort, maxReceiveBuffer);
		NetworkError error = cloudBroker.Login("DDR", "AAA");
		cout << "Cloud Broker login result:" << error << endl;
	#endif

		
	brokerController.StartAllBroker();

#endif

#ifdef CloudBridge_
	CloudBridge cloudBroker("104.199.183.220", managerPort, maxReceiveBuffer);
	cloudBroker.Login("DDR", "AAA");
#endif


	PAUSE;


#ifdef CloudBridge_
	cloudBroker.Logout();
#endif

#ifdef Broker
	brokerController.StopAllBroker();
#endif

#ifdef IoTManager_
	manager.StopManager();
#endif

#ifdef BrocastServer_
	brocastServer.StopServer();	
#endif
	
	return 0;
}
