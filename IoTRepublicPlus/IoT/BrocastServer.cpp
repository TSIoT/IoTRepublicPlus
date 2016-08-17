#include "BrocastServer.h"


BrocastServer::BrocastServer(int port, int maxSize):UdpServer(port, maxSize)
{

}


BrocastServer::~BrocastServer()
{

}


void BrocastServer::Event_ReceivedData(string ipAddress, int port, std::vector<char> *buffer, int dataLength)
{
	//cout << "Brocas server received data" << endl;
	string msg = "Hi";
	std::vector<char> sendData;
	sendData.insert(sendData.end(),msg.begin(), msg.end());
	this->SendData(ipAddress, port, &sendData);
}