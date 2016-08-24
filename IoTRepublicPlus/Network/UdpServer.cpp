#include "UdpServer.h"
#include <string.h>

UdpServer::UdpServer(int port, int maxSize)
{
	this->serverPort = port;
	this->maxReceiveBuffer = maxSize;
}

UdpServer::~UdpServer()
{

}

void UdpServer::StartServer()
{
	Thread_create(&this->serverThread, (TSThreadProc)UdpServer::server_main_loop_entry, this);
	Thread_run(&this->serverThread);
	//puts("UDP server started");
}

void UdpServer::StopServer()
{
	Thread_stop(&this->serverThread);
	Thread_kill(&this->serverThread);
	//puts("UDP server stoped");
}

void UdpServer::SendData(string ip, int port, std::vector<char> *sendData)
{	
	if (this->listener > 0)
	{
		struct sockaddr_in sockaddr;
		memset(&sockaddr, 0, sizeof(sockaddr));
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
		sockaddr.sin_port = htons(port);
		sendto(this->listener, &sendData->at(0), sendData->size(), 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	}	
}

void UdpServer::server_main_loop_entry(UdpServer *serverObj)
{
	serverObj->server_loop();
}

int UdpServer::server_loop()
{
	cout << "UDP Server started" << endl;
	cout << "Max receive buffer:" << this->maxReceiveBuffer << endl;
	cout << "Port:" << this->serverPort << endl;

	std::vector<char> buf(this->maxReceiveBuffer);

#if defined(WIN32)
	WSADATA wsa;
	//printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		cout << "UDP server, WSADATA init Failed. Error Code :"<< WSAGetLastError() << endl;		
		return -1;
	}
	//printf("Initialised.\n");
#endif

	this->listener = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (this->listener <= 0)
	{
		cout << "Broadcast server Error: listenForPackets - socket() failed." << endl;		
		return -1;
	}

	// bind the port
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));	
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);	
	sockaddr.sin_port = htons(this->serverPort);

	int bBroadcast = 1;
	if (setsockopt(this->listener, SOL_SOCKET, SO_BROADCAST, (const char*)&bBroadcast, sizeof(int)) == -1)
	{		
		cout << "UDP Server socket set failed" << endl;
	}

	int status = bind(this->listener, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	if (status == -1)
	{
		cout << "Broadcast server Error: listenForPackets - bind() failed." << endl;
		CloseTSSocket(this->listener);	
		return -1;
	}

	// receive
	struct sockaddr_in receiveSockaddr;
#if defined(WIN32)
	int receiveSockaddrLen = sizeof(receiveSockaddr);
#elif defined(__linux__) || defined(__FreeBSD__)
	socklen_t receiveSockaddrLen = sizeof(receiveSockaddr);
#endif

	cout << "Broad cast server started" << endl;;
	int n = 0;
	while (1)
	{
		n = recvfrom(this->listener, 
			&buf.at(0), 
			this->maxReceiveBuffer, 
			0, 
			(struct sockaddr *)&receiveSockaddr,
			&receiveSockaddrLen);

		if (n > 0)
		{			
			string ip=inet_ntoa(receiveSockaddr.sin_addr);
			int port = ntohs(receiveSockaddr.sin_port);
			this->Event_ReceivedData(ip,port, &buf,n);

			//this->SendData(ip,port, &buf);

			//ms_sleep(1000);
			/*
			if (sendto(this->listener, sendData, strlen(sendData), 0, (struct sockaddr *)&receiveSockaddr, sizeof(receiveSockaddr))<0)
			{
				puts("Broadcast server Send error");
			}
			else
			{
				puts("Broadcast server Response sended");
			}
			*/
		}
		
	}

	return 0;
}

void UdpServer::Event_ReceivedData(string ipAddress, int port, std::vector<char> *buffer, int dataLength)
{
	cout << "Ip:" << ipAddress.c_str() << ",Port:" << port << endl;
}