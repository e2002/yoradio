/**************************************************************

    Example of esp32 deep sleep when playback is stopped.
    This file must be in the root directory of the sketch.

**************************************************************/
#define SLEEP_DELAY     60  // 1 min
#define WAKEUP_PIN_1    GPIO_NUM_12
#define WAKEUP_LEVEL    LOW

Ticker deepSleepTicker;

void goToSleep(){
  if(BRIGHTNESS_PIN!=255) analogWrite(BRIGHTNESS_PIN, 0); /*  BRIGHTNESS_PIN added in v0.7.330  */
  esp_deep_sleep_start();
}

void yoradio_on_setup(){
  esp_sleep_enable_ext0_wakeup(WAKEUP_PIN_1, WAKEUP_LEVEL);
  deepSleepTicker.attach(SLEEP_DELAY, goToSleep);
}

void player_on_start_play(){
  deepSleepTicker.detach();
}

void player_on_stop_play(){
  deepSleepTicker.attach(SLEEP_DELAY, goToSleep);
}
