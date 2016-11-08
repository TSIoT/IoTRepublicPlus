#include "SystemUtility.h"
#include "file.h"
#include "JsonUtility.h"
#include <fstream>
#include <iterator>


static const char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char decodeTable[256] =
{
	/* ASCII table */
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
	64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
	64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

void ms_sleep(int ms)
{
#if defined(WIN32)
	Sleep(ms);
#elif defined(__linux__) || defined(__FreeBSD__)
	usleep(ms * 1000);
#endif
}

unsigned long long get_millis()
{
	unsigned long long millisecondsSinceEpoch = 0;
#if defined(__linux__) || defined(__FreeBSD__)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	millisecondsSinceEpoch =
		(unsigned long long)(tv.tv_sec) * 1000 +
		(unsigned long long)(tv.tv_usec) / 1000;
	return millisecondsSinceEpoch;
#elif defined(WIN32)
	millisecondsSinceEpoch = GetTickCount();
#endif
	return millisecondsSinceEpoch;
}

void charcat(char *base_buf, char *target, int starIdx, int length)
{
	int i = 0, idx = starIdx;

	for (i = 0; i < length; i++)
	{
		base_buf[idx] = target[i];
		idx++;
	}
}

void printAllChar(char *data, int length)
{
	int i = 0;
	for (i = 0; i < length; i++)
	{
		if (data[i] == '\0')
			cout << ',';			
		else
		{
			cout << data[i];			
		}

	}
	cout << endl;
}

/*
void writeLog(string log)
{
	TSFile fileHandle;	
	char *buffer=new char[log.size()];
	
	for (int i = 0; i < log.size(); i++)
	{
		buffer[i] = log.at(i);
	}

	char *path = "C:\\Users\\loki.chuang\\Documents\\Visual Studio 2015\\Projects\\IoTRepublicPlus\\Debug\\log.txt";

	File_open(&fileHandle, path, O_RDWR);
	File_write(&fileHandle, buffer, log.size());	
	File_close(&fileHandle);

	free(buffer);
}
*/

void writeLog(std::vector<char> *log)
{
#if defined(WIN32)
	TSFile fileHandle;
	char *path = "C:\\Users\\loki.chuang\\Documents\\Visual Studio 2015\\Projects\\IoTRepublicPlus\\Debug\\log.txt";
	char *writeBuf = new char[log->size()];
	int length = log->size();

	for (int i = 0; i < length; i++)
	{
		if (log->at(i) == 0)
		{
			writeBuf[i] = ',';
		}
		else
		{
			writeBuf[i] = log->at(i);
		}
	}


	File_open(&fileHandle, path, O_APPEND | O_RDWR);
	File_write(&fileHandle, writeBuf, length);
	File_write(&fileHandle, "\n", 1);
	File_close(&fileHandle);
#endif // defined
}

bool ReadTextFileToVector(string path, std::vector<char> *vector)
{
	bool succeed = 0;
	ifstream fileHandle;
	fileHandle.open(path);
	//std::vector<char> readBuffer;
	std::istream_iterator<char> istream_iterator;

	int fileSize = 0;

	if (fileHandle.is_open())
	{
		fileHandle.seekg(0, std::ifstream::end);
		fileSize = (int)fileHandle.tellg();
		vector->reserve(fileSize);
		fileHandle.seekg(0, std::ifstream::beg);

		vector->insert(vector->begin(), std::istream_iterator<char>(fileHandle), std::istream_iterator<char>());

		fileHandle.close();	
		succeed = 1;
	}
	else
	{
		succeed = 0;
		cout << "Cannot load config file!!" << endl;
	}

	return succeed;
}

int Base64encode(char *encoded, char *source, int len)
{
	int i;
	char *p;

	p = encoded;
	for (i = 0; i < len - 2; i += 3)
	{
		*p++ = basis_64[(source[i] >> 2) & 0x3F];
		*p++ = basis_64[((source[i] & 0x3) << 4) |
			((int)(source[i+1] & 0xF0) >> 4)];
		*p++ = basis_64[((source[i+1] & 0xF) << 2) |
			((int)(source[i+2] & 0xC0) >> 6)];
		*p++ = basis_64[source[i+2] & 0x3F];
	}

	if (i < len)
	{
		*p++ = basis_64[(source[i] >> 2) & 0x3F];
		if (i == (len - 1)) 
		{
			*p++ = basis_64[((source[i] & 0x3) << 4)];
			*p++ = '=';
		}
		else 
		{
			*p++ = basis_64[((source[i] & 0x3) << 4) |
				((int)(source[i+1] & 0xF0) >> 4)];
			*p++ = basis_64[((source[i+1] & 0xF) << 2)];
		}
		*p++ = '=';
	}

	*p++ = '\0';
	return p - encoded - 1; //last '0' is not needed
}

int Base64decode(char *bufplain, char *bufcoded)
{
	int nbytesdecoded;
	register const unsigned char *bufin;
	register unsigned char *bufout;
	register int nprbytes;

	bufin = (const unsigned char *)bufcoded;
	while (decodeTable[*(bufin++)] <= 63);
	nprbytes = (bufin - (const unsigned char *)bufcoded) - 1;
	nbytesdecoded = ((nprbytes + 3) / 4) * 3;

	bufout = (unsigned char *)bufplain;
	bufin = (const unsigned char *)bufcoded;

	while (nprbytes > 4) {
		*(bufout++) =
			(unsigned char)(decodeTable[*bufin] << 2 | decodeTable[bufin[1]] >> 4);
		*(bufout++) =
			(unsigned char)(decodeTable[bufin[1]] << 4 | decodeTable[bufin[2]] >> 2);
		*(bufout++) =
			(unsigned char)(decodeTable[bufin[2]] << 6 | decodeTable[bufin[3]]);
		bufin += 4;
		nprbytes -= 4;
	}

	/* Note: (nprbytes == 1) would be an error, so just ingore that case */
	if (nprbytes > 1) {
		*(bufout++) =
			(unsigned char)(decodeTable[*bufin] << 2 | decodeTable[bufin[1]] >> 4);
	}
	if (nprbytes > 2) {
		*(bufout++) =
			(unsigned char)(decodeTable[bufin[1]] << 4 | decodeTable[bufin[2]] >> 2);
	}
	if (nprbytes > 3) {
		*(bufout++) =
			(unsigned char)(decodeTable[bufin[2]] << 6 | decodeTable[bufin[3]]);
	}

	*(bufout++) = '\0';
	nbytesdecoded -= (4 - nprbytes) & 3;
	return nbytesdecoded;
}
