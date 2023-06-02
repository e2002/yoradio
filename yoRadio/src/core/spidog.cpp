#include "spidog.h"

SPIDog sdog;

SPIDog::SPIDog() {
	_busy = false;
}

bool SPIDog::begin(){
	if(_spiMutex==NULL){
		_spiMutex = xSemaphoreCreateMutex();
		if(_spiMutex==NULL) return false;
	}
	return true;
}

bool SPIDog::takeMutex(){
	if(_spiMutex == NULL) {
		return false; 
	}
	do { } while (xSemaphoreTake(_spiMutex, portMAX_DELAY) != pdPASS);
	_busy = true;
	return true;
}

void SPIDog::giveMutex(){
	if(_spiMutex != NULL) xSemaphoreGive(_spiMutex);
	_busy = false;
}

bool SPIDog::canTake(){
	if(_spiMutex == NULL) {
		return false; 
	}
	return xSemaphoreTake(_spiMutex, 0) == pdPASS;
}

bool SPIDog::breakMutex(uint8_t ticks){
	if(!_busy){
		giveMutex();
		vTaskDelay(ticks);
		return takeMutex();
	}
	return false;
}
