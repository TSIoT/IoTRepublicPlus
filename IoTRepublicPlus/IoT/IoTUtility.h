#ifndef IoTUtility_H_INCLUDED
#define IoTUtility_H_INCLUDED

#include <iostream>
#include "IoTPackage.h"
#include <vector>
using namespace std;

class IoTUtility
{
public:
	enum GetPackageError
	{
		GetPackageError_NoError,
		GetPackageError_UnknownError,
		GetPackageError_lengthIsNotLongEnough,
		GetPackageError_StartWordError,
		GetPackageError_HeaderNotCompleted,
		GetPackageError_PackageNotCompleted,
		GetPackageError_ChecksumError,
	};

	IoTUtility(string protocalVersion,char segmentSymbol)
	{
		this->currentVersion = protocalVersion;
		this->segmentSymbol = segmentSymbol;
	}

	//void IoTUtility::FreeIoTPackage(IoTPackage *package);
	//IoTPackage* GetCompletedPackage(char *buffer,int *bufferLength, GetPackageError *error);
	IoTPackage* GetCompletedPackage(std::vector<char> *buffer, GetPackageError *error);
	//bool isVaildPackage(char *buffer, int *buffer_length, GetPackageError *error);
	bool isVaildPackage(std::vector<char> *buffer, GetPackageError *error);
	string GetCurrentProtorolVersion();

private:
	//string currentVersion="IUDP1.0";
	string currentVersion;
	int headerColumns = 5;
	char segmentSymbol;

	//IoTPackage* IoTUtility::decodeOnlyHeader(char *buffer, int buffer_length);
	IoTPackage* decodeOnlyHeader(std::vector<char> *buffer);
	//IoTPackage* IoTUtility::decodePackage(char *buffer, int buffer_length);
	IoTPackage* decodePackage(std::vector<char> *buffer);
	//string* popHeader(char *buffer, int buffer_length);
	string popHeader(std::vector<char> *buffer, int startIndex);

};

#endif
