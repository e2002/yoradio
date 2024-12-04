/**
 * Example of display backlight control depending on playback.
 * To connect the plugin, copy its folder to the src/plugins directory.
 */
#include "backlightcontrols.h"
#include <Arduino.h>
#include <Ticker.h>
#include "../../core/options.h"

Ticker backlightTicker;
backlightControls blc;

const uint8_t backlightPin       = 13;
const uint8_t backlightInitValue = HIGH;
const uint16_t turnBlOffInterval = 120;   /* 2 min */

void backlightOff(){
  backlightTicker.detach();
  digitalWrite(backlightPin, !backlightInitValue);
}

backlightControls::backlightControls() {
  registerPlugin();
  log_i("Plugin is registered");
}

void backlightControls::on_setup(){
  log_i("%s called", __func__ );
  pinMode(backlightPin, OUTPUT);
  digitalWrite(backlightPin, backlightInitValue);
  backlightTicker.attach(turnBlOffInterval, backlightOff);
}

void backlightControls::on_track_change(){
  log_i("%s called", __func__ );
  digitalWrite(backlightPin, backlightInitValue);
  backlightTicker.detach();
  backlightTicker.attach(turnBlOffInterval, backlightOff);
}

void backlightControls::on_stop_play(){
  log_i("%s called", __func__ );
  digitalWrite(backlightPin, backlightInitValue);
  backlightTicker.detach();
  backlightTicker.attach(turnBlOffInterval, backlightOff);
}
