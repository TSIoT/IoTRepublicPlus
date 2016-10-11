#ifndef FakeBroker_H_INCLUDED
#define FakeBroker_H_INCLUDED
#include <iostream>

#include "../Utility/thread.h"
#include "IBroker.h"
#include "../IoT/IoTUtility.h"
#include "../IoT/IoTCommand.h"
#include "../Network/TcpClient.h"
#include "../Utility/thread.h"

#include "../IoT/IoTRule.h"


using namespace std;

class FakeBroker:public IBroker
{
public:
	FakeBroker(string name, BrokerType type, string ip, int port);
	~FakeBroker();

	void Start();
	void Stop();	

private:
	TcpClient *client;	

	TSThread listenThread;

	int maxRecvSize = 2000;

	static void faker_main_loop_entry(FakeBroker *Obj);
	void loop();
	void registerAllFakeDevice();
};

#endif

