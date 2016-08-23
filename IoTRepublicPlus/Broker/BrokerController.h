#ifndef BrokerController_H_INCLUDED
#define BrokerController_H_INCLUDED

#include <iostream>
#include <vector>
#include "IBroker.h"

using namespace std;

class BrokerController
{
public:
	BrokerController();
	~BrokerController();

	void AddBroker(IBroker *broker);
	void RemoveBroker(string brokerName);
	void ClearAllBroker();

	void StartAllBroker();
	void StopAllBroker();

	void StartBroker(string brokerName);
	void StopBroker(string brokerName);
	

private:
	vector<IBroker*> brokerList;


};




#endif