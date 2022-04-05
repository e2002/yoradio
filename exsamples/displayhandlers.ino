/**************************************************************

    An example of displaying user information on the display.
    This file must be in the root directory of the sketch.

**************************************************************/

#if DSP_MODEL==DSP_ST7735
// 3600s = 60 minutes to not flooding
#define WEATHER_REQUEST_INTERVAL      3600  // 60min

#include <JSON_Decoder.h>                                     // https://github.com/Bodmer/OpenWeather
#include <OpenWeather.h>                                      // https://github.com/Bodmer/JSON_Decoder
#include <Ticker.h>

String api_key = "********************************";          // openweathermap.org API key

String latitude =  "55.7512";
String longitude = "37.6184";

String units = "metric";
String language = "ru";

OW_Weather ow;
Ticker ticker;

/***********************************************
   scrolled line
 ***********************************************/
Scroll hello;

char weather[140] = { 0 };
bool weatherRequest = false;
TaskHandle_t weatherUpdateTaskHandle;

void getWeather( void * pvParameters ) {
  OW_current *current = new OW_current;
  OW_hourly *hourly = new OW_hourly;
  OW_daily  *daily = new OW_daily;
  delay(5000);
  ow.getForecast(current, hourly, daily, api_key, latitude, longitude, units, language);
  sprintf(weather, "TEMP: %.1f C * PRESS: %d HG * HUM: %d%%", current->temp, (int)(current->pressure / 1.333), current->humidity);
  weatherRequest = true;
  vTaskDelete( NULL );
}

void updateWeather() {
  xTaskCreatePinnedToCore(
    getWeather,                   /* Task function. */
    "getWeather1",                /* name of task. */
    8192,                         /* Stack size of task */
    NULL,                         /* parameter of the task */
    0,                            /* priority of the task */
    &weatherUpdateTaskHandle,     /* Task handle to keep track of created task */
    0);                           /* pin task to core CORE_FOR_LOOP_CONTROLS */
}

/***********************************************
   Occurs when the network is connected
 ***********************************************/
void network_on_connect() {
  ticker.attach(WEATHER_REQUEST_INTERVAL, updateWeather);
  updateWeather();
}

/*********************************************************************************************
   The display has initialized, the network is connected, the player is ready to play.
   DspCore class documentation is missing :^( See the source in src/displays
 *********************************************************************************************/
void dsp_on_start(DspCore *dsp) {

}

/***********************************************
   Occurs when the display is initialized
 ***********************************************/
void dsp_on_init() {
  hello.init(5, " * ", 1, TFT_LINEHGHT*4+6, 0, ORANGE, TFT_BG);
  Serial.println(TFT_LINEHGHT*4+6);
}

/************************
   The loop cycle
 ************************/
void dsp_on_loop() {
  if (weatherRequest) {
    weatherRequest = false;
    hello.setText(weather);
  }
  if (display.mode == PLAYER) hello.loop();
}

/***********************************************
   Occurs when the display changes mode
 ***********************************************/
void dsp_on_newmode(displayMode_e newmode) {
  if (newmode == PLAYER) {
    hello.reset();
  } else {
    hello.lock();
  }
}

/************************
   Before print the clock
 ************************/
bool dsp_before_clock(DspCore *dsp, bool dots) {
  if (display.mode == PLAYER) {
    dsp->setFont();
    dsp->setTextSize(1);
    display.centerText(dsp->utf8Rus("Hello from plugin!", true), display.screenheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT * 2 - 2, PINK, TFT_BG);
  }
  return true; // false, if you need to disable the drawing of the clock
}

#endif
