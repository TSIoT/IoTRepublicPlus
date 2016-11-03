#include "CloudUploader.h"
#include <sstream>
#if defined(WIN32)
#pragma comment(lib,"curl/libcurl.lib")
#include "../curl/curl.h"

#elif defined(__linux__)
#include <curl/curl.h>

#endif

//public
CloudUploader::CloudUploader(string url, string loginId, string loginPw, 
	string name, BrokerType type, string ip, int port):IBroker(name, type, ip, port)
{
	this->webServerAddress = url;
	this->loginId = loginId;
	this->loginPw = loginPw;
	this->client = new TcpClient(IBroker::ManagerIp, IBroker::ManagerServerPort, this->maxRecvSize);
	this->ioTUtility=new IoTUtility(IoTPackage::ProtocolVersion, IoTPackage::SegmentSymbol);
	this->allDevices.reserve(50);

	//Load subscript information
	this->subscriptObjs.reserve(50);
	
	SubscriptObj *subObj = NULL;

	subObj = new SubscriptObj("arduino.grove.mois01", "mois", 10*60*1000);
	this->subscriptObjs.push_back(*subObj);

	subObj=new SubscriptObj("arduino.grove.dl01", "lux", 10 * 60 * 1000);
	this->subscriptObjs.push_back(*subObj);

	subObj = new SubscriptObj("arduino.grove.pro.ths01", "humi", 60 * 60 * 1000);
	this->subscriptObjs.push_back(*subObj);

	subObj = new SubscriptObj("arduino.grove.pro.ths01", "tempe", 60 * 60 * 1000);
	this->subscriptObjs.push_back(*subObj);	

	subObj = new SubscriptObj("arduino.grove.dl01", "lux", 60 * 1000);
	this->subscriptObjs.push_back(*subObj);
	
}

CloudUploader::~CloudUploader()
{
	
}

void CloudUploader::Start()
{	
	this->client->Connect();
	if (this->client->IsConnected)
	{
		//this->uploadData("arduino.grove.pro.ths01.tempe","20");
		
		cout << "CloudUploader start!" << endl;
		//need register
		this->brokerRegister();		

		this->getAllDeviceDescription();
		this->sendSubscriptRule();
		Thread_create(&this->listenThread, (TSThreadProc)CloudUploader::cloudUploder_main_loop_entry, this);
		Thread_run(&this->listenThread);
		
	}

}

void CloudUploader::Stop()
{	
	Thread_stop(&this->listenThread);
	Thread_kill(&this->listenThread);	
}


//private method

void CloudUploader::cloudUploder_main_loop_entry(CloudUploader *Obj)
{
	Obj->loop();
}

void CloudUploader::loop()
{
	std::vector<char> recvBuffer;	
	recvBuffer.reserve(this->maxRecvSize);
	std::vector<char> packageBuffer;
	packageBuffer.reserve(this->maxRecvSize);

	int n = 0;
	if (this->client->IsConnected)
	{
		while (1)
		{			
			n=this->client->RecviveData(&recvBuffer);
			if (n > 0)
			{
				//cout << "CloudUploader receive data" << endl;
				writeLog(&recvBuffer);
				packageBuffer.insert(packageBuffer.end(), recvBuffer.begin(), recvBuffer.end());
				this->handleRecvData(&packageBuffer);
				recvBuffer.clear();
			}
		}
	}
}

void CloudUploader::handleRecvData(std::vector<char> *packageBuffer)
{
	IoTPackage *package;
	IoTUtility::GetPackageError error;

	do
	{
		package = this->ioTUtility->GetCompletedPackage(packageBuffer, &error);
		if (package != NULL)
		{
			string commandJsonData(package->DataVector.begin(), package->DataVector.end());
			IoTCommand cmd(commandJsonData);
			string devId=this->getUploadDeviceId(package->SorIp,cmd.ID);
			this->uploadData(devId,cmd.Value);
			cout << "Receive from:" << package->SorIp << endl;
			cout << "Command id:" << cmd.ID << endl;
			cout << "Command Value:" << cmd.Value << endl;
			cout << "Upload id:" << devId << endl;
		}

	} while (error == IoTUtility::GetPackageError_NoError);		
}


//upload data to cloud server
size_t CloudUploader::recvWebCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
	CloudUploader *obj = static_cast<CloudUploader*>(userp);
	size_t realsize = size * nmemb;
	obj->readBuffer.append(contents, realsize);
	//readBuffer.append(contents, realsize);
	return realsize;
}

