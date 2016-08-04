#include "IoTUtility.h"
#include "../Utility/SystemUtility.h"
#include <string>

//public
IoTPackage* IoTUtility::GetCompletedPackage(char *buffer, int *bufferOffset, GetPackageError *error)
{
	IoTPackage *package = NULL;
	bool isVaild = this->isVaildPackage(buffer, bufferOffset, error);

	if (isVaild)
	{
		package = this->decodePackage(buffer, *bufferOffset);
		*error = GetPackageError_NoError;
		package->IsCompletedPackage = 1;

		//modify the buffer and bufferLength
		int totalLength = package->HeaderLength + package->DataLength + 2;//2 is for check sum
		int another_package_len = *bufferOffset - totalLength;
		int idx = totalLength;
		int i = 0;

		//copy the exists sended data to IoTPackage
		package->DataForSending = new char[totalLength];		
		for (i = 0; i < totalLength; i++)
		{
			package->DataForSending[i] = buffer[i];
		}
		package->SendLength = totalLength;

		//printAllChar(package->DataForSending, totalLength);

		//clear data that related to the completed package
		for (i = 0; i < another_package_len; i++)
		{
			buffer[i] = buffer[idx];
			idx++;
		}

		for (; i < *bufferOffset; i++)
		{
			buffer[i] = '\0';
		}
		*bufferOffset = another_package_len;

		//cout << "received a vaild package" << endl;
	}
	else
	{
		package = NULL;
		//cout << "invaild package(mabye not completed yet)" << endl;		
	}
	
	return package;
}

bool IoTUtility::isVaildPackage(char *buffer, int *buffer_length, GetPackageError *error)
{
	IoTPackage *package = NULL;
	int totalLength = 0;
	unsigned int sum = 0, sendedSum=0;
	bool isVaild = 0;
	buffer[*buffer_length] = '\0';

	bool lengthIsLongEnough = 0;
	bool startWordIsCorrect = 0;
	bool headerIsCompleted = 0;
	bool packageIsCompleted = 0;
	bool checkSumNoError = 0;
	
	//check if the length is long enough
	if ((unsigned int)*buffer_length >= this->currentVersion.length())
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
		if (memcmp(this->currentVersion.c_str(), buffer, this->currentVersion.length()) == 0)
		{
			startWordIsCorrect = 1;
		}
		else
		{
			cout << "Start word is wrong:"<< buffer << endl;
			*error = GetPackageError_StartWordError;
		}
	}

	if (lengthIsLongEnough && startWordIsCorrect)
	{
		//Check if the header is completed
		int totalClumnCount = 0;
		for (int i = 0; i < *buffer_length; i++)
		{
			if (buffer[i] == '\0')
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
		package= this->decodeOnlyHeader(buffer, *buffer_length);
		if (*buffer_length >= (package->HeaderLength + package->DataLength + 2))
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
			sum += buffer[i];
		}
		
		sendedSum = 0;
		sendedSum |= (unsigned char)buffer[totalLength];
		sendedSum = sendedSum << 8;
		sendedSum |= (unsigned char)buffer[totalLength+1];

		if (sum == sendedSum)
		{
			checkSumNoError = 1;
			
		}
		else
		{
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

IoTPackage* IoTUtility::decodeOnlyHeader(char *buffer, int buffer_length)
{
	char *temp_buf = new char[buffer_length];
	string *tempStr = NULL;
	std::fill_n(temp_buf, buffer_length, '\0');
	memcpy(temp_buf, buffer, buffer_length);

	IoTPackage *package = new IoTPackage();

	//version
	tempStr =this->popHeader(temp_buf, buffer_length);
	package->Ver =*tempStr;
	delete tempStr;

	//header length
	tempStr = this->popHeader(temp_buf, buffer_length);
	package->HeaderLength = std::stoi(*tempStr);
	delete tempStr;

	//data length
	tempStr = this->popHeader(temp_buf, buffer_length);
	package->DataLength = std::stoi(*tempStr);
	delete tempStr;

	//source IoT ip
	tempStr = this->popHeader(temp_buf, buffer_length);
	package->SorIp = *tempStr;
	delete tempStr;

	//destination IoT ip
	tempStr = this->popHeader(temp_buf, buffer_length);
	package->DesIp = *tempStr;
	delete tempStr;
	
	//package->Ver = this->popHeader(buffer, buffer_length);

	delete[] temp_buf;
	return package;
}

IoTPackage* IoTUtility::decodePackage(char *buffer, int buffer_length)
{
	int totalLength = 0,i=0,j=0;

	IoTPackage *package = this->decodeOnlyHeader(buffer, buffer_length);

	//copy data to package class
	totalLength = package->HeaderLength + package->DataLength;
	package->Data = new char[package->DataLength];
	std::fill_n(package->Data, package->DataLength,0);
	for (i = package->HeaderLength; i < totalLength; i++,j++)
	{		
		package->Data[j] = buffer[i];
	}

	//Check sum
	package->Checksum = 0;

	package->Checksum |= (unsigned char)buffer[totalLength];
	package->Checksum = package->Checksum << 8;
	package->Checksum |= (unsigned char)buffer[totalLength + 1];

	return package;
}

string* IoTUtility::popHeader(char *buffer, int buffer_length)
{
	int i = 0,j=0;
	string *headerValue;
	char *valueTemp = new char[buffer_length];	
	//printAllChar(buffer, buffer_length);

	std::fill_n(valueTemp, buffer_length, '\0');

	for (i = 0; i < buffer_length; i++)
	{
		if (buffer[i] == this->segmentSymbol)
		{
			i++;
			break;
		}
		else
		{
			valueTemp[i] = buffer[i];
		}
	}

	headerValue = new string(valueTemp);

	for (j = 0; j < buffer_length; j++,i++)
	{
		if (i < buffer_length)
		{
			buffer[j] = buffer[i];
		}			
		else
		{
			buffer[j] = 0;
		}
			
	}

	delete[] valueTemp;
	return headerValue;
}

