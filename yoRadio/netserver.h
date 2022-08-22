#ifndef netserver_h
#define netserver_h
#include "Arduino.h"

#include "ESPAsyncWebServer.h"
#include "AsyncUDP.h"

enum requestType_e : uint8_t  { PLAYLIST=1, STATION=2, ITEM=3, TITLE=4, VOLUME=5, NRSSI=6, BITRATE=7, MODE=8, EQUALIZER=9, BALANCE=10, PLAYLISTSAVED=11, GETMODE=12, GETINDEX=13, GETACTIVE=14, GETSYSTEM=15, GETSCREEN=16, GETTIMEZONE=17, GETWEATHER=18, GETCONTROLS=19, DSPON=20 };
enum htmlPath_e    : uint8_t  { PINDEX=1, PSETTINGS=2, PUPDATE=3, PIR=4, PPLAYLIST=5, PSSIDS=6 };

class NetServer {
  public:
    uint8_t playlistrequest; // ClientId want the playlist
    bool importRequest;
    bool resumePlay;
    htmlPath_e htmlPath;
  public:
    NetServer() {};
    bool begin();
    void loop();
    void requestOnChange(requestType_e request, uint8_t clientId);
    void setRSSI(int val);
    void onWsMessage(void *arg, uint8_t *data, size_t len, uint8_t clientId);
    bool savePlaylist(const char* post);
    void takeMallocDog();
    void giveMallocDog();
    uint32_t max, htmlpos;
    bool theend;
#if IR_PIN!=255
    bool irRecordEnable;
    void irToWs(const char* protocol, uint64_t irvalue);
    void irValsToWs();
    void chunkedHtmlPage(const String& contentType, AsyncWebServerRequest *request);
#endif
  private:
    requestType_e request;
    int rssi;
    
    void getPlaylist(uint8_t clientId);
    bool importPlaylist();
};

extern NetServer netserver;

#endif
