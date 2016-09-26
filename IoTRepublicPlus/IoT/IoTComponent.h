#ifndef IoTComponent_H_INCLUDED
#define IoTComponent_H_INCLUDED

#include <iostream>
using namespace std;

class IoTComponent
{
public:
	enum ComType
	{
		Unknown,
		Content,
		Button
	};

	string ID;
	string Group;
	string Name;
	ComType Type;
	int ReadFrequency;

	IoTComponent();
	~IoTComponent();
};

#endif

