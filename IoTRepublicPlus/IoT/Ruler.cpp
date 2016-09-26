#include "Ruler.h"


Ruler::Ruler()
{
	this->IsRulerStarted = 0;
	this->rules.reserve(50);
}

Ruler::~Ruler()
{
}

void Ruler::StartRuler(int actionFrequency)
{	
	this->actionFrequency = actionFrequency;

	Thread_create(&this->rulerThread, (TSThreadProc)Ruler::ruler_main_loop_entry, this);
	Thread_run(&this->rulerThread);	
}

void Ruler::StopRuler()
{		
	Thread_stop(&this->rulerThread);
	Thread_kill(&this->rulerThread);

	int ruleCount = this->rules.size();
	for (int i = 0; i < ruleCount; i++)
	{
		delete this->rules.at(i).rule;
	}

	cout << "Ruller stoped!" << endl;

	
}

void Ruler::handleRulePackage(IoTPackage *package)
{
	string content(package->DataVector.begin(), package->DataVector.end());
	json_t *root = JsonUtility::LoadJsonData(content);
	if (root != NULL)
	{
		cout << "Got rule command" << endl;
		string rootName = JsonUtility::GetFirstKeyName(root);
		if (rootName == "IOTRUL")
		{
			cout << "Add a rule" << endl;
			IoTRule *rule=new IoTRule(content);
			this->addRule(rule);
		}
		else if (rootName == "IOTCMD")
		{
			//cout << "Ruler Receive command" << endl;
			IoTCommand cmd(&package->DataVector);
			this->handleRulerCommand(&cmd);
		}
		
		delete root;
	}	
}


//private 
void Ruler::ruler_main_loop_entry(Ruler *clientObj)
{
	clientObj->mainLoop();
}

void Ruler::mainLoop()
{
	cout << "Ruler loop start" << endl;
	while (1)
	{
		this->scanRules();
		ms_sleep(this->actionFrequency);
	}
}

void Ruler::addRule(IoTRule *rule)
{
	RuleInstance instance;
	instance.rule = rule;
	instance.RuleID = this->nextRuleId;
	instance.WaitingTimeMs = 0;
	this->rules.push_back(instance);

	this->nextRuleId++;
}

void Ruler::removeRule(int ruleId)
{
	int ruleCount = this->rules.size();

	for (int i = 0; i < ruleCount; i++)
	{
		if (this->rules.at(i).RuleID == ruleId)
		{
			this->rules.erase(this->rules.begin() + i);
			break;
		}
	}


}

void Ruler::scanRules()
{
	int ruleCount = this->rules.size();

	for (int i = 0; i < ruleCount; i++)
	{
		if (this->rules.at(i).WaitingTimeMs <= 0)
		{
			//cout << "Excute rule no:" << i << endl;
			this->rules.at(i).WaitingTimeMs = this->rules.at(i).rule->ActionFrequency;
			this->excuteReadRule(&this->rules.at(i));
		}
		else
		{
			this->rules.at(i).WaitingTimeMs -= this->actionFrequency;
		}
	}
}

void Ruler::handleRulerCommand(IoTCommand *cmd)
{
	cout << "Ruler recv package";
	int ruleCount = this->rules.size();

	for (int i = 0; i < ruleCount; i++)
	{
		if (this->rules.at(i).isWaitingResponse && this->rules.at(i).rule->ReadID == cmd->ID)
		{
			this->rules.at(i).isWaitingResponse = 0;
			this->excuteWriteRule(&this->rules.at(i), cmd->Value);
			break;
		}
	}
}

void Ruler::excuteReadRule(RuleInstance *rule)
{
	cout << "Excuting read:" << rule->RuleID <<endl;
}

void Ruler::excuteWriteRule(RuleInstance *rule, string writeValue)
{
	cout << "Excuting write:" << rule->RuleID << endl;
}


