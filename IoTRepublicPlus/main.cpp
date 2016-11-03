
#include <fstream>
#include <iterator>

#include "main.h"
#include "IoT/IoTManager.h"
#include "IoT/BrocastServer.h"
#include "Utility/SystemUtility.h"

#include "Utility/JsonUtility.h"
#include "Utility/file.h"
#include <direct.h> //_getcwd()


#include "Broker/CloudBridge.h"
#include "Broker/IBroker.h"
#include "Broker/XBeeBroker_ApiMode.h"
#include "Broker/BrokerController.h"
#include "Broker/FakeBroker.h"
#include "Broker/CloudUploader.h"

#include <stdlib.h>
#include <stdint.h>

#define LoadConfigFile_


//if allow LoadConfigFile_ config file, all define below will be useless
#define BrocastServer_
#define IoTManager_

//#define Broker
//#define XBeeBroker_api
//#define FakeBroker_
//#define CloudBridge_
//#define CloudUploader_


#ifdef LoadConfigFile_

void LoadConfigFile(string path, ManagerConfig *config)
{
	string readStr;
	std::vector<char> readBuffer;
	if (!ReadTextFileToVector(path, &readBuffer))
	{
		return;
	}
	json_t *root = JsonUtility::LoadJsonData(&readBuffer);

	config->BrocastServer= std::stoi(JsonUtility::GetValueInFirstObject(root, "BrocastServer"));
	config->IoTManager = std::stoi(JsonUtility::GetValueInFirstObject(root, "IoTManager"));
	config->XBeeBroker = std::stoi(JsonUtility::GetValueInFirstObject(root, "XBeeBroker"));
	config->FakeBroker = std::stoi(JsonUtility::GetValueInFirstObject(root, "FakeBroker"));
	config->CloudBridge = std::stoi(JsonUtility::GetValueInFirstObject(root, "CloudBridge"));
	config->CloudUploader = std::stoi(JsonUtility::GetValueInFirstObject(root, "CloudUploader"));
	
	config->ManagerPort = std::stoi(JsonUtility::GetValueInFirstObject(root, "ManagerPort"));	
	config->BrocastPort = std::stoi(JsonUtility::GetValueInFirstObject(root, "BrocastPort"));	
	config->MaxReceiveBuffer = std::stoi(JsonUtility::GetValueInFirstObject(root, "MaxReceiveBuffer"));	
	config->MaxClientOfManager = std::stoi(JsonUtility::GetValueInFirstObject(root, "MaxClientOfManager"));

	config->RulerActionFrequency = std::stoi(JsonUtility::GetValueInFirstObject(root, "RulerActionFrequency"));

	config->ManagerIp= JsonUtility::GetValueInFirstObject(root, "ManagerIp");
	config->CloudLoginId = JsonUtility::GetValueInFirstObject(root, "CloudLoginId");
	config->CloudLoginPw = JsonUtility::GetValueInFirstObject(root, "CloudLoginPw");

	config->XBeeBrokerName= JsonUtility::GetValueInFirstObject(root, "XBeeBrokerName");
	config->XBeeComNumber = std::stoi(JsonUtility::GetValueInFirstObject(root, "XBeeComNumber"));
	config->XBeeBaudRate = std::stoi(JsonUtility::GetValueInFirstObject(root, "XBeeBaudRate"));

	config->CloudBridgeName= JsonUtility::GetValueInFirstObject(root, "CloudBridgeName");
	config->CloudServerIp = JsonUtility::GetValueInFirstObject(root, "CloudServerIp");
	config->CloudServerPort = std::stoi(JsonUtility::GetValueInFirstObject(root, "CloudServerPort"));

	config->CloudUploaderName = JsonUtility::GetValueInFirstObject(root, "CloudUploaderName");
	config->WebServerUrl = JsonUtility::GetValueInFirstObject(root, "WebServerUrl");
}

