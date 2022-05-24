// yoEncoder.h
// based on https://github.com/igorantolic/ai-esp32-rotary-encoder code

#ifndef _YOENCODER_h
#define _YOENCODER_h

#include "Arduino.h"

typedef enum
{
	BUT_DOWN = 0,
	BUT_PUSHED = 1,
	BUT_UP = 2,
	BUT_RELEASED = 3,
	BUT_DISABLED = 99,
} ButtonState;

class yoEncoder
{

private:
	portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
	portMUX_TYPE buttonMux = portMUX_INITIALIZER_UNLOCKED;
	volatile long encoder0Pos = 0;

	volatile int8_t lastMovementDirection = 0; //1 right; -1 left
	volatile unsigned long lastMovementAt = 0;
	unsigned long rotaryAccelerationCoef = 150;

	bool _circleValues = false;
	bool isEnabled = true;

	uint8_t encoderAPin;
	uint8_t encoderBPin;
	long encoderSteps;

	long _minEncoderValue = -1 << 15;
	long _maxEncoderValue = 1 << 15;

	uint8_t old_AB;
	long lastReadEncoder0Pos;
	
	int8_t enc_states[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
	void (*ISR_callback)();

public:
	yoEncoder(
		uint8_t encoderAPin,
		uint8_t encoderBPin,
		uint8_t encoderSteps,
		bool    internalPullup = true);
	void setBoundaries(long minValue = -100, long maxValue = 100, bool circleValues = false);
	void IRAM_ATTR readEncoder_ISR();

	void setup(void (*ISR_callback)(void));
	void begin();
	void reset(long newValue = 0);
	void enable();
	void disable();
	long readEncoder();
	void setEncoderValue(long newValue);
	long encoderChanged();

	unsigned long getAcceleration() { return this->rotaryAccelerationCoef; }
	void setAcceleration(unsigned long acceleration) { this->rotaryAccelerationCoef = acceleration; }
	void disableAcceleration() { setAcceleration(0); }
};
#endif
