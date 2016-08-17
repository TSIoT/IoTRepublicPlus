#include <iostream>
#include <stdlib.h>
#include "IoT/IoTManager.h"
#include "IoT/BrocastServer.h"
#include "Utility/SystemUtility.h"

#include "Utility/JsonUtility.h"
#include "Broker/CloudBridge.h"



using namespace std;

#define ServerPort 6210
#define MaxRecvBuf 2000
#define MaxClient 500

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
	std::vector<int> myvector;

	// set some values (from 1 to 10)
	for (int i = 1; i <= 10; i++) myvector.push_back(i);

	// erase the 6th element
	myvector.erase(myvector.begin() + 4);

	// erase the first 3 elements:
	//myvector.erase(myvector.begin()+3, myvector.begin() + 3);

	std::cout << "myvector contains:";
	for (unsigned i = 0; i<myvector.size(); ++i)
		std::cout << ' ' << myvector[i];
	std::cout << '\n';

	PAUSE;
	*/
	
	/*
	TcpClient client("104.199.183.220", 6210, 2000);
	//TcpClient client("192.168.156.199", 6210, 2000);
	client.Connect();
	PAUSE;
	client.Disconnect();
	*/

	BrocastServer brocastServer(6215, MaxRecvBuf);
	brocastServer.StartServer();
	//PAUSE;
	//brocastServer.StopServer();
	//PAUSE;

	
	IoTManager manager(ServerPort, MaxRecvBuf, MaxClient);
	manager.StartManager();
	
	//CloudBridge cloudBroker("192.168.156.199", ServerPort, MaxRecvBuf);
	CloudBridge cloudBroker("104.199.183.220", ServerPort, MaxRecvBuf);
	NetworkError error = cloudBroker.Login("DDR", "AAA");
	cout << "Cloud Broker login result:" << error << endl;

	PAUSE;	
	cloudBroker.Logout();	
	manager.StopManager();	
	
	return 0;
}
