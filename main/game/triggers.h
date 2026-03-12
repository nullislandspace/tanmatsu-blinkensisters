#ifndef TRIGGERS_H
#define TRIGGERS_H

typedef enum _TRIGGER_TYPE {
	TRIGGER_TYPE_NONE = 0,
	TRIGGER_TYPE_FG,  // Only in old-style LUA
	TRIGGER_TYPE_TIMER
} TRIGGER_TYPE;

struct TRIGGER {
	TRIGGER_TYPE type;
	bool isActive;
	Uint32 objid;
	double tickIncrement;
	double lastTick;
	char funcname[100];
};

void initTriggers();
void deInitTriggers();
void registerFGTrigger(Uint32 objid, bool isActive, char* funcname);
Uint32 registerTimerTrigger(char* funcname, double tickIncrement);
void setTriggerActive(Uint32 objid, bool isActive);
void handleTriggers();

void setTimerTriggerInterval(Uint32 triggerid, double tickIncrement);
double getTimerTriggerInterval(Uint32 triggerid);
void setTimerTriggerActive(Uint32 triggerid, bool isActive);
bool getTimerTriggerActive(Uint32 triggerid);

extern TRIGGER gTriggers[];


#endif // TRIGGERS_H
