#include "TcpServer.h"


//Public methods
TcpServer::TcpServer(int port,int maxSize, int maxSockets)// constructor
{
    this->serverPort=port;
    this->maxReceiveBuffer=maxSize;
    this->maxClient=maxSockets;


    cout << "TcpServer(int port,int maxSize, int maxSockets) Invoked" << endl;
    this->initServer();
}

TcpServer::~TcpServer() //destructor
{
    cout <<"TCP Closed!"<<endl;
}


//Protected methods
void TcpServer::StartServer()
{
    cout << "Tcp server start" << endl;
    Thread_create(&this->serverThread, (TSThreadProc)TcpServer::server_main_loop_entry, this);
	Thread_run(&this->serverThread);
}

void TcpServer::StopServer()
{
    cout << "Tcp server Stop" << endl;
	Thread_stop(&this->serverThread);
	Thread_kill(&this->serverThread);
    this->uninit();
    
}

TSSocket TcpServer::GetSocket(int socketIndex)
{
    return this->clientSockets[socketIndex];
}

void TcpServer::CutConnection(int socketIndex)
{
    FD_CLR(this->clientSockets[socketIndex], &this->master);
    CloseTSSocket(this->clientSockets[socketIndex]);
    this->clientSockets[socketIndex] = 0;

    this->Event_ConnectionDenied(socketIndex);

}

void TcpServer::SendDataToExistsConnection(int socketIndex,char* buffer, int length)
{
    if(this->clientSockets[socketIndex]>0)
    {
        send(this->clientSockets[socketIndex],buffer,length,0);
    }

}

void TcpServer::SendDataToExistsConnection(int socketIndex,string buffer)
{
    if(this->clientSockets[socketIndex]>0)
    {
        //char *sendBuffer=new char[buffer.length()];		
        //std::copy(buffer.begin(),buffer.end(),sendBuffer);
        //send(this->clientSockets[socketIndex],sendBuffer,buffer.length(),0);

		send(this->clientSockets[socketIndex], buffer.c_str(), buffer.length(), 0);

        //delete[] sendBuffer;
    }

}

//Private methods
NetworkError TcpServer::initServer()
{
    int i=0;
    //struct sockaddr_in address;

    this->clientSockets=new TSSocket[this->maxClient];

	for (i = 0; i < this->maxClient; i++)
	{
		this->clientSockets[i] = 0;
	}
	return NetworkError_NoError;
}

void TcpServer::uninit()
{

    CloseTSSocket(this->listener);
    delete[] this->clientSockets;

}

void TcpServer::server_main_loop_entry(TcpServer *serverObj)
{
    serverObj->server_loop();
}

int TcpServer::server_loop()
{
    cout << "TCP Server starting" << endl;
    cout << "Port:"<< this->serverPort << endl;
    cout << "Max Receive buffer:"<< this->maxReceiveBuffer << endl;
    cout << "Max Client number:"<< this->maxClient << endl;

#if defined(WIN32)
	WSADATA wsa;
	//printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Main server WSAStartup failed. Error Code : %d", WSAGetLastError());
		PAUSE;
		return -1;
	}
	//printf("Initialised.\n");
