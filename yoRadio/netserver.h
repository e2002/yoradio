#ifndef netserver_h
#define netserver_h
#include "Arduino.h"

#include "ESPAsyncWebServer.h"
#include "AsyncUDP.h"

enum requestType_e { PLAYLIST, STATION, ITEM, TITLE, VOLUME, NRSSI, BITRATE, MODE, EQUALIZER, BALANCE, PLAYLISTSAVED };

class NetServer {
  public:
    uint8_t playlistrequest; // ClientId want the playlist
    bool importRequest;
  public:
    NetServer() {};
    bool begin();
    void loop();
    void requestOnChange(requestType_e request, uint8_t clientId);
    void setRSSI(int val);
    void onWsMessage(void *arg, uint8_t *data, size_t len);
    bool savePlaylist(const char* post);
  private:
    requestType_e request;
    int rssi;
    void getPlaylist(uint8_t clientId);
    bool importPlaylist();
};

extern NetServer netserver;

#endif
