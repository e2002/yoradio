#ifndef NEXTION_H
#define NEXTION_H

//#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include "../../display.h"

#define TXBUFLEN  255
#define RXBUFLEN  50
//#define NEXTION_BAUD  38400
//#define NEXTION_BAUD    74880
#define NEXTION_BAUD  115200

#define ICON_NA       44
#define ICON_AAC      ICON_NA+1
#define ICON_FLAC     ICON_NA+2
#define ICON_MP3      ICON_NA+3
#define ICON_WAV      ICON_NA+4

#define WEATHER_REQUEST_INTERVAL          1800 //30min
#define WEATHER_REQUEST_INTERVAL_FAULTY   30

class Nextion {
  private:
    char          txbuf[TXBUFLEN];
    char          rxbuf[RXBUFLEN];
    uint16_t      rx_pos;
    int           _tchY;
    char          _espcoreversion[16];
    TaskHandle_t  _TaskCore0=NULL;
    QueueHandle_t _displayQueue=NULL;
    bool          _dummyDisplay;
    bool          _volInside;
    Ticker        _timer;
    unsigned long _volDelay;
    void createCore0Task();
    void processQueue();
    void drawVU();
  public:
    displayMode_e mode;
    bool dt;
    TaskHandle_t weatherUpdateTaskHandle;
//    bool weatherRequest;
  public:
    Nextion();
    void  begin(bool dummy=false);
    void  start();
    void  apScreen();
    void  loop();
    void  putcmd(const char* cmd);
    void  putcmd(const char* cmd, const char* val, uint16_t dl=0);
    void  putcmd(const char* cmd, int val, bool toString=false, uint16_t dl=0);
    void  putcmdf(const char* fmt, int val, uint16_t dl=0);
    void  bootString(const char* bs);
    void  newTitle(const char* title);
    void  newNameset(const char* meta);
    void  setVol(uint8_t vol, bool dialog);
    void  fillVU(uint8_t LC, uint8_t RC);
    char* utf8Rus(char* str, bool uppercase);
    void  printClock(struct tm timeinfo);
    void  bitrate(int bpm);
    void  bitratePic(uint8_t pic);
    void  rssi();
    void  weatherVisible(uint8_t vis);
    void  localTime(struct tm timeinfo);
    void  drawPlaylist(uint16_t currentPlItem);
    void  swichMode(displayMode_e newmode);
    void  drawNextStationNum(uint16_t num);
    void  putRequest(requestParams_t request);
    void  startWeather();
    bool  getForecast();
    static void  updateWeather();
    static void  getWeather(void * pvParameters);
    void sleep();
    void wake();
};

extern Nextion nextion;

#endif