#endif
	
	int activity = 0, i = 0, valread = 0, on = 1;
	
	//int maxRecvBufSize = this->maxReceiveBuffer;
	char *buffer = new char[this->maxReceiveBuffer];
	//std::vector<char> buffer(maxRecvBufSize);
	
    // Accept a new connection need the variable
    TSSocket new_socket;
    struct sockaddr_in  address;

    //select server need the variable
    TimeSpan timeout;
    fd_set readfds;
	
    #if defined(WIN32)
        int addrlen = sizeof(struct sockaddr_in);
	#elif defined(__linux__) || defined(__FreeBSD__)
        socklen_t addrlen = sizeof(struct sockaddr_in);
    #endif

	//timeout.tv_sec = 15;
	//timeout.tv_usec = 0;

	timeout.tv_sec = 0;
	timeout.tv_usec = 100;
	
    if((this->listener=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        cout << "TCP server Could not create socket\n" <<endl;

		return NetworkError_SocketError;
    }

	#if defined(WIN32)
	if ((setsockopt(this->listener, SOL_SOCKET, SO_REUSEADDR,(char*) &on, sizeof(int)))<0)
	{
		cout << "setsockopt failed" << endl;
		return NetworkError_SocketError;
	}
	else
	{
		cout << "TCP Server Socket created" << endl;
	}
	#elif defined(__linux__) || defined(__FreeBSD__)
	if((setsockopt(this->listener,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int)))<0)
    {
        cout << "setsockopt failed" <<endl;
        return ErrorCode_SocketError;
    }
    else
    {
        cout << "TCP Server Socket created" << endl;
    }
	#endif


    //address.sin_family = AF_UNSPEC;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(this->serverPort);
	
	//Bind
	if (bind(this->listener, (struct sockaddr *)&address, sizeof(address)) == -1)
	{
	    cout << "TCP server Bind failed"<<endl;
        return NetworkError_SocketError;
	}
	else
    {
        cout <<"TCP Socket Binded"<<endl;
    }

    listen(this->listener, this->maxClient);

    FD_ZERO(&this->master);
	FD_ZERO(&readfds);

    FD_SET(this->listener, &this->master);

    while(1)
    {
        readfds = this->master;
		activity = select(this->maxClient, &readfds, NULL, NULL, &timeout);

		if (activity == -1)
		{
		    cout << "Main server select call failed" <<endl;
			break;
		}

		//Add new connection into set
		if (FD_ISSET(this->listener, &readfds))
		{
			if ((new_socket = accept(this->listener, (struct sockaddr *)&address, &addrlen))<0)
			{
			    cout << "Accept connection error" << endl;
				break;
			}

			//inform user of socket number - used in send and receive commands
			cout << "Main server has new connection , socket fd is" << new_socket
			<<" Ip:" << inet_ntoa(address.sin_addr)
			<<" Port:" <<ntohs(address.sin_port)<<endl;


			//add new socket to array of sockets
			for (i = 0; i < this->maxClient; i++)
			{
				if (this->clientSockets[i] == 0)
				{
					this->clientSockets[i] = new_socket;
					FD_SET(new_socket, &this->master);
					this->Event_NewConnection(i);
					cout << "Main server adding a new item to list of sockets at index:"<< i <<endl;

					break;
				}
			}
		}

		for (i = 0; i <this->maxClient; i++)
		{
			if (FD_ISSET(this->clientSockets[i], &readfds))
			{
			    //get details of the client
				getpeername(this->clientSockets[i], (struct sockaddr*)&address, &addrlen);
                //std::fill_n(buffer,this->maxReceiveBuffer,'\0');
				//buffer.erase(buffer.begin(), buffer.end());
				//std::fill_n(buffer.data(), this->maxReceiveBuffer, '\0');
				valread = recv(this->clientSockets[i], buffer, this->maxReceiveBuffer, 0);

				if (valread <= 0)
				{
					//Somebody disconnected , get his details and print
					cout << "Main server got host disconnected ip:"<< inet_ntoa(address.sin_addr)
					<< " Port:" << ntohs(address.sin_port) <<endl;

					this->CutConnection(i);
					//Close the socket and mark as 0 in list for reuse
					//this->Event_ConnectionDenied(i);
				}
				//Echo back the message that came in
				else if (valread>0)
				{
				    //cout << this->serverPort<<":";
				    //cout << string(buffer) <<endl;
				    this->Event_ReceivedData(i,buffer,valread);
				}				
			}
		}
    }
	
    return NetworkError_UnknownError;
}

//Events for override by child class
void TcpServer::Event_ReceivedData(int socketIndex, char* buffer, int dataLength)
{
    cout << "Tcp received data handler!" <<endl;
}

void TcpServer::Event_NewConnection(int socketIndex)
{
    cout << "Tcp Got new connection!" <<endl;
}

void TcpServer::Event_ConnectionDenied(int socketIndex)
{
    cout << "Connection denied" <<endl;
}
