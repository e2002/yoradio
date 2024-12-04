/**
 * Example of esp32 deep sleep when playback is stopped.
 * To connect the plugin, copy its folder to the src/plugins directory.
 */

#include "deepsleep.h" 
#include <Arduino.h>
#include <Ticker.h>
#include "../../core/options.h"
#include "../../core/display.h"

#define SLEEP_DELAY     60        /* 1 min        deep sleep delay                                                */
#define WAKEUP_PIN      ENC_BTNB  /*              wakeup pin (one of: BTN_XXXX, ENC_BTNB, ENC2_BTNB)              */
                                  /*              must be one of: 0,2,4,12,13,14,15,25,26,27,32,33,34,35,36,39    */
#define WAKEUP_LEVEL    LOW       /*              wakeup level (usually LOW)                                      */

Ticker deepSleepTicker;
deepSleep dsleep;

deepSleep::deepSleep() {
  registerPlugin();
  log_i("Plugin is registered");
}

void goToSleep(){
  if(BRIGHTNESS_PIN!=255) analogWrite(BRIGHTNESS_PIN, 0);               /*  BRIGHTNESS_PIN added in v0.7.330      */
  if(display.deepsleep()) {                                             /*  if deep sleep is possible             */
    esp_deep_sleep_start();                                             /*  go to sleep                           */
  }else{                                                                /*  else                                  */
    deepSleepTicker.detach();                                           /*  detach the timer                      */
  }
}

void deepSleep::on_setup(){                                             /*  occurs during loading                 */
  log_i("%s called", __func__ );
  if(WAKEUP_PIN!=255){
    esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKEUP_PIN, WAKEUP_LEVEL);   /*  enable wakeup pin                     */
    deepSleepTicker.attach(SLEEP_DELAY, goToSleep);                       /*  attach to delay                       */
  }
}

void deepSleep::on_start_play(){                                        /*  occurs during player is start playing */
  log_i("%s called", __func__ );
  if(WAKEUP_PIN!=255){
    deepSleepTicker.detach();                                             /*  detach the timer                      */
  }
}

void deepSleep::on_stop_play(){                                         /*  occurs during player is stop playing  */
  log_i("%s called", __func__ );
  if(WAKEUP_PIN!=255){
    deepSleepTicker.attach(SLEEP_DELAY, goToSleep);                       /*  attach to delay                       */
  }
}
