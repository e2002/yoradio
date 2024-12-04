/*
 *******************************************************************************************
 * Attention!
 * This method of connecting plugins no longer works and is left here for history.
 *******************************************************************************************

*/
/******************************************************************************************************************

    Example of esp32 deep sleep when playback is stopped.
    This file must be in the root directory of the sketch.

*******************************************************************************************************************/
#define SLEEP_DELAY     60        /* 1 min        deep sleep delay                                                */
#define WAKEUP_PIN      ENC_BTNB  /*              wakeup pin (one of: BTN_XXXX, ENC_BTNB, ENC2_BTNB)              */
                                  /*              must be one of: 0,2,4,12,13,14,15,25,26,27,32,33,34,35,36,39    */
#define WAKEUP_LEVEL    LOW       /*              wakeup level (usually LOW)                                      */

#if WAKEUP_PIN!=255
Ticker deepSleepTicker;

void goToSleep(){
  if(BRIGHTNESS_PIN!=255) analogWrite(BRIGHTNESS_PIN, 0);               /*  BRIGHTNESS_PIN added in v0.7.330      */
  if(display.deepsleep()) {                                             /*  if deep sleep is possible             */
    esp_deep_sleep_start();                                             /*  go to sleep                           */
  }else{                                                                /*  else                                  */
    deepSleepTicker.detach();                                           /*  detach the timer                      */
  }
}

void yoradio_on_setup(){                                                /*  occurs during loading                 */
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKEUP_PIN, WAKEUP_LEVEL);   /*  enable wakeup pin                     */
  deepSleepTicker.attach(SLEEP_DELAY, goToSleep);                       /*  attach to delay                       */
}

void player_on_start_play(){                                            /*  occurs during player is start playing */
  deepSleepTicker.detach();                                             /*  detach the timer                      */
}

void player_on_stop_play(){                                             /*  occurs during player is stop playing  */
  deepSleepTicker.attach(SLEEP_DELAY, goToSleep);                       /*  attach to delay                       */
}
#endif  /*  #if WAKEUP_PIN!=255 */