/*
char encoding_table[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
'w', 'x', 'y', 'z', '0', '1', '2', '3',
'4', '5', '6', '7', '8', '9', '+', '/' };

int mod_table[] = { 0, 2, 1 };

void base64_encode(unsigned char *input, int input_length, unsigned char *outputBuffer, int *output_length)
{
	*output_length = 4 * ((input_length + 2) / 3);
	//char *inputBuffer = (char*)malloc(*output_length);
	if (outputBuffer == NULL) return;

	for (int i = 0, j = 0; i < input_length;) 
	{
		uint32_t octet_a = i < input_length ? (unsigned char)input[i++] : 0;
		uint32_t octet_b = i < input_length ? (unsigned char)input[i++] : 0;
		uint32_t octet_c = i < input_length ? (unsigned char)input[i++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		outputBuffer[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		outputBuffer[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		outputBuffer[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		outputBuffer[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	for (int i = 0; i < mod_table[input_length % 3]; i++)
		outputBuffer[*output_length - 1 - i] = '=';
	
}

char* base64_decode(char *data, int input_length, char *outputBuffer, int *output_length)
{
	char decoding_table[256] = {0};
	for (int i = 0; i < 64; i++)
		decoding_table[encoding_table[i]] = i;

	//if (decoding_table == NULL) build_decoding_table();

	if (input_length % 4 != 0) return NULL;

	*output_length = input_length / 4 * 3;
	if (data[input_length - 1] == '=') (*output_length)--;
	if (data[input_length - 2] == '=') (*output_length)--;

	//char *outputBuffer = (char *)malloc(*output_length);
	if (outputBuffer == NULL) return NULL;

	for (int i = 0, j = 0; i < input_length;) 
	{
		uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

		uint32_t triple = (sextet_a << 3 * 6)
			+ (sextet_b << 2 * 6)
			+ (sextet_c << 1 * 6)
			+ (sextet_d << 0 * 6);

		if (j < *output_length) outputBuffer[j++] = (triple >> 2 * 8) & 0xFF;
		if (j < *output_length) outputBuffer[j++] = (triple >> 1 * 8) & 0xFF;
		if (j < *output_length) outputBuffer[j++] = (triple >> 0 * 8) & 0xFF;
	}

	return outputBuffer;
}
*/


int main()
{
	/*
	char myTest[5] = {0x02,'S','Y','N',0X03};
	char outBuf[50] = {0};
	char outBuf2[50] = { 0 };
	int resultLen = 0;
	int resultLen2 = 0;

	resultLen=Base64encode(outBuf, myTest, 5);
	resultLen2 = Base64decode(outBuf2, outBuf);
	*/

	BrokerController brokerController;
	ManagerConfig config;
	IoTManager *manager=NULL;
	BrocastServer *brocastServer=NULL;

	LoadConfigFile("../Debug/manager.config", &config);

	if (config.BrocastServer)
	{
		brocastServer=new  BrocastServer(config.BrocastPort, config.MaxReceiveBuffer);
		brocastServer->StartServer();
		ms_sleep(100);
	}

	if (config.IoTManager)
	{
		manager=new  IoTManager(config.ManagerPort, config.MaxReceiveBuffer, config.MaxClientOfManager);
		manager->StartManager();
		manager->StartRuler(config.RulerActionFrequency);
		ms_sleep(100);
	}

	if (config.XBeeBroker)
	{
		IBroker *xbeeBroker = new XBeeBroker_ApiMode("XBeeApi", IBroker::BrokerType_XBeeApiMode,
			config.ManagerIp, config.ManagerPort,
			config.XBeeComNumber, config.XBeeBaudRate);
		brokerController.AddBroker(xbeeBroker);
	}

	if (config.FakeBroker)
	{
		IBroker *fakeBroker = new FakeBroker("FakeBroker", IBroker::BrokerType_XBeeApiMode,
			config.ManagerIp, config.ManagerPort);
		brokerController.AddBroker(fakeBroker);
	}

	if (config.CloudBridge)
	{		
		CloudBridge *cloudBroker=new CloudBridge("CloudBridge", IBroker::BrokerType_CloudBridge,
			config.ManagerIp, config.ManagerPort,
			config.CloudServerIp, config.CloudServerPort, config.MaxReceiveBuffer);

		NetworkError error = cloudBroker->Login(config.CloudLoginId, config.CloudLoginPw);
		if (error == NetworkError_NoError)
		{
			//brokerController.AddBroker((IBroker*)&cloudBroker);
			brokerController.AddBroker(cloudBroker);
		}		
	}

	if (config.CloudUploader)
	{		
		IBroker *uploader = new CloudUploader(config.WebServerUrl,
			config.CloudLoginId, config.CloudLoginPw, "CloudUploader", 
			IBroker::BrokerType_CloudUploader, config.ManagerIp, config.ManagerPort);

		brokerController.AddBroker(uploader);
	}

	brokerController.StartAllBroker();

	PAUSE

	brokerController.StopAllBroker();	

	if (manager!=NULL && config.IoTManager)
	{
		manager->StopRuler();
		manager->StopManager();
	}

	if (brocastServer!=NULL && config.BrocastServer)
	{
		brocastServer->StopServer();
	}

}

