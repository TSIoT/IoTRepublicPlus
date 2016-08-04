#ifndef IoTPackage_H_INCLUDED
#define IoTPackage_H_INCLUDED

#include <iostream>
#include <sstream>

#include "../Utility/SystemUtility.h"

using namespace std;

class IoTPackage
{
public:
	static const char SegmentSymbol = '\0';

	bool IsCompletedPackage;
	string Ver;
	int HeaderLength;
	int DataLength;
	string SorIp;
	string DesIp;
	unsigned int Checksum;
	int SendLength;
	char *Data;
	char *DataForSending;
	
	
	IoTPackage::IoTPackage()
	{
		this->IsCompletedPackage = 0;
		this->Ver = "";
		this->HeaderLength = 0;
		this->DataLength = 0;
		this->SorIp = "";
		this->DesIp = "";
		this->Checksum = 0;
		this->Data = NULL;
		this->DataForSending = NULL;
		this->SendLength = 0;
	}

	IoTPackage::IoTPackage(string ver, string soIp, string desIp, char *data, int dataLength)
	{		
		int headerLength = 0;		
		stringstream tempStream;

		this->IsCompletedPackage = 1;
		this->Ver = ver;
		this->SorIp = soIp;
		this->DesIp = desIp;
		this->DataLength = dataLength;

		headerLength += this->Ver.length()+1; //1 is for segment symbol
		headerLength += this->SorIp.length() + 1; //1 is for segment symbol
		headerLength += this->DesIp.length() + 1; //1 is for segment symbol
		tempStream << dataLength;

		//cout << "DataLength:" << tempStream.str() << endl;
		headerLength += tempStream.str().length()+1;//1 is for segment symbol

		if (headerLength <= 8)
		{
			headerLength += 1;
		}
		else if (headerLength <= 97)
		{
			headerLength += 2;
		}
		else
		{
			headerLength += 3;
		}
		headerLength++; //for segment symbol

		this->HeaderLength = headerLength;

		this->Data = new char[dataLength];
		memcpy(this->Data, data, dataLength);
		
		this->Packet();		
		
	}

	IoTPackage::~IoTPackage()
	{
		//cout << "IoTPackage Class ended" << endl;
		if (this->Data != NULL)
			delete[] this->Data;

		if (this->DataForSending != NULL)
			delete[] this->DataForSending;
	}

	void Packet()
	{
		int offset = 0, i = 0;
		unsigned int sum = 0;
		stringstream tempStream;

		this->DataForSending = new char[this->HeaderLength + this->DataLength + 2]; //2 is for checksum
		std::fill_n(this->DataForSending, this->HeaderLength + this->DataLength, 0);

		//version
		for (i = 0; i <(int)this->Ver.length(); i++, offset++)
		{
			this->DataForSending[offset] = this->Ver.at(i);
		}
		this->DataForSending[offset] = SegmentSymbol;
		offset++;

		//printAllChar(this->DataForSending, offset);


		//header length
		tempStream.str(std::string()); //clear tempStream
		tempStream << this->HeaderLength;
		//cout << "Header length:" << tempStream.str() << endl;;

		for (i = 0; i < (int)tempStream.str().length(); i++, offset++)
		{
			this->DataForSending[offset] = tempStream.str().at(i);
		}
		this->DataForSending[offset] = SegmentSymbol;
		offset++;

		//data length
		tempStream.str(std::string()); //clear tempStream
		tempStream << this->DataLength;
		//cout << "Header length:" << tempStream.str() << endl;;
		for (i = 0; i <(int)tempStream.str().length(); i++, offset++)
		{
			this->DataForSending[offset] = tempStream.str().at(i);
		}
		this->DataForSending[offset] = SegmentSymbol;
		offset++;

		//source ip
		tempStream.str(std::string()); //clear tempStream
		for (i = 0; i <(int)this->SorIp.length(); i++, offset++)
		{
			this->DataForSending[offset] = this->SorIp.at(i);
		}
		this->DataForSending[offset] = SegmentSymbol;
		offset++;

		//destination ip
		tempStream.str(std::string()); //clear tempStream
		for (i = 0; i <(int)this->DesIp.length(); i++, offset++)
		{
			this->DataForSending[offset] = this->DesIp.at(i);
		}
		this->DataForSending[offset] = SegmentSymbol;
		offset++;


		//data
		for (i = 0; i < this->DataLength; i++, offset++)
		{
			this->DataForSending[offset] = this->Data[i];			
		}

		//checksum
		for (i = 0; i < offset; i++)
		{
			sum += (unsigned char)this->DataForSending[i];
		}

		while ((sum >> 16) > 0)
		{
			sum = (sum & 0xFFFF) + (sum >> 16);
		}

		this->DataForSending[offset] = (uint8_t)(sum >> 8);
		offset++;
		this->DataForSending[offset] = (uint8_t)sum;
		offset++;

		this->Checksum = sum;

		this->SendLength = offset;

		//printAllChar(this->DataForSending, offset);
	}	
};

#endif