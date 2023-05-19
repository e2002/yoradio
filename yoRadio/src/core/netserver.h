#ifndef netserver_h
#define netserver_h
#include "Arduino.h"

#include "../AsyncWebServer/ESPAsyncWebServer.h"
#include "AsyncUDP.h"

enum requestType_e : uint8_t  { PLAYLIST=1, STATION=2, STATIONNAME=3, ITEM=4, TITLE=5, VOLUME=6, NRSSI=7, BITRATE=8, MODE=9, EQUALIZER=10, BALANCE=11, PLAYLISTSAVED=12, GETMODE=13, GETINDEX=14, GETACTIVE=15, GETSYSTEM=16, GETSCREEN=17, GETTIMEZONE=18, GETWEATHER=19, GETCONTROLS=20, DSPON=21, SDPOS=22, SDLEN=23, SDSNUFFLE=24, SDINIT=25, GETPLAYERMODE=26, CHANGEMODE=27 };
enum import_e      : uint8_t  { IMDONE=0, IMPL=1, IMWIFI=2 };
const char emptyfs_html[] PROGMEM = R"(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=0.25"><meta charset="UTF-8"><link rel="icon" href="data:;base64,iVBORw0KGgo="><title>ёRadio - WEB Board Uploader</title><style>body{background-color:#000;color:#e3d25f;font-size:20px;}
hr{margin:20px 0;border:0; border-top:#555 1px solid;} p{text-align:center;margin-bottom:10px;} section{max-width:500px; text-align:center;margin:0 auto 30px auto;}
input[type=file]{color:#ccc;} input[type=file]::file-selector-button, input[type=submit]{border:2px solid #e3d25f;color:#000;padding:6px 16px;border-radius:25px;background-color:#e3d25f;margin:0 6px;cursor:pointer;}
input[type=submit]{font-size:18px;text-transform:uppercase;padding:8px 26px;margin-top:10px;font-family:Times;} span{color:#ccc} .flex{display:flex;justify-content: space-around;margin-top:10px;}
input[type=text],input[type=password]{width:170px;background:#272727;color:#e3d25f;padding:6px 12px;font-size:20px;border:#2d2d2d 1px solid;margin:4px 0 0 4px;border-radius:4px;outline:none;}
@media screen and (max-width:480px) {section{zoom:0.7;-moz-transform:scale(0.7);}}
</style></head><body>
<section>
<h2>ёRadio - WEB Board Uploader</h2>
<hr />
<span>Select <u>ALL</u> files from <i>yoRadio/data/www/</i><br />and upload them using the form below</span>
<hr />
<form action="/%ACTION%" method="post" enctype="multipart/form-data">
<p><label for="www">www:</label> <input type="file" name="www" id="www" multiple></p>
<hr />
<span>-= OPTIONAL =-<br />You can also upload <i>playlist.csv</i><br />and <i>wifi.csv files</i> from your backup</span>
<p><label for="data">wifi:</label><input type="file" name="data" id="data" multiple></p>
<hr />
<p><input type="submit" name="submit" value="Upload Files"></p>
</form>
<div style="padding:10px 0 0;"%UPLOADWIFI%>
<hr />
<form action="/%ACTION%" method="post" enctype="multipart/form-data">
<span>-= OPTIONAL =-<br />If you can't connect from PC to 192.168.4.1 address<br />setup WiFi connection first</span>
<div class="flex"><div><label for="ssid">ssid:</label><input type="text" id="ssid" name="ssid" value="" maxlength="30" autocomplete="off"></div>
<div><label for="pass">pass:</label><input type="password" id="pass" name="pass" value="" maxlength="40" autocomplete="off"></div>
</div>
<p><input type="submit" name="submit" value="Save Credentials"></p>
</form>
</div>
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
    int  getRSSI()        { return rssi; };
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
    int rssi, newConfigMode;
    void getPlaylist(uint8_t clientId);
    bool importPlaylist();
    static size_t chunkedHtmlPageCallback(uint8_t* buffer, size_t maxLen, size_t index);
    static void beginUpload(AsyncWebServerRequest *request);
    static void beginUpdate(AsyncWebServerRequest *request);
    void processQueue();
};

extern NetServer netserver;

#endif
