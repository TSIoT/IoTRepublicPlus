#pragma once
#include <iostream>
#include <stdlib.h>
using namespace std;

class ManagerConfig
{
public:
	int BrocastServer=0;
	int IoTManager=0;
	int XBeeBroker=0;
	int FakeBroker=0;
	int CloudBridge=0;
	int CloudUploader=0;

	int ManagerPort=0;
	int BrocastPort=0;
	int MaxReceiveBuffer=0;
	int MaxClientOfManager=0;
	int RulerActionFrequency = 0;

	string ManagerIp="";
	string CloudLoginId="";
	string CloudLoginPw="";

	string XBeeBrokerName="";
	int XBeeComNumber = 0;
	int XBeeBaudRate = 0;

	string CloudBridgeName = "";
	string CloudServerIp = "";
	int CloudServerPort = 0;

	string CloudUploaderName="";
	string WebServerUrl = "";


};
