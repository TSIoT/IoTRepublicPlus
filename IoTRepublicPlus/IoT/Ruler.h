#ifndef Ruler_H_INCLUDED
#define Ruler_H_INCLUDED

#include <iostream>
#include <vector>
#include "IoTRule.h"
#include "IoTPackage.h"
#include "IoTCommand.h"
#include "../Utility/thread.h"

using namespace std;

class Ruler
{
public:
	bool IsRulerStarted;

	Ruler();
	~Ruler();	
	void StartRuler(int actionFrequency);
	void StopRuler();		
	void handleRulePackage(IoTPackage *package);

protected:
	class RuleInstance
	{
	public:
		int RuleID = -1;
		IoTRule *rule = NULL;
		int WaitingTimeMs;
		bool isWaitingResponse = 0;
	};

	std::vector<RuleInstance> rules;

	void addRule(IoTRule *rule);
	void removeRule(int ruleId);

private:	
	TSThread rulerThread;
	int nextRuleId = 0;

	
	int actionFrequency;

	static void ruler_main_loop_entry(Ruler *clientObj);
	void mainLoop();
	void scanRules();
	void handleRulerCommand(IoTCommand *cmd);

	virtual void excuteReadRule(RuleInstance *rule);
	virtual void excuteWriteRule(RuleInstance *rule, string writeValue);
	
};

#endif


