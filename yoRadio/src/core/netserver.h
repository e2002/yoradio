#ifndef netserver_h
#define netserver_h
#include "Arduino.h"

#include "../AsyncWebServer/ESPAsyncWebServer.h"
#include "AsyncUDP.h"

enum requestType_e : uint8_t  { PLAYLIST=1, STATION=2, STATIONNAME=3, ITEM=4, TITLE=5, VOLUME=6, NRSSI=7, BITRATE=8, MODE=9, EQUALIZER=10, BALANCE=11, PLAYLISTSAVED=12, STARTUP=13, GETINDEX=14, GETACTIVE=15, GETSYSTEM=16, GETSCREEN=17, GETTIMEZONE=18, GETWEATHER=19, GETCONTROLS=20, DSPON=21, SDPOS=22, SDLEN=23, SDSNUFFLE=24, SDINIT=25, GETPLAYERMODE=26, CHANGEMODE=27 };
enum import_e      : uint8_t  { IMDONE=0, IMPL=1, IMWIFI=2 };
const char emptyfs_html[] PROGMEM = R"(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=0.25"><meta charset="UTF-8">
<link rel="icon" type="image/png" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAYFBMVEUAAADYw1PcyVjYxFTaxlXYxFTbx1bcyVjZxVXbyFfcyFfaxlbax1bcyVjcyVjbyFfbyFfZxVXaxlbbx1fcyFjcyVjbx1fZxVXcyFjcyVjax1bbyFfcyVjbyFfax1bWwVKMlHGzAAAAH3RSTlMA+wv0zu6dBeVqSryjMRaCU97Fjz8liNk5HbFdDnWsEHoUsAAAAeFJREFUWMPtlllyrDAMRS1P2NjMQzc9RPvf5Ut1IPYjDRbJR1KVnD8Z7i1ZsgXsh1JW3usrC9Ta+2og620DiCjaaY65U4AIpqLqBb7R3B5xJucYRpI+U7jgHwsVLgjSLu74DmSvMTdhQVMMHAYeBhiQFAO5Y3CiGFzWBhDilmKQ4zsqm5uwQGvkCRfsytFkJIOhWWo+vz8uCfWMRqEVAJwsn+PsKgFA+YJR4UWe50Oc1Gt8vrFfyGC19153+afUvVMA+ADAaH5QXhvA/wB3yEICfgAqsvys8BngiPor4AaSpM8BN7lQRrrAbcBSLvMeKqmvVhtYh8mxqjCi7Tnnk4YDKYzRy9DPA2Uy9CoYDBShsCrKitxCnUUnm7qHFwyUYTlOAXYHWxP0TTzBbm1UBGIPfMkDZRcMur1bFPdAxEQPXhI1TNLSj+HxK9l9u8H41RrcKQZub5THbdxA7M3WAZL/EvRp0PDPGEgM9CxBqo9mYMcpAAPyzNZMx2aysUUWzYSi7lzSwALGGG3rvO/zurajM4BQJh0aXAGglACYg2v6uw64h2ZJfOIcp2lxh4ZgkEncRjAKF8AtYCI53M2mQc1IlNrAM7lyZ0akHKURsVaokxuLYxfD6ot8w+nOFuyP5/wDsZKME0E1GogAAAAASUVORK5CYII=">
<title>ёRadio - WEB Board Uploader</title><style>html, body { margin: 0; padding: 0; height: 100%; } body{background-color:#000;color:#e3d25f;font-size:20px;display:flex;flex-direction:column;}
hr{margin:20px 0;border:0; border-top:#555 1px solid;} p{text-align:center;margin-bottom:10px;} section{max-width:500px; text-align:center;margin:0 auto 30px auto;padding:20px;flex:1;}
.hidden{display:none;}a { color: var(--accent-color); text-decoration: none; font-weight: bold } a:hover { text-decoration: underline }
#copy { text-align: center; padding: 14px; font-size: 14px; }
input[type=file]{color:#ccc;} input[type=file]::file-selector-button, input[type=submit]{border:2px solid #e3d25f;color:#000;padding:6px 16px;border-radius:25px;background-color:#e3d25f;margin:0 6px;cursor:pointer;}
input[type=submit]{font-size:18px;text-transform:uppercase;padding:8px 26px;margin-top:10px;font-family:Times;} span{color:#ccc} .flex{display:flex;justify-content: space-around;margin-top:10px;}
input[type=text],input[type=password]{width:170px;background:#272727;color:#e3d25f;padding:6px 12px;font-size:20px;border:#2d2d2d 1px solid;margin:4px 0 0 4px;border-radius:4px;outline:none;}
@media screen and (max-width:480px) {section{zoom:0.7;-moz-transform:scale(0.7);}}
</style>
<script type="text/javascript" src="/variables.js"></script>
</head><body>
<section>
<h2>ёRadio - WEB Board Uploader</h2>
<hr />
<span>Select <u>ALL</u> files from <i>yoRadio/data/www/</i><br />and upload them using the form below</span>
<hr />
<form action="/webboard" method="post" enctype="multipart/form-data">
<p><label for="www">www:</label> <input type="file" name="www" id="www" multiple></p>
<hr />
<span>-= OPTIONAL =-<br />You can also upload <i>playlist.csv</i><br />and <i>wifi.csv files</i> from your backup</span>
<p><label for="data">wifi:</label><input type="file" name="data" id="data" multiple></p>
<hr />
<p><input type="submit" name="submit" value="Upload Files"></p>
</form>
<div style="padding:10px 0 0;" id="wupload">
<hr />
<form name="wifiform" method="post" enctype="multipart/form-data">
<span>-= OPTIONAL =-<br />If you can't connect from PC to 192.168.4.1 address<br />setup WiFi connection first</span>
<div class="flex"><div><label for="ssid">ssid:</label><input type="text" id="ssid" name="ssid" value="" maxlength="30" autocomplete="off"></div>
<div><label for="pass">pass:</label><input type="password" id="pass" name="pass" value="" maxlength="40" autocomplete="off"></div>
</div>
<p><input type="submit" name="submit" value="Save Credentials"></p>
</form>
</div>
</section>
<div id="copy">powered by <a target="_blank" href="https://github.com/e2002/yoradio/">ёRadio</a><span id="version"></span></div>
</body>
<script>
document.wifiform.action = `/${formAction}`;
if(playMode=='player') document.getElementById("wupload").classList.add("hidden");
document.getElementById("version").innerHTML=` | v${yoVersion}`;
</script>
</html>
)";
const char index_html[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <meta name="theme-color" content="#e3d25f">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="default">
  <link rel="icon" type="image/png" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAYFBMVEUAAADYw1PcyVjYxFTaxlXYxFTbx1bcyVjZxVXbyFfcyFfaxlbax1bcyVjcyVjbyFfbyFfZxVXaxlbbx1fcyFjcyVjbx1fZxVXcyFjcyVjax1bbyFfcyVjbyFfax1bWwVKMlHGzAAAAH3RSTlMA+wv0zu6dBeVqSryjMRaCU97Fjz8liNk5HbFdDnWsEHoUsAAAAeFJREFUWMPtlllyrDAMRS1P2NjMQzc9RPvf5Ut1IPYjDRbJR1KVnD8Z7i1ZsgXsh1JW3usrC9Ta+2og620DiCjaaY65U4AIpqLqBb7R3B5xJucYRpI+U7jgHwsVLgjSLu74DmSvMTdhQVMMHAYeBhiQFAO5Y3CiGFzWBhDilmKQ4zsqm5uwQGvkCRfsytFkJIOhWWo+vz8uCfWMRqEVAJwsn+PsKgFA+YJR4UWe50Oc1Gt8vrFfyGC19153+afUvVMA+ADAaH5QXhvA/wB3yEICfgAqsvys8BngiPor4AaSpM8BN7lQRrrAbcBSLvMeKqmvVhtYh8mxqjCi7Tnnk4YDKYzRy9DPA2Uy9CoYDBShsCrKitxCnUUnm7qHFwyUYTlOAXYHWxP0TTzBbm1UBGIPfMkDZRcMur1bFPdAxEQPXhI1TNLSj+HxK9l9u8H41RrcKQZub5THbdxA7M3WAZL/EvRp0PDPGEgM9CxBqo9mYMcpAAPyzNZMx2aysUUWzYSi7lzSwALGGG3rvO/zurajM4BQJh0aXAGglACYg2v6uw64h2ZJfOIcp2lxh4ZgkEncRjAKF8AtYCI53M2mQc1IlNrAM7lyZ0akHKURsVaokxuLYxfD6ot8w+nOFuyP5/wDsZKME0E1GogAAAAASUVORK5CYII=">
  <link rel="stylesheet" href="theme.css" type="text/css" />
  <link rel="stylesheet" href="style.css" type="text/css" />
  <script type="text/javascript" src="variables.js"></script>
  <script type="text/javascript" src="script.js"></script>
  <script type="text/javascript" src="dragpl.js"></script>
  </head>
<body>
<div id="content" class="hidden progmem">
</div><!--content-->
<div id="progress"><span id="loader"></span></div>
</body>
</html>
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
    bool begin(bool quiet=false);
    void loop();
    void requestOnChange(requestType_e request, uint8_t clientId);
    void setRSSI(int val) { rssi = val; };
    int  getRSSI()        { return rssi; };
    void chunkedHtmlPage(const String& contentType, AsyncWebServerRequest *request, const char * path, bool doproc = true);
    void onWsMessage(void *arg, uint8_t *data, size_t len, uint8_t clientId);
    bool irRecordEnable;
#if IR_PIN!=255
    void irToWs(const char* protocol, uint64_t irvalue);
    void irValsToWs(); 
#endif
		void resetQueue();
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
    int _readPlaylistLine(File &file, char * line, size_t size);
};

extern NetServer netserver;

#endif
