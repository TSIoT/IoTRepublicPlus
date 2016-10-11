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
#include "Broker/FakeBroker.h"
#include "Broker/CloudUploader.h"


using namespace std;

//#define BrocastServer_
#define IoTManager_

#define Broker
//#define XBeeBroker_api
//#define FakeBroker_
#define CloudBridge_
//#define CloudUploader_


int main()
{	
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
	manager.StartRuler(1000);

	ms_sleep(100);
#endif

#ifdef Broker
	//Broker
	string managerIp("127.0.0.1");	
	BrokerController brokerController;
	string cloudLoginId = "DDR";
	string cloudLoginPw = "AAAAA";
	
	
	#ifdef XBeeBroker_api
		//XBee broker
		int comNumber = 13;
		int baudRate = 9600;
		IBroker *xbeeBroker = new XBeeBroker_ApiMode("XBeeApi", IBroker::BrokerType_XBeeApiMode, 
			managerIp, managerPort, 
			comNumber, baudRate);
		brokerController.AddBroker(xbeeBroker);
	#endif

	#ifdef FakeBroker_	
		IBroker *fakeBroker = new FakeBroker("FakeBroker", IBroker::BrokerType_XBeeApiMode,
			managerIp, managerPort);
		brokerController.AddBroker(fakeBroker);
	#endif
	
	#ifdef CloudBridge_
		int cloudServerPort = 6210;
		//string cloudServerIp = "104.199.183.220";
		string cloudServerIp = "192.168.156.233";
		//string cloudServerIp = "192.168.0.134";
		CloudBridge cloudBroker(cloudServerIp, cloudServerPort, maxReceiveBuffer, 
			"CloudBridge", IBroker::BrokerType_CloudBridge);

		NetworkError error = cloudBroker.Login(cloudLoginId, cloudLoginPw);
		if (error == NetworkError_NoError)
		{
			brokerController.AddBroker((IBroker*)&cloudBroker);
		}
		
		//cout << "Cloud Broker login result:" << error << endl;
	#endif

	#ifdef CloudUploader_	
		string webServerUrl = "http://104.199.183.220/AddDailyData.php";

		IBroker *uploader = new CloudUploader(webServerUrl, cloudLoginId, cloudLoginPw,"CloudUploader", IBroker::BrokerType_CloudUploader,managerIp,managerPort);
			
		brokerController.AddBroker(uploader);
	#endif

		
	brokerController.StartAllBroker();

#endif

/*
#ifdef CloudBridge_
	CloudBridge cloudBroker("104.199.183.220", managerPort, maxReceiveBuffer);
	cloudBroker.Login("DDR", "AAA");
#endif
*/
	PAUSE;
	
#ifdef Broker
	brokerController.StopAllBroker();
#endif

#ifdef IoTManager_
	manager.StopRuler();
	manager.StopManager();
#endif

#ifdef BrocastServer_
	brocastServer.StopServer();	
#endif
	
	return 0;
}

