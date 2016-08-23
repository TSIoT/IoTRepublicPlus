#ifndef Broadcast_H_INCLUDED
#define Broadcast_H_INCLUDED
#include <iostream>
#include <vector>
#include "../Network/UdpServer.h"

using namespace std;


class BrocastServer:public UdpServer
{
public :
	BrocastServer(int port, int maxSize);
	~BrocastServer();



private :
	void Event_ReceivedData(string ipAddress, int port, std::vector<char> *buffer, int dataLength);

};

#endif
