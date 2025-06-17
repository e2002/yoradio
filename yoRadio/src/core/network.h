#ifndef network_h
#define network_h
#include <Ticker.h>
#include "time.h"
#include "WiFi.h"
#include "rtcsupport.h"

#define apSsid      "yoRadioAP"
#define apPassword  ""
//#define TSYNC_DELAY 10800000    // 1000*60*60*3 = 3 hours
#define TSYNC_DELAY       3600000     // 1000*60*60   = 1 hour
#define WEATHER_STRING_L  254

enum n_Status_e { CONNECTED, SOFT_AP, FAILED, SDREADY };

class MyNetwork {
  public:
    n_Status_e status;
    struct tm timeinfo;
    bool firstRun, forceTimeSync, forceWeather;
    bool lostPlaying = false, beginReconnect = false;
    //uint8_t tsFailCnt, wsFailCnt;
    Ticker ctimer;
    char *weatherBuf;
    bool trueWeather;
  public:
    MyNetwork() {};
    void begin();
    void requestTimeSync(bool withTelnetOutput=false, uint8_t clientId=0);
    void requestWeatherSync();
    void setWifiParams();
    bool wifiBegin(bool silent=false);
  private:
    Ticker rtimer;
    void raiseSoftAP();
    static void WiFiLostConnection(WiFiEvent_t event, WiFiEventInfo_t info);
    static void WiFiReconnected(WiFiEvent_t event, WiFiEventInfo_t info);
};

extern MyNetwork network;

extern __attribute__((weak)) void network_on_connect();

#endif
