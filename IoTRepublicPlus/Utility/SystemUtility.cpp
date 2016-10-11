#include "SystemUtility.h"
#include "file.h"


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