void CloudUploader::uploadData(string deviceId, string value)
{
	CURL *curl;
	CURLcode res;
	string request = string("deviceId=") + deviceId + string("&value=") + value
		+ string("&loginId=") + this->loginId + string("&password=") + this->loginPw;

	int findResult = -1;
	long readPageTimeOutSecond = 2L;

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	if (curl)
	{		
		curl_easy_setopt(curl, CURLOPT_URL, this->webServerAddress.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, this->recvWebCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, readPageTimeOutSecond);
		//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "ID=DDR&PW=AAA");
		res = curl_easy_perform(curl);

		if (res == CURLE_OK)
		{
			cout << this->readBuffer << endl;
		}
		else
		{
			cout << "curl_easy_perform() failed:" << curl_easy_strerror(res) << endl;;
		}
		this->readBuffer.clear();
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();

}


//Device list maintain
void CloudUploader::getAllDeviceDescription()
{	
	IoTCommand cmd(IoTCommand::command_t_Management, "Dis_All", "0");
	IoTPackage package(this->selfIoTIp, IBroker::managerIoTIp, cmd.sendedData.str());
	this->client->SendData(&package.DataVectorForSending);
	IoTPackage *descPackage = this->waitIoTPackage(1000);
	if (descPackage != NULL)
	{
		this->saveAllDeviceDescription(descPackage);				
	}	
}

void CloudUploader::saveAllDeviceDescription(IoTPackage *package)
{
	std::vector<char> *rawData = &package->DataVector;	
	int dataLength = package->DataVector.size();
	this->allDevices.clear();

	//Place the raw data into device info
	stringstream  strBuilder;
	int objCount = 0;

	IoTDevice *dInfo=new IoTDevice();;

	for (int i = 0; i < dataLength; i++)
	{
		if (rawData->at(i) == '\0')
		{
			if (objCount % 2 == 0)
			{
				dInfo->IoTIP = strBuilder.str();
			}
			else if (objCount % 2 == 1)
			{
				dInfo->SetJSDescription(strBuilder.str());
				
				this->allDevices.push_back(*dInfo);
				dInfo = new IoTDevice();
			}

			strBuilder.str("");
			strBuilder.clear();			
			objCount++;
		}
		else
		{
			strBuilder << rawData->at(i);
		}
	}
}

int CloudUploader::findComponentIndex(string devId, string comId)
{
	int resultIndex = -1;

	int deviceCount = this->allDevices.size();
	int deviceIndex = -1;
	for (int i = 0; i < deviceCount; i++)
	{
		if (this->allDevices.at(i).DeviceID == devId)
		{
			deviceIndex = i;
			break;
		}
	}

	if (deviceIndex >= 0)
	{
		int comCount = this->allDevices.at(deviceIndex).Component.size();
		for (int i = 0; i < comCount; i++)
		{
			if (this->allDevices.at(deviceIndex).Component.at(i).ID == comId)
			{
				resultIndex = deviceIndex;
				break;
			}
		}
	}

	return resultIndex;
}
/*
bool CloudUploader::isComponentExists(string devId,string comId)
{
	bool isExists = 0;
	int deviceCount = this->allDevices.size();
	int deviceIndex = -1;
	for (int i = 0; i < deviceCount; i++)
	{
		if (this->allDevices.at(i).DeviceID == devId)
		{
			deviceIndex = i;
			break;
		}
	}

	if (deviceIndex >= 0)
	{
		int comCount = this->allDevices.at(deviceIndex).Component.size();
		for (int i = 0; i < comCount; i++)
		{
			if (this->allDevices.at(deviceIndex).Component.at(i).ID == comId)
			{
				isExists = 1;
				break;
			}
		}
	}

	return isExists;
}
*/
IoTPackage* CloudUploader::waitIoTPackage(int waitTime)
{
	//IoTUtility ioTUtility(IoTPackage::ProtocolVersion, IoTPackage::SegmentSymbol);
	unsigned long long startTime;
	std::vector<char> recvBuffer;
	recvBuffer.reserve(this->maxRecvSize);	

	IoTUtility::GetPackageError errorFromManager;	
	IoTPackage *package = NULL;
	
	startTime = get_millis();

	do
	{
		if (get_millis() - startTime > waitTime)
		{
			cout << "waitIoTPackage() Time out!" << endl;
			break;
		}

		this->client->RecviveData(&recvBuffer);
		package = this->ioTUtility->GetCompletedPackage(&recvBuffer, &errorFromManager);

	} while (errorFromManager == IoTUtility::GetPackageError_PackageNotCompleted);


	
	return package;
}

string CloudUploader::getUploadDeviceId(string iotIp,string comId)
{
	int daviceCount = this->allDevices.size();
	string deviceId="";
	for (int i = 0; i < daviceCount; i++)
	{
		if (this->allDevices.at(i).IoTIP == iotIp)
		{
			deviceId = this->allDevices.at(i).DeviceID+"."+ comId;
			break;
		}
	}

	return deviceId;
}

//rule handle
void CloudUploader::sendSubscriptRule()
{
	int subObjCount = this->subscriptObjs.size();

	for (int i = 0; i < subObjCount; i++)
	{
		string devId = this->subscriptObjs.at(i).DevId;
		string comId = this->subscriptObjs.at(i).ComId;
		int targetIndex = this->findComponentIndex(devId, comId);
		if (targetIndex >= 0)
		{
			IoTRule rule;
			rule.Type = "Sub";
			rule.Name = "Subscription";

			rule.ReadFrom = this->allDevices.at(targetIndex).IoTIP;
			rule.ReadID = this->subscriptObjs.at(i).ComId;
			rule.ReadValue = "";

			rule.WriteTo = this->selfIoTIp;
			rule.WriteID = this->subscriptObjs.at(i).ComId;
			rule.WriteValue = "";

			rule.ActionFrequency = this->subscriptObjs.at(i).ReadFrequency;
			rule.Packet();

			IoTPackage package(IBroker::protocolVersion, this->selfIoTIp, IBroker::managerIoTIp, rule.JsonData);

			this->client->SendData(&package.DataVectorForSending);
		}
	}
}


//Broker Manager function
void CloudUploader::brokerRegister()
{
	this->selfIoTIp = this->askIoTIp();
	string selfDescription = "{\"IOTDEV\":{\"DeviceName\":\"CloudUploader\",\"FunctionGroup\":\"Broker\",\"DeviceID\":\"iotrepublic.broker.clouduploader\",\"Component\":[]}}";
	std::vector<char> sendData(selfDescription.begin(), selfDescription.end());
	IoTPackage package(this->protocolVersion, this->selfIoTIp, this->managerIoTIp, &sendData);
	this->client->SendData(&package.DataVectorForSending);
}


