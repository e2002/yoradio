/**************************************************************

    Example of display backlight control depending on playback.
    This file must be in the root directory of the sketch.

**************************************************************/
#include <Ticker.h>

const uint8_t backlightPin       = 13;
const uint8_t backlightInitValue = HIGH;
const uint16_t turnBlOffInterval = 120;   /* 2 min */

Ticker backlightTicker;

void backlightOff(){
  backlightTicker.detach();
  digitalWrite(backlightPin, !backlightInitValue);
}

void yoradio_on_setup() {
  pinMode(backlightPin, OUTPUT);
  digitalWrite(backlightPin, backlightInitValue);
  backlightTicker.attach(turnBlOffInterval, backlightOff);
}

void player_on_track_change(){
  digitalWrite(backlightPin, backlightInitValue);
  backlightTicker.detach();
  backlightTicker.attach(turnBlOffInterval, backlightOff);
}

void player_on_stop_play(){
  digitalWrite(backlightPin, backlightInitValue);
  backlightTicker.detach();
  backlightTicker.attach(turnBlOffInterval, backlightOff);
}
