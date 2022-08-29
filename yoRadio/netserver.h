#ifndef netserver_h
#define netserver_h
#include "Arduino.h"

#include "ESPAsyncWebServer.h"
#include "AsyncUDP.h"

enum requestType_e : uint8_t  { PLAYLIST=1, STATION=2, ITEM=3, TITLE=4, VOLUME=5, NRSSI=6, BITRATE=7, MODE=8, EQUALIZER=9, BALANCE=10, PLAYLISTSAVED=11, GETMODE=12, GETINDEX=13, GETACTIVE=14, GETSYSTEM=15, GETSCREEN=16, GETTIMEZONE=17, GETWEATHER=18, GETCONTROLS=19, DSPON=20 };
enum import_e      : uint8_t  { IMDONE=0, IMPL=1, IMWIFI=2 };

#define PLOW() //player.setBufsize(1600*2, -1); vTaskDelay(150)
#define PHIG() //vTaskDelay(150); player.setBufsize(1600*AUDIOBUFFER_MULTIPLIER2, -1)

class NetServer {
  public:
    //uint8_t playlistrequest; // ClientId want the playlist/* Cleanup this */
    import_e importRequest;
    bool resumePlay;
    char chunkedPathBuffer[40];
  public:
    NetServer() {};
    bool begin();
    void loop();
    void requestOnChange(requestType_e request, uint8_t clientId);
    void setRSSI(int val) { rssi = val; };
    void chunkedHtmlPage(const String& contentType, AsyncWebServerRequest *request, const char * path, bool gzip = false);
    void onWsMessage(void *arg, uint8_t *data, size_t len, uint8_t clientId);
#if IR_PIN!=255
    bool irRecordEnable;
    void irToWs(const char* protocol, uint64_t irvalue);
    void irValsToWs(); 
#endif
  private:
    requestType_e request;
    int rssi;
    void getPlaylist(uint8_t clientId);
    bool importPlaylist();
    static size_t chunkedHtmlPageCallback(uint8_t* buffer, size_t maxLen, size_t index);
    static void beginUpload(AsyncWebServerRequest *request);
    static void beginUpdate(AsyncWebServerRequest *request);
};

extern NetServer netserver;

#endif
