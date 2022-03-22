/**************************************************************
*
*   An example of displaying user information on the display.
*   This file must be in the root directory of the sketch.
*
**************************************************************/

#if DSP_MODEL==DSP_ST7735

#include <JSON_Decoder.h>                                     // https://github.com/Bodmer/OpenWeather
#include <OpenWeather.h>                                      // https://github.com/Bodmer/JSON_Decoder

String api_key = "********************************";          // openweathermap.org API key

String latitude =  "55.7512";
String longitude = "37.6184";

String units = "metric";
String language = "ru";

OW_Weather ow;

/***********************************************
 * scrolled line
 ***********************************************/
Scroll hello;


/***********************************************
 * Occurs when the display is initialized
 ***********************************************/
void dsp_on_init(){
  hello.init(" * ", 1, TFT_LINEHGHT*4+6, 2000, ORANGE, TFT_BG);
}

/*********************************************************************************************
 * The display has initialized, the network is connected, the player is ready to play.
 * DspCore class documentation is missing :^( See the source in src/displays
 *********************************************************************************************/
void dsp_on_start(DspCore *dsp){
  OW_current *current = new OW_current;
  OW_hourly *hourly = new OW_hourly;
  OW_daily  *daily = new OW_daily;
  char weather[140] = { 0 };
  ow.getForecast(current, hourly, daily, api_key, latitude, longitude, units, language);

  sprintf(weather, "temp: %.1f * pressure: %d * humidity: %d", current->temp, (int)(current->pressure/1.333), current->humidity);

  hello.setText(dsp->utf8Rus(weather, true));
}

/************************
 * The loop cycle
 ************************/
void dsp_on_loop(){
  if(display.mode==PLAYER) hello.loop();
}

/***********************************************
 * Occurs when the display changes mode
 ***********************************************/
void dsp_on_newmode(displayMode_e newmode){
  if (newmode == PLAYER) {
    hello.reset();
  }else{
    hello.lock();
  }
}

/************************
 * Before print the clock
 ************************/
bool dsp_before_clock(DspCore *dsp, bool dots){
  if(display.mode==PLAYER){
    dsp->setFont();
    dsp->setTextSize(1);
    display.centerText(dsp->utf8Rus("Hello from plugin!", true), display.screenheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT * 2 - 2, PINK, TFT_BG);
  }
  return true; // false, if you need to disable the drawing of the clock
}

#endif
