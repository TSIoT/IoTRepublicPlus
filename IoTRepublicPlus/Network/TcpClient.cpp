#include "TcpClient.h"
#include <string>


//Public methods
TcpClient::TcpClient(string targetIp, int port, int maxRecvSize)// constructor
{
	this->targetIp = targetIp;
    this->serverPort=port;
    this->maxReceiveBuffer= maxRecvSize;
	this->IsConnected = 0;

    cout << "TcpClient(int port,int maxSize) Invoked" << endl;
    this->initClient();
}

TcpClient::~TcpClient() //destructor
{
    cout <<"TCP Closed!"<<endl;
}


NetworkError TcpClient::Connect()
{
	NetworkError errorCode = NetworkError_NoError;

	struct sockaddr_in address;
	address.sin_addr.s_addr = inet_addr(this->targetIp.c_str());
	address.sin_family = AF_INET;
	address.sin_port = htons(this->serverPort);

	if (connect(this->clientSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		puts("Tcp client connect error");
		errorCode = NetworkError_ConnectError;
	}
	else
	{
		this->IsConnected = 1;
	}

	return errorCode;
}

void TcpClient::Disconnect()
{
	CloseTSSocket(this->clientSocket);
}


void TcpClient::SendData(char* buffer, int length)
{
    if(this->IsConnected)
    {
        send(this->clientSocket,buffer,length,0);
    }
	else
	{
		cout << "Tcp connection is not connected" << endl;
	}
}


void TcpClient::SendData(string buffer)
{
    if(this->IsConnected)
    {
		send(this->clientSocket, buffer.c_str(), buffer.length(), 0);
    }
}


void TcpClient::StartListen()
{
	cout << "Tcp client start listen" << endl;
    Thread_create(&this->clientThread, (TSThreadProc)TcpClient::client_main_loop_entry, this);
	Thread_run(&this->clientThread);
}

void TcpClient::StopListen()
{
	cout << "Tcp client stop listen" << endl;
	Thread_stop(&this->clientThread);
	Thread_kill(&this->clientThread);

}

//Protected methods



//Private method
void TcpClient::initClient()
{
#if defined(WIN32)
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("TcpClient init Failed. Error Code : %d", WSAGetLastError());
		PAUSE;		
	}
#endif

	if ((this->clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("TcpClient Could not create socket\n");
		PAUSE;
	}
}

void TcpClient::client_main_loop_entry(TcpClient *clientObj)
{
	clientObj->client_loop();
}

void TcpClient::client_loop()
{
	int n = 0;
	char *recvBuffer = new char[this->maxReceiveBuffer];	

	while (1)
	{		
		n = recv(this->clientSocket, recvBuffer, this->maxReceiveBuffer, 0);
		if (n > 0)
		{
			this->Event_ReceivedData(recvBuffer, n);
		}
	}	
}


//Events for override by child class
void TcpClient::Event_ReceivedData(char* data, int dataLength)
{
	data[dataLength] = '\0';
	string msg(data);
	cout << "Tcp client received data handler!" << endl;
	cout << "Data:" << msg << endl;		
}