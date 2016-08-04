#include <iostream>
#include <stdlib.h>
#include "IoT/IoTManager.h"
#include "Utility/SystemUtility.h"

#include "Utility/JsonUtility.h"

using namespace std;


#define ServerPort 6210
#define MaxRecvBuf 2000
#define MaxClient 500

int main()
{
	
	/*
	JsonUtility json;
	//json.jsonTest();
	//json.ArrayTest();
	//json.CommandTest();
	//json.DumpTest();
	PAUSE;
	*/

	/*
	std::vector<int> myvector;

	// set some values (from 1 to 10)
	for (int i = 1; i <= 10; i++) myvector.push_back(i);

	// erase the 6th element
	myvector.erase(myvector.begin() + 4);

	// erase the first 3 elements:
	//myvector.erase(myvector.begin()+3, myvector.begin() + 3);

	std::cout << "myvector contains:";
	for (unsigned i = 0; i<myvector.size(); ++i)
		std::cout << ' ' << myvector[i];
	std::cout << '\n';

	PAUSE;
	*/
	
	IoTManager manager(ServerPort, MaxRecvBuf, MaxClient);
	manager.StartManager();

	PAUSE;
	manager.StopManager();	
	
	return 0;
}
