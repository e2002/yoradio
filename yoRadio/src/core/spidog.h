#ifndef spidog_h
#define spidog_h
#include <Arduino.h>
#include "options.h"

#ifndef SDOG_PORT_DELAY
  #define SDOG_PORT_DELAY  portMAX_DELAY
#endif

class SPIDog {
	private:
		SemaphoreHandle_t _spiMutex=NULL;
		bool _busy;
  public:
  	SPIDog();
  	~SPIDog(){}
  	bool begin();
  	bool takeMutex();
  	void giveMutex();
  	bool canTake();
  	bool breakMutex(uint8_t ticks=5);
  	bool busy() { return _busy; }
  	bool tm() { return takeMutex(); }
  	void gm() { giveMutex(); }
};

extern SPIDog sdog;

#endif
