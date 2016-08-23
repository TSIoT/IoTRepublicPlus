#include "BrokerController.h"

BrokerController::BrokerController()
{
	this->brokerList.reserve(5);
}

BrokerController::~BrokerController()
{
}


void BrokerController::AddBroker(IBroker *broker)
{
	this->brokerList.push_back(broker);
}

void BrokerController::RemoveBroker(string brokerName)
{
	int brokerCount = this->brokerList.size();
	for (int i = 0; i < brokerCount; i++)
	{
		if (this->brokerList.at(i)->Name == brokerName)
		{
			delete this->brokerList.at(i);
			this->brokerList.erase(this->brokerList.begin() + i);
			break;
		}
	}
}

void BrokerController::ClearAllBroker()
{
	int brokerCount = this->brokerList.size();
	for (int i = 0; i < brokerCount; i++)
	{		
		this->brokerList.at(i)->Stop();
		delete this->brokerList.at(i);		
	}
	this->brokerList.erase(this->brokerList.begin(),this->brokerList.end());
}

void BrokerController::StartAllBroker()
{
	int brokerCount = this->brokerList.size();
	for (int i = 0; i < brokerCount; i++)
	{
		this->brokerList.at(i)->Start();
	}
	
}

void BrokerController::StopAllBroker()
{
	int brokerCount = this->brokerList.size();
	for (int i = 0; i < brokerCount; i++)
	{
		this->brokerList.at(i)->Stop();
	}
}

void BrokerController::StartBroker(string brokerName) 
{

}


void BrokerController::StopBroker(string brokerName)
{

}