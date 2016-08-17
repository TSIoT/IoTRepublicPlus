#ifndef UDPSERVER_H_INCLUDED
#define UDPSERVER_H_INCLUDED
using namespace std;
#include <iostream>
#include <vector>

#include "../Utility/thread.h"
#include "../Utility/NetworkUtility.h"


class UdpServer
{
public:
	UdpServer(int port, int maxSize);
	~UdpServer();

	void StartServer();
	void StopServer();
	void SendData(string ip, int port, std::vector<char> *sendData);

protected:
	
	int serverPort, maxReceiveBuffer;
		
	/*void StopServer();	
	void SendData(string targetIp, std::vector<char> *buffer);*/
	

private:
	TSSocket listener;	
	TSThread serverThread;	

	static void server_main_loop_entry(UdpServer *serverObj);
	int server_loop();

	virtual void Event_ReceivedData(string ipAddress, int port, std::vector<char> *buffer, int dataLength);
	
};



#endif