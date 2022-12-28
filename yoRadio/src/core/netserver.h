#ifndef netserver_h
#define netserver_h
#include "Arduino.h"

#include "ESPAsyncWebServer.h"
#include "AsyncUDP.h"

enum requestType_e : uint8_t  { PLAYLIST=1, STATION=2, STATIONNAME=3, ITEM=4, TITLE=5, VOLUME=6, NRSSI=7, BITRATE=8, MODE=9, EQUALIZER=10, BALANCE=11, PLAYLISTSAVED=12, GETMODE=13, GETINDEX=14, GETACTIVE=15, GETSYSTEM=16, GETSCREEN=17, GETTIMEZONE=18, GETWEATHER=19, GETCONTROLS=20, DSPON=21, SDPOS=22, SDLEN=23, SDSNUFFLE=24 };
enum import_e      : uint8_t  { IMDONE=0, IMPL=1, IMWIFI=2 };

struct nsRequestParams_t
{
  requestType_e type;
  uint8_t clientId;
};

class NetServer {
  public:
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
    QueueHandle_t nsQueue;
    int rssi;
    void getPlaylist(uint8_t clientId);
    bool importPlaylist();
    static size_t chunkedHtmlPageCallback(uint8_t* buffer, size_t maxLen, size_t index);
    static void beginUpload(AsyncWebServerRequest *request);
    static void beginUpdate(AsyncWebServerRequest *request);
    void processQueue();
};

extern NetServer netserver;

#endif
