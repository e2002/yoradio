#include "spidog.h"

SPIDog sdog;

SPIDog::SPIDog() {
	_busy = false;
}

bool SPIDog::begin(){
	_spiMutex = xSemaphoreCreateMutex();
	return (_spiMutex != NULL);
}

bool SPIDog::takeMutex(){
	if(_spiMutex == NULL) {
		return false; 
	}
	if(xSemaphoreTake(_spiMutex, SDOG_PORT_DELAY) == pdTRUE){
		_busy = true;
		return true;
	}
	return false;
}

void SPIDog::giveMutex(){
	if(_spiMutex != NULL) xSemaphoreGive(_spiMutex);
	_busy = false;
}

bool SPIDog::breakMutex(uint8_t ticks){
	if(!_busy){
		giveMutex();
		vTaskDelay(ticks);
		return takeMutex();
	}
	return false;
}
