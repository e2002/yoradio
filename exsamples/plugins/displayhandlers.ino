/**************************************************************

    An example of displaying user information on the display.
    This file must be in the root directory of the sketch.

**************************************************************/

#if (DSP_MODEL==DSP_ST7735) || (DSP_MODEL==DSP_ST7789) || (DSP_MODEL==DSP_SSD1327) || (DSP_MODEL==DSP_ILI9341) || (DSP_MODEL==DSP_ILI9225)

#define WEATHER_REQUEST_INTERVAL          1800 //30min
#define WEATHER_REQUEST_INTERVAL_FAULTY   30

#include <WiFiClient.h>
#include <Ticker.h>

const char* host  = "api.openweathermap.org";
const char* lat   = "55.7512";
const char* lon   = "37.6184";
const char* key   = "********************************";

Ticker ticker;

/***********************************************
   scrolled line
 ***********************************************/
Scroll hello;

char weather[254] = { 0 };
bool weatherRequest = false;
TaskHandle_t weatherUpdateTaskHandle;

bool getForecast() {
  WiFiClient client;
  if (!client.connect(host, 80)) {
    Serial.println("## OPENWEATHERMAP ###: connection  failed");
    return false;
  }
  char httpget[250] = {0};
  sprintf(httpget, "GET /data/2.5/weather?lat=%s&lon=%s&units=metric&lang=ru&appid=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", lat, lon, key, host);
  client.print(httpget);
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 2000UL) {
      Serial.println("## OPENWEATHERMAP ###: client available timeout !");
      client.stop();
      return false;
    }
  }
  timeout = millis();
  String line = "";
  if (client.connected()) {
    while (client.available())
    {
      line = client.readStringUntil('\n');
      if (strstr(line.c_str(), "\"temp\"") != NULL) {
        client.stop();
        break;
      }
      if ((millis() - timeout) > 500)
      {
        client.stop();
        Serial.println("## OPENWEATHERMAP ###: client read timeout !");
        return false;
      }
    }
  }
  if (strstr(line.c_str(), "\"temp\"") == NULL) {
    Serial.println("## OPENWEATHERMAP ###: weather not found !");
    return false;
  }
  char *tmpe;
  char *tmps;
  const char* cursor = line.c_str();
  char desc[120], temp[20], hum[20], press[20];

  tmps = strstr(cursor, "\"description\":\"");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: description not found !"); return false;}
  tmps += 15;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: description not found !"); return false;}
  strlcpy(desc, tmps, tmpe - tmps + 1);
  cursor = tmpe + 3;

  tmps = strstr(cursor, "\"temp\":");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: temp not found !"); return false;}
  tmps += 7;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: temp not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  float tempf = atof(temp);

  tmps = strstr(cursor, "\"pressure\":");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: pressure not found !"); return false;}
  tmps += 11;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: pressure not found !"); return false;}
  strlcpy(press, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  int pressi = (float)atoi(press) / 1.333;

  tmps = strstr(cursor, "humidity\":");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: humidity not found !"); return false;}
  tmps += 10;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: humidity not found !"); return false;}
  strlcpy(hum, tmps, tmpe - tmps + 1);

  Serial.printf("## OPENWEATHERMAP ###: description: %s, temp:%.1f C, pressure:%dmmHg, humidity:%s%%\n", desc, tempf, pressi, hum);
  sprintf(weather, "%s, %.1f C * давление: %d мм * влажность: %s%%", desc, tempf, pressi, hum);

  return true;

}

void getWeather( void * pvParameters ) {
  delay(200);
  if (getForecast()) {
    weatherRequest = true;
    ticker.detach();
    ticker.attach(WEATHER_REQUEST_INTERVAL, updateWeather);
  } else {
    ticker.detach();
    ticker.attach(WEATHER_REQUEST_INTERVAL_FAULTY, updateWeather);
  }
  vTaskDelete( NULL );
}

void updateWeather() {
  xTaskCreatePinnedToCore(
    getWeather,                   /* Task function. */
    "getWeather1",                /* name of task. */
    1024 * 4,                     /* Stack size of task */
    NULL,                         /* parameter of the task */
    0,                            /* priority of the task */
    &weatherUpdateTaskHandle,     /* Task handle to keep track of created task */
    0);                           /* pin task to core CORE_FOR_LOOP_CONTROLS */
}

/***********************************************
   Occurs when the network is connected
 ***********************************************/
void network_on_connect() {
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
  if (DSP_MODEL == DSP_ST7735 || (DSP_MODEL == DSP_SSD1327)) {
    hello.init(5, " * ", 1, TFT_LINEHGHT * 4 + 6, 0, config.theme.weather, config.theme.background);
  }else if(DSP_MODEL == DSP_ILI9225){
    hello.init(5, " * ", 1, TFT_LINEHGHT * 6 + 5, 0, config.theme.weather, config.theme.background);
  } else {
    hello.init(5, " * ", 2, TFT_LINEHGHT * 9 + 5, 0, config.theme.weather, config.theme.background);
  }
}

/************************
   The loop cycle
 ************************/
void dsp_on_loop(DspCore *dsp) {
  if (weatherRequest) {
    weatherRequest = false;
    hello.setText(dsp->utf8Rus(weather, true));
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
    display.centerText(dsp->utf8Rus("Hello from plugin!", true), display.screenheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT * 2 - 2, 0xF97F, config.theme.background);
  }
  return true; // false, if you need to disable the drawing of the clock
}

#endif