#else
int main()
{

	//LoadConfigFile("manager.config");

	int managerPort = 6210;
	int brocastPort = 6215;
	int maxReceiveBuffer = 2000;
	int maxClientOfManager = 500;

#ifdef BrocastServer_
	BrocastServer brocastServer(brocastPort, maxReceiveBuffer);
	brocastServer.StartServer();
	ms_sleep(100);
#endif

#ifdef IoTManager_
	IoTManager manager(managerPort, maxReceiveBuffer, maxClientOfManager);
	manager.StartManager();
	manager.StartRuler(1000);

	ms_sleep(100);
#endif

#ifdef Broker
	//Broker
	string managerIp("127.0.0.1");
	BrokerController brokerController;
	string cloudLoginId = "DDR";
	string cloudLoginPw = "AAAAA";


#ifdef XBeeBroker_api
	//XBee broker
	int comNumber = 20;
	int baudRate = 9600;
	IBroker *xbeeBroker = new XBeeBroker_ApiMode("XBeeApi", IBroker::BrokerType_XBeeApiMode,
		managerIp, managerPort,
		comNumber, baudRate);
	brokerController.AddBroker(xbeeBroker);
#endif

#ifdef FakeBroker_	
	IBroker *fakeBroker = new FakeBroker("FakeBroker", IBroker::BrokerType_XBeeApiMode,
		managerIp, managerPort);
	brokerController.AddBroker(fakeBroker);
#endif

#ifdef CloudBridge_
	int cloudServerPort = 6210;
	//string cloudServerIp = "104.199.183.220";
	string cloudServerIp = "192.168.156.233";
	//string cloudServerIp = "192.168.0.134";
	CloudBridge cloudBroker(cloudServerIp, cloudServerPort, maxReceiveBuffer,
		"CloudBridge", IBroker::BrokerType_CloudBridge);

	NetworkError error = cloudBroker.Login(cloudLoginId, cloudLoginPw);
	if (error == NetworkError_NoError)
	{
		brokerController.AddBroker((IBroker*)&cloudBroker);
	}

	//cout << "Cloud Broker login result:" << error << endl;
#endif

#ifdef CloudUploader_	
	string webServerUrl = "http://104.199.183.220/AddDailyData.php";

	IBroker *uploader = new CloudUploader(webServerUrl, cloudLoginId, cloudLoginPw, "CloudUploader", IBroker::BrokerType_CloudUploader, managerIp, managerPort);

	brokerController.AddBroker(uploader);
#endif


	brokerController.StartAllBroker();

#endif

	PAUSE;

#ifdef Broker
	brokerController.StopAllBroker();
#endif

#ifdef IoTManager_
	manager.StopRuler();
	manager.StopManager();
#endif

#ifdef BrocastServer_
	brocastServer.StopServer();
#endif

	return 0;
}
#endif



