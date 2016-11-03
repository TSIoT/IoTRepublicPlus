#ifndef CloudBridge_H_INCLUDED
#define CloudBridge_H_INCLUDED

#include <iostream>
#include "../Network/TcpClient.h"
#include "../Utility/NetworkUtility.h"
#include "../Utility/thread.h"
#include "IBroker.h"

using namespace std;


class CloudBridge:public IBroker
{
public:
	CloudBridge(string name, BrokerType type, string managerIp, int managerPort,string targetIp, int port, int maxRecvSize);
	~CloudBridge();
	
	bool IsLoggeed;

	NetworkError Login(string id, string password);
	void Start();
	void Stop();	
	
	void ScanAllDevice();
	
private:	
	TcpClient *socketToCloud;
	TcpClient *socketToManager;
	TSThread cloudThread;
	TSThread managerThread;
	TSThread keepAliveThread;
	static TSMutex mutexLock;


	//string managerIp = "127.0.0.1";
	//int managerPort = 6210;
	int maxRecvSize;
	int keepAliveFrequency = 5 * 60 * 1000; //5 minite send a keep empty msg

	enum LoginError
	{
		LoginError_NoError,
		LoginError_UnknownError,
		LoginError_IdError,
		LoginError_PasswordError,
		LoginError_Timeout
	};


	LoginError checkLoginInfo(string id, string password);	
	//LoginError checkLogin(string id, string password);

	void startListener();
	void stopListener();
	static void managerSocketLoopEntry(CloudBridge *clientObj);
	static void cloudSocketLoopEntry(CloudBridge *clientObj);
	static void keepAliveLoopEntry(CloudBridge *clientObj);

	void managerLoop();
	void cloudLoop();
	void keepAliveLoop();

	bool reconnectToManager(std::vector<char> *dataVector);
};

#endif