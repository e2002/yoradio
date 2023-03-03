#ifndef netserver_h
#define netserver_h
#include "Arduino.h"

#include "../AsyncWebServer/ESPAsyncWebServer.h"
#include "AsyncUDP.h"

enum requestType_e : uint8_t  { PLAYLIST=1, STATION=2, STATIONNAME=3, ITEM=4, TITLE=5, VOLUME=6, NRSSI=7, BITRATE=8, MODE=9, EQUALIZER=10, BALANCE=11, PLAYLISTSAVED=12, GETMODE=13, GETINDEX=14, GETACTIVE=15, GETSYSTEM=16, GETSCREEN=17, GETTIMEZONE=18, GETWEATHER=19, GETCONTROLS=20, DSPON=21, SDPOS=22, SDLEN=23, SDSNUFFLE=24 };
enum import_e      : uint8_t  { IMDONE=0, IMPL=1, IMWIFI=2 };
const char emptyfs_html[] PROGMEM = R"(
<!DOCTYPE html><html><head><meta charset="UTF-8"><title>ёRadio - WEB Board Uploader</title><style>body {background-color: #000; color: #e3d25f; font-size: 20px;}
hr {margin: 20px 0; border: 0; border-top: #555 1px solid;} p {text-align: center;margin-bottom: 10px;} section {max-width: 500px; text-align: center;margin:0 auto;}
input[type=file] {color: #ccc;} input[type=file]::file-selector-button, input[type=submit] {border: 2px solid #e3d25f;color: #ccc;padding: 6px 16px;border-radius: 15px;background-color: #000;margin: 0 6px;}
span{color:#ccc}</style></head><body>
<section>
<h2>ёRadio - WEB Board Uploader</h2>
<hr />
<span>Please select <u>ALL</u> files from <i>yoRadio/data/www/</i> and upload them using the form below</span>
<hr />
<form action="/%ACTION%" method="post" enctype="multipart/form-data">
<p><label for="www">www:</label> <input type="file" name="www" id="www" multiple></p>
<hr />
<span>Optional, you can also upload <i>playlist.csv</i> and <i>wifi.csv files</i> from backup</span>
<p><label for="data">wifi:</label><input type="file" name="data" id="data" multiple></p>
<hr />
<p><input type="submit" name="submit" value="Upload"></p>
</form>
</section>
</body></html>
)";

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
    bool irRecordEnable;
#if IR_PIN!=255
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
