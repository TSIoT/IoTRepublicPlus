#include "FakeBroker.h"

FakeBroker::FakeBroker(string name, BrokerType type, string ip, int port) :IBroker(name, type, ip, port)
{
	this->client = new TcpClient(IBroker::ManagerIp, IBroker::ManagerServerPort, 2000);
}

FakeBroker::~FakeBroker()
{
	
}

void FakeBroker::Start()
{	
	this->client->Connect();
	if (this->client->IsConnected)
	{
		//cout << "Fake broker start!" << endl;
		this->registerAllFakeDevice();

		Thread_create(&this->listenThread, (TSThreadProc)FakeBroker::faker_main_loop_entry, this);
		Thread_run(&this->listenThread);
	}


	/*
	string iotIp = this->askIoTIp();

	string selfDescription = "{\"IOTDEV\":{\"DeviceName\":\"Fake Broker\",\"FunctionGroup\":\"Broker\",\"DeviceID\":\"iotrepublic.broker.faker\",\"Component\":[]}}";
	std::vector<char> sendData(selfDescription.begin(), selfDescription.end());
	IoTPackage registerPackage(IBroker::protocolVersion, iotIp, IBroker::managerIoTIp, &sendData);
	
	IoTRule rule;
	rule.Name = "Subscription";
	rule.ReadFrom = "DEV.0";
	rule.ReadID = "mosi";
	rule.ReadValue = "";
	
	rule.WriteTo = iotIp;
	rule.WriteID = "mosi";	
	rule.WriteValue = "";
	
	rule.ActionFrequency = 3000;
	rule.Packet();

	IoTPackage package(IBroker::protocolVersion, iotIp, IBroker::managerIoTIp, rule.JsonData);
	
	this->client->Connect();
	this->client->SendData(&registerPackage.DataVectorForSending);
	this->client->SendData(&package.DataVectorForSending);	

	Thread_create(&this->listenThread, (TSThreadProc)FakeBroker::faker_main_loop_entry, this);
	Thread_run(&this->listenThread);	
	*/
}

void FakeBroker::Stop()
{	
	Thread_stop(&this->listenThread);
	Thread_kill(&this->listenThread);	
}

void FakeBroker::faker_main_loop_entry(FakeBroker *Obj)
{
	Obj->loop();
}

void FakeBroker::loop()
{
	cout << "Fake broker start!" << endl;
	std::vector<char> recvBuffer;
	recvBuffer.reserve(this->maxRecvSize);

	IoTPackage *packageFromManager;
	IoTPackage *responsePackage;
	IoTCommand *recvCmd;
	IoTCommand *responseCmd;

	IoTUtility::GetPackageError errorFromManager;

	while (1)
	{
		this->client->RecviveData(&recvBuffer);

		if (recvBuffer.size() > 0)
		{			
			do
			{
				packageFromManager = this->ioTUtility->GetCompletedPackage(&recvBuffer, &errorFromManager);
				if (packageFromManager != NULL)
				{
					recvCmd = new IoTCommand(&packageFromManager->DataVector);
					
					if (recvCmd->CmdType == IoTCommand::command_t_ReadRequest)
					{
						int value_int = (rand() % 100);
						
						responseCmd = new IoTCommand(IoTCommand::command_t_ReadResponse, recvCmd->ID, std::to_string(value_int));
					}
					else
					{
						responseCmd = new IoTCommand(IoTCommand::command_t_ReadResponse, recvCmd->ID, "OK");
					}
										
					responsePackage = new IoTPackage(packageFromManager->DesIp, packageFromManager->SorIp, responseCmd->sendedData.str());
					this->client->SendData(&responsePackage->DataVectorForSending);

					delete recvCmd;
					delete responseCmd;
					delete responsePackage;
					delete packageFromManager;					
				}

			} while (errorFromManager == IoTUtility::GetPackageError_NoError);
		}

	}


}

void FakeBroker::registerAllFakeDevice()
{
	string iotIp = "";
	string deviceDesc = "";
	IoTPackage *package = NULL;
	
	iotIp = this->askIoTIp();
	deviceDesc = "{\"IOTDEV\":{\"DeviceName\":\"Moisture sensor\",\"FunctionGroup\":\"Sensor\",\"DeviceID\":\"arduino.grove.mois01\",\"Component\":[{\"ID\":\"mois\",\"Group\":\"1\",\"Name\":\"Moisture Value\",\"Type\":\"Content\",\"ReadFrequency\":\"2000\"}]}}";
	package = new IoTPackage(IoTPackage::ProtocolVersion, iotIp, IBroker::managerIoTIp, deviceDesc);
	this->client->SendData(&package->DataVectorForSending);
	
	iotIp = this->askIoTIp();
	deviceDesc = "{\"IOTDEV\":{\"DeviceName\":\"Digital light sensor\",\"FunctionGroup\":\"Sensor\",\"DeviceID\":\"arduino.grove.dl01\",\"Component\":[{\"ID\":\"lux\",\"Group\":\"1\",\"Name\":\"Brightness\",\"Type\":\"Content\",\"ReadFrequency\":\"10000\"}]}}";
	package = new IoTPackage(IoTPackage::ProtocolVersion, iotIp, IBroker::managerIoTIp, deviceDesc);
	this->client->SendData(&package->DataVectorForSending);

	iotIp = this->askIoTIp();
	deviceDesc = "{\"IOTDEV\":{\"DeviceName\":\"Temperature and humidity sensor\",\"FunctionGroup\":\"Sensor\",\"DeviceID\":\"arduino.grove.pro.ths01\",\"Component\":[{\"ID\":\"tempe\",\"Group\":\"1\",\"Name\":\"Temperature\",\"Type\":\"Content\",\"ReadFrequency\":\"10000\"},{\"ID\":\"humi\",\"Group\":\"2\",\"Name\":\"Humidity\",\"Type\":\"Content\",\"ReadFrequency\":\"10000\"}]}}";
	package = new IoTPackage(IoTPackage::ProtocolVersion, iotIp, IBroker::managerIoTIp, deviceDesc);
	this->client->SendData(&package->DataVectorForSending);
	
	iotIp = this->askIoTIp();
	deviceDesc = "{\"IOTDEV\":{\"DeviceName\":\"ACT\",\"FunctionGroup\":\"Actuator\",\"DeviceID\":\"arduino.uno.act01\",\"Component\":[{\"ID\":\"FON\",\"Group\":\"1\",\"Name\":\"Fan ON\",\"Type\":\"Button\"},{\"ID\":\"FOF\",\"Group\":\"1\",\"Name\":\"FAN Off\",\"Type\":\"Button\"},{\"ID\":\"WON\",\"Group\":\"2\",\"Name\":\"Water mercury On\",\"Type\":\"Button\"},{\"ID\":\"WOF\",\"Group\":\"2\",\"Name\":\"Water mercury Off\",\"Type\":\"Button\"},{\"ID\":\"SON\",\"Group\":\"3\",\"Name\":\"Smoke On\",\"Type\":\"Button\"},{\"ID\":\"SOF\",\"Group\":\"3\",\"Name\":\"Smoke Off\",\"Type\":\"Button\"}]}}";
	package = new IoTPackage(IoTPackage::ProtocolVersion, iotIp, IBroker::managerIoTIp, deviceDesc);
	this->client->SendData(&package->DataVectorForSending);
	
}

