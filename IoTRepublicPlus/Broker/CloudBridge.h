#ifndef CloudBridge_H_INCLUDED
#define CloudBridge_H_INCLUDED

#include <iostream>
#include "../Network/TcpClient.h"
#include "../Utility/NetworkUtility.h"
#include "../Utility/thread.h"

using namespace std;


class CloudBridge
{
public:
	CloudBridge(string targetIp, int port, int maxRecvSize);
	~CloudBridge();

	bool IsLoggeed;

	NetworkError Login(string id, string password);
	void Logout();
	
private:	
	TcpClient *socketToCloud;
	TcpClient *socketToManager;
	TSThread cloudThread;
	TSThread managerThread;

	string managerIp = "127.0.0.1";
	int managerPort = 6210;
	int maxRecvSize;

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

	void managerLoop();
	void cloudLoop();
};

#endif