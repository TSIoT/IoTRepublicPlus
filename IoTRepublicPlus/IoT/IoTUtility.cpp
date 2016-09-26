#include "IoTUtility.h"
#include "../Utility/SystemUtility.h"
#include <string>
#include <string.h>

//public
IoTPackage* IoTUtility::GetCompletedPackage(std::vector<char> *buffer, GetPackageError *error)
{
	IoTPackage *package = NULL;

	bool isValid = this->isValidPackage(buffer, error);

	if (isValid)
	{
		//char testArray[100] = {0};
		package = this->decodePackage(buffer);
		*error = GetPackageError_NoError;
		package->IsCompletedPackage = 1;


		//copy the exists sended data to IoTPackage
		int totalLength = package->HeaderLength + package->DataLength + 2;//2 is for check sum
		package->DataVectorForSending.insert(
			package->DataVectorForSending.begin(),
			buffer->begin(),
			buffer->begin() + totalLength);
		/*
		for (i = 0; i < totalLength; i++)
		{
			package->DataForSending[i] = buffer[i];
		}
		package->SendLength = totalLength;
		*/
		//modify the buffer

		if ((int)buffer->size() < totalLength)
		{
			cout << "This should not happened" << endl;
		}

		buffer->erase(buffer->begin(), buffer->begin()+ totalLength);
		//buffer->clear();

	}
	else
	{
		package = NULL;
		//cout << "invaild package(mabye not completed yet)" << endl;
	}

	return package;
}

bool IoTUtility::isValidPackage(std::vector<char> *buffer, GetPackageError *error)
{
	IoTPackage *package = NULL;
	int totalLength = 0;
	unsigned int sum = 0, sendedSum = 0;
	bool isVaild = 0;

	bool lengthIsLongEnough = 0;
	bool startWordIsCorrect = 0;
	bool headerIsCompleted = 0;
	bool packageIsCompleted = 0;
	bool checkSumNoError = 0;

	//check if the length is long enough
	if (buffer->size() >= this->currentVersion.length())
	{
		lengthIsLongEnough = 1;
	}
	else
	{
		//cout << "header length is still too short" << endl;
		*error = GetPackageError_lengthIsNotLongEnough;
	}


	if (lengthIsLongEnough)
	{
		//Check if the start word is correct
		if (memcmp(this->currentVersion.c_str(), &buffer->at(0), this->currentVersion.length()) == 0)
		{
			startWordIsCorrect = 1;
		}
		else
		{
			buffer->push_back('\0');
			cout << "Start word is wrong:" << &buffer->at(0) << endl;
			*error = GetPackageError_StartWordError;
		}
	}

	if (lengthIsLongEnough && startWordIsCorrect)
	{
		//Check if the header is completed
		int totalClumnCount = 0;
		for (int i = 0; i < (int)buffer->size(); i++)
		{
			if (buffer->at(i) == '\0')
			{
				totalClumnCount++;
			}

			if (totalClumnCount >= this->headerColumns)
				break;
		}

		if (totalClumnCount >= this->headerColumns)
		{
			headerIsCompleted = 1;
		}
		else
		{
			//cout << "Header is not completed" << endl;
			*error = GetPackageError_HeaderNotCompleted;
		}
	}

	if (lengthIsLongEnough && startWordIsCorrect && headerIsCompleted)
	{
		//check if the packaage is completed
		package = this->decodeOnlyHeader(buffer);
		if ((int)buffer->size() >= (package->HeaderLength + package->DataLength + 2))
		{
			//cout << "The package is completed" << endl;
			packageIsCompleted = 1;
		}
		else
		{
			//cout << "package is not completed" << endl;
			*error = GetPackageError_PackageNotCompleted;
		}
	}

	if (lengthIsLongEnough && startWordIsCorrect && headerIsCompleted && packageIsCompleted)
	{
		//check sum
		totalLength = package->HeaderLength + package->DataLength;
		for (int i = 0; i < totalLength; i++)
		{
			sum += buffer->at(i);
		}
		
		while ((sum >> 16) > 0)
		{
			sum = (sum & 0xFFFF) + (sum >> 16);
		}
		
		sendedSum = 0;
		sendedSum |= (unsigned char)buffer->at(totalLength);
		sendedSum = sendedSum << 8;
		sendedSum |= (unsigned char)buffer->at(totalLength + 1);

		if (sum == sendedSum)
		{
			checkSumNoError = 1;

		}
		else
		{			
			//writeLog(buffer);
			cout << "Check sum error!" << endl;
			//if check sum error, need to clear the buffer
			*error = GetPackageError_ChecksumError;
		}
	}

	if (package != NULL)
		delete package;

	if (lengthIsLongEnough && startWordIsCorrect &&
		headerIsCompleted && packageIsCompleted &&
		checkSumNoError)
	{
		//cout << "Received a vaild package!" << endl;;
		isVaild = 1;
		*error = GetPackageError_NoError;

	}

	return isVaild;
}

string IoTUtility::GetCurrentProtorolVersion()
{
	return this->currentVersion;
}

//private
IoTPackage* IoTUtility::decodeOnlyHeader(std::vector<char> *buffer)
{
	IoTPackage *package = new IoTPackage();
	int startIndex = 0;
	string tempStr;
	//printAllChar(&buffer->at(0),buffer->size());
	//version
	package->Ver = this->popHeader(buffer, startIndex);
	startIndex += package->Ver.length()+1; //+1 is for segement symbol

	//header length
	tempStr = this->popHeader(buffer, startIndex);
	package->HeaderLength = std::stoi(tempStr);

	startIndex += tempStr.length() + 1; //+1 is for segement symbol

	//data length
	tempStr = this->popHeader(buffer, startIndex);
	package->DataLength = std::stoi(tempStr);

	startIndex += tempStr.length() + 1; //+1 is for segement symbol

	//source IoT ip
	package->SorIp = this->popHeader(buffer, startIndex);
	startIndex += package->SorIp.length() + 1; //+1 is for segement symbol

	//destination IoT ip
	package->DesIp = this->popHeader(buffer, startIndex);

	return package;
}

IoTPackage* IoTUtility::decodePackage(std::vector<char> *buffer)
{
	int totalLength = 0, i = 0, j = 0;

	IoTPackage *package = this->decodeOnlyHeader(buffer);

	//copy data to package class
	totalLength = package->HeaderLength + package->DataLength;
	package->DataVector.reserve(package->DataLength);
	for (i = package->HeaderLength; i < totalLength; i++, j++)
	{
		package->DataVector.push_back(buffer->at(i));
	}
	//Check sum
	package->Checksum = 0;

	package->Checksum |= (unsigned char)buffer->at(totalLength);
	package->Checksum = package->Checksum << 8;
	package->Checksum |= (unsigned char)buffer->at(totalLength + 1);

	return package;
}

string IoTUtility::popHeader(std::vector<char> *buffer,int startIndex)
{
	stringstream tempBuffer;

	for (int i = startIndex; i < (int)buffer->size(); i++)
	{
		if (buffer->at(i) != '\0')
		{
			tempBuffer << buffer->at(i);
			//cout << buffer->at(i);
		}
		else
		{
			break;
		}
	}
    //cout << endl;

	return tempBuffer.str();
}
