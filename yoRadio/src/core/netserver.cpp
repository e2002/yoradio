#include "netserver.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "../ESPFileUpdater/ESPFileUpdater.h"
#include "config.h"
#include "player.h"
#include "telnet.h"
#include "display.h"
#include "options.h"
#include "network.h"
#include "mqtt.h"
#include "controls.h"
#include "commandhandler.h"
#include <Update.h>

//#include <Ticker.h>
#if USE_OTA
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#include <NetworkUdp.h>
#else
#include <WiFiUdp.h>
#endif
#include <ArduinoOTA.h>
#endif

#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <freertos/FreeRTOS.h>
#ifdef USE_SD
  #include "sdmanager.h"
#endif
#ifndef MIN_MALLOC
  #define MIN_MALLOC 24112
#endif
#ifndef NSQ_SEND_DELAY
  //#define NSQ_SEND_DELAY       portMAX_DELAY
  #define NSQ_SEND_DELAY       pdMS_TO_TICKS(300)
#endif
#ifndef NS_QUEUE_TICKS
  //#define NS_QUEUE_TICKS pdMS_TO_TICKS(2)
  #define NS_QUEUE_TICKS 0
#endif

// Global list for radio-browser servers to persist across searches
String g_ipv4_servers[20];
// For the search task
TaskHandle_t g_searchTaskHandle = NULL;
#define FS_REQUIRED_FREE_SPACE 150 // in KB - must be minimum x1.5 of the limit_per_page in search.js (100)

//#define CORS_DEBUG //Enable CORS policy: 'Access-Control-Allow-Origin' (for testing)

NetServer netserver;

AsyncWebServer webserver(80);
AsyncWebSocket websocket("/ws");

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void handleIndex(AsyncWebServerRequest * request);
void handleNotFound(AsyncWebServerRequest * request);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void vTaskSearchRadioBrowser(void *pvParameters);
void handleSearchPost(AsyncWebServerRequest *request);
#ifdef UPDATEURL
void checkForOnlineUpdate();
void startOnlineUpdate();
#endif

bool  shouldReboot  = false;
#ifdef MQTT_ROOT_TOPIC
//Ticker mqttplaylistticker;
bool  mqttplaylistblock = false;
void mqttplaylistSend() {
  mqttplaylistblock = true;
//  mqttplaylistticker.detach();
  mqttPublishPlaylist();
  mqttplaylistblock = false;
}
#endif

char* updateError() {
  sprintf(netserver.nsBuf, "Update failed with error (%d)<br /> %s", (int)Update.getError(), Update.errorString());
  return netserver.nsBuf;
}

void handleSearch(AsyncWebServerRequest *request) {
  // handle search request
  if (request->hasParam("search")) {
    if (g_searchTaskHandle != NULL) {
      request->send(429, "text/plain", "Search task is already running.");
      return;
    }
    String searchQuery = request->getParam("search")->value();
    char* search_str = new (std::nothrow) char[searchQuery.length() + 1];
    if (!search_str) {
      request->send(500, "text/plain", "Failed to allocate memory for search task.");
      return;
    }
    strcpy(search_str, searchQuery.c_str());
    xTaskCreate(vTaskSearchRadioBrowser, "searchRadioBrowser", 8192, (void*)search_str, 1, &g_searchTaskHandle);
    request->send(200, "application/json", "{\"status\":\"searching\"}");
  }
}

void handleSearchPost(AsyncWebServerRequest *request) {
  // handle preview or add to playlist
  bool addtoplaylist = false;
  if (request->hasParam("addtoplaylist", true)) {
    if (request->getParam("addtoplaylist", true)->value() == "true") addtoplaylist = true;
  }
  if (!request->hasParam("url", true) || !request->hasParam("name", true)) {
    request->send(400, "text/plain", "Missing url or name");
    return;
  }
  String sUrl = request->getParam("url", true)->value();
  String sName = request->getParam("name", true)->value();
  sName.trim();
  sUrl.trim();
  if (sName.length() >= sizeof(config.station.name)) sName = sName.substring(0, sizeof(config.station.name) - 1);
  if (sUrl.length() >= sizeof(config.station.url)) sUrl = sUrl.substring(0, sizeof(config.station.url) - 1);
  if (!addtoplaylist) { // This is a preview
    config.loadStation(0); // Load into temporary station slot
    launchPlaybackTask(sUrl, sName);
    netserver.requestOnChange(GETINDEX, 0);
    request->send(200, "text/plain", "PREVIEW");
  } else { // This is add to playlist
    int sOvol = 0;
    // Check for duplicate URL before adding
    bool found = false;
    int foundIdx = 0;
    auto normalizeUrl = [](const String& url) -> String {
        String u = url;
        u.trim();
        if (u.startsWith("http://")) u = u.substring(7);
        else if (u.startsWith("https://")) u = u.substring(8);
        u.trim();
        return u;
        };
    String normNewUrl = normalizeUrl(sUrl);
    uint16_t cs = config.playlistLength();
    for (int i = 1; i <= cs; ++i) {
      config.loadStation(i);
      String existingUrl = String(config.station.url);
      String normExistingUrl = normalizeUrl(existingUrl);
      if (normExistingUrl.equalsIgnoreCase(normNewUrl)) {
        found = true;
        foundIdx = i;
        break;
      }
    }
    if (found) { // play the slot if it already exists
      player.sendCommand({PR_PLAY, (uint16_t)foundIdx});
      request->send(200, "text/plain", "DUPLICATE");
    } else { // add it and play it
      File playlistfile = SPIFFS.open(PLAYLIST_PATH, "a");
      if (playlistfile) {
        playlistfile.printf("%s\t%s\t%d\r\n", sName.c_str(), sUrl.c_str(), sOvol);
        playlistfile.close();
        uint16_t newIdx = cs + 1;
        config.indexPlaylist();
        config.initPlaylist();
        player.sendCommand({PR_PLAY, newIdx});
        netserver.requestOnChange(PLAYLISTSAVED, 0);
        request->send(200, "text/plain", "ADDED");
      } else {
        request->send(500, "text/plain", "Failed to open playlist file");
      }
    }
  }
}

bool NetServer::begin(bool quiet) {
  if(network.status==SDREADY) return true;
  if(!quiet) Serial.print("##[BOOT]#\tnetserver.begin\t");
  importRequest = IMDONE;
  irRecordEnable = false;
  nsQueue = xQueueCreate( 20, sizeof( nsRequestParams_t ) );
  while(nsQueue==NULL){;}

  webserver.on("/", HTTP_ANY, handleIndex);
  webserver.on("/search", HTTP_GET, handleSearch);
  webserver.on("/search", HTTP_POST, handleSearchPost);
  webserver.onNotFound(handleNotFound);
  webserver.onFileUpload(handleUpload);

  webserver.serveStatic("/", SPIFFS, "/www/").setCacheControl("max-age=3600");
#ifdef CORS_DEBUG
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("content-type"));
#endif
  webserver.begin();
  //if(strlen(config.store.mdnsname)>0)
  //  MDNS.begin(config.store.mdnsname);
  websocket.onEvent(onWsEvent);
  webserver.addHandler(&websocket);
#if USE_OTA
  if(strlen(config.store.mdnsname)>0)
    ArduinoOTA.setHostname(config.store.mdnsname);
#ifdef OTA_PASS
  ArduinoOTA.setPassword(OTA_PASS);
#endif
  ArduinoOTA
    .onStart([]() {
      display.putRequest(NEWMODE, UPDATING);
      telnet.printf("Start OTA updating %s\n", ArduinoOTA.getCommand() == U_FLASH?"firmware":"filesystem");
    })
    .onEnd([]() {
      telnet.printf("\nEnd OTA update, Rebooting...\n");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      telnet.printf("Progress OTA: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      telnet.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        telnet.printf("Auth Failed\n");
      } else if (error == OTA_BEGIN_ERROR) {
        telnet.printf("Begin Failed\n");
      } else if (error == OTA_CONNECT_ERROR) {
        telnet.printf("Connect Failed\n");
      } else if (error == OTA_RECEIVE_ERROR) {
        telnet.printf("Receive Failed\n");
      } else if (error == OTA_END_ERROR) {
        telnet.printf("End Failed\n");
      }
    });
  ArduinoOTA.begin();
#endif

  if(!quiet) Serial.println("done");
  return true;
}

size_t NetServer::chunkedHtmlPageCallback(uint8_t* buffer, size_t maxLen, size_t index){
  File requiredfile;
  bool sdpl = strcmp(netserver.chunkedPathBuffer, PLAYLIST_SD_PATH) == 0;
  if(sdpl){
    requiredfile = config.SDPLFS()->open(netserver.chunkedPathBuffer, "r");
  }else{
    requiredfile = SPIFFS.open(netserver.chunkedPathBuffer, "r");
  }
  if (!requiredfile) return 0;
  size_t filesize = requiredfile.size();
  size_t needread = filesize - index;
  if (!needread) {
    requiredfile.close();
    display.unlock();
    return 0;
  }
  size_t canread = (needread > maxLen) ? maxLen : needread;
  DBGVB("[%s] seek to %d in %s and read %d bytes with maxLen=%d", __func__, index, netserver.chunkedPathBuffer, canread, maxLen);
  //netserver.loop();
  requiredfile.seek(index, SeekSet);
  requiredfile.read(buffer, canread);
  index += canread;
  if (requiredfile) requiredfile.close();
  return canread;
}

void NetServer::chunkedHtmlPage(const String& contentType, AsyncWebServerRequest *request, const char * path) {
  memset(chunkedPathBuffer, 0, sizeof(chunkedPathBuffer));
  strlcpy(chunkedPathBuffer, path, sizeof(chunkedPathBuffer)-1);
  AsyncWebServerResponse *response;
  #ifndef NETSERVER_LOOP1
  display.lock();
  #endif
  response = request->beginChunkedResponse(contentType, chunkedHtmlPageCallback);
  response->addHeader("Cache-Control","max-age=3600");
  request->send(response);
}

#ifndef DSP_NOT_FLIPPED
  #define DSP_CAN_FLIPPED true
#else
  #define DSP_CAN_FLIPPED false
#endif
#if !defined(HIDE_WEATHER) && (!defined(DUMMYDISPLAY) && !defined(USE_NEXTION))
  #define SHOW_WEATHER  true
#else
  #define SHOW_WEATHER  false
#endif

#ifndef NS_QUEUE_TICKS
  #define NS_QUEUE_TICKS 0
#endif

const char *getFormat(BitrateFormat _format) {
  switch (_format) {
    case BF_MP3:  return "MP3";
    case BF_AAC:  return "AAC";
    case BF_FLAC: return "FLC";
    case BF_OGG:  return "OGG";
    case BF_WAV:  return "WAV";
    case BF_VOR:  return "VOR";
    case BF_OPU:  return "OPU";
    default:      return "bitrate";
  }
}

void NetServer::processQueue(){
  if(nsQueue==NULL) return;
  nsRequestParams_t request;
  if(xQueueReceive(nsQueue, &request, NS_QUEUE_TICKS)){
    uint8_t clientId = request.clientId;
    wsBuf[0]='\0';
    switch (request.type) {
      case PLAYLIST:        getPlaylist(clientId); break;
      case PLAYLISTSAVED:   {
        #ifdef USE_SD
        if(config.getMode()==PM_SDCARD) {
        //  config.indexSDPlaylist();
          config.initSDPlaylist();
        }
        #endif
        if(config.getMode()==PM_WEB){
          config.indexPlaylist(); 
          config.initPlaylist(); 
        }
        getPlaylist(clientId); break;
      }
      case GETACTIVE: {
          bool dbgact = false, nxtn=false;
          //String act = F("\"group_wifi\",");
          nsBuf[0]='\0';
          APPEND_GROUP("group_wifi");
          if (network.status == CONNECTED) {
                                                                //act += F("\"group_system\",");
                                                                APPEND_GROUP("group_system");
            if (BRIGHTNESS_PIN != 255 || DSP_CAN_FLIPPED || DSP_MODEL == DSP_NOKIA5110 || dbgact)    APPEND_GROUP("group_display");
          #ifdef USE_NEXTION
                                                                APPEND_GROUP("group_nextion");
            if (!SHOW_WEATHER || dbgact)                        APPEND_GROUP("group_weather");
            nxtn=true;
          #endif
                                                              #if defined(LCD_I2C) || defined(DSP_OLED)
                                                                APPEND_GROUP("group_oled");
                                                              #endif
                                                              #if !defined(HIDE_VU) && !defined(DUMMYDISPLAY)
                                                                APPEND_GROUP("group_vu");
                                                              #endif
            if (BRIGHTNESS_PIN != 255 || nxtn || dbgact)        APPEND_GROUP("group_brightness");
            if (DSP_CAN_FLIPPED || dbgact)                      APPEND_GROUP("group_tft");
            if (TS_MODEL != TS_MODEL_UNDEFINED || dbgact)       APPEND_GROUP("group_touch");
            if (DSP_MODEL == DSP_NOKIA5110)                     APPEND_GROUP("group_nokia");
                                                                APPEND_GROUP("group_timezone");
            if (SHOW_WEATHER || dbgact)                         APPEND_GROUP("group_weather");
                                                                APPEND_GROUP("group_controls");
            if (ENC_BTNL != 255 || ENC2_BTNL != 255 || dbgact)  APPEND_GROUP("group_encoder");
            if (IR_PIN != 255 || dbgact)                        APPEND_GROUP("group_ir");
            if (!psramInit())                                   APPEND_GROUP("group_buffer");
                                                              #if RTCSUPPORTED
                                                                APPEND_GROUP("group_rtc");
                                                              #else
                                                                APPEND_GROUP("group_wortc");
                                                              #endif
          }
          size_t len = strlen(nsBuf);
          if (len > 0 && nsBuf[len - 1] == ',') nsBuf[len - 1] = '\0';
          
          snprintf(wsBuf, sizeof(wsBuf), "{\"act\":[%s]}", nsBuf);
          break;
        }
      case GETINDEX:      {
          requestOnChange(STATION, clientId); 
          requestOnChange(TITLE, clientId); 
          requestOnChange(VOLUME, clientId); 
          requestOnChange(EQUALIZER, clientId); 
          requestOnChange(BALANCE, clientId); 
          requestOnChange(BITRATE, clientId); 
          requestOnChange(MODE, clientId); 
          requestOnChange(SDINIT, clientId);
          requestOnChange(GETPLAYERMODE, clientId); 
          if (config.getMode()==PM_SDCARD) { requestOnChange(SDPOS, clientId); requestOnChange(SDLEN, clientId); requestOnChange(SDSNUFFLE, clientId); } 
          return; 
          break;
        }
      case GETSYSTEM:     sprintf (wsBuf, "{\"sst\":%d,\"aif\":%d,\"vu\":%d,\"softr\":%d,\"vut\":%d,\"mdns\":\"%s\",\"ipaddr\":\"%s\", \"abuff\": %d, \"telnet\": %d, \"watchdog\": %d }", 
                                  config.store.smartstart != 2, 
                                  config.store.audioinfo, 
                                  config.store.vumeter, 
                                  config.store.softapdelay,
                                  config.vuThreshold,
                                  config.store.mdnsname,
                                  config.ipToStr(WiFi.localIP()),
                                  config.store.abuff,
                                  config.store.telnet,
                                  config.store.watchdog); 
                                  break;
      case GETSCREEN:     sprintf (wsBuf, "{\"flip\":%d,\"inv\":%d,\"nump\":%d,\"tsf\":%d,\"tsd\":%d,\"dspon\":%d,\"br\":%d,\"con\":%d,\"scre\":%d,\"scrt\":%d,\"scrb\":%d,\"scrpe\":%d,\"scrpt\":%d,\"scrpb\":%d,\"volumepage\":%d,\"clock12\":%d}", 
                                  config.store.flipscreen, 
                                  config.store.invertdisplay, 
                                  config.store.numplaylist, 
                                  config.store.fliptouch, 
                                  config.store.dbgtouch, 
                                  config.store.dspon, 
                                  config.store.brightness, 
                                  config.store.contrast,
                                  config.store.screensaverEnabled,
                                  config.store.screensaverTimeout,
                                  config.store.screensaverBlank,
                                  config.store.screensaverPlayingEnabled,
                                  config.store.screensaverPlayingTimeout,
                                  config.store.screensaverPlayingBlank,
                                  config.store.volumepage, 
                                  config.store.clock12);
                                  break;
      case GETTIMEZONE:   sprintf (wsBuf, "{\"tz_name\":\"%s\",\"tzposix\":\"%s\",\"sntp1\":\"%s\",\"sntp2\":\"%s\", \"timeint\":%d,\"timeintrtc\":%d}", 
                                  config.store.tz_name, 
                                  config.store.tzposix, 
                                  config.store.sntp1, 
                                  config.store.sntp2,
                                  config.store.timeSyncInterval,
                                  config.store.timeSyncIntervalRTC); 
                                  break;
      case GETWEATHER:    sprintf (wsBuf, "{\"wen\":%d,\"wlat\":\"%s\",\"wlon\":\"%s\",\"wkey\":\"%s\",\"wint\":%d}", 
                                  config.store.showweather, 
                                  config.store.weatherlat, 
                                  config.store.weatherlon, 
                                  config.store.weatherkey,
                                  config.store.weatherSyncInterval); 
                                  break;
      case GETCONTROLS:   sprintf (wsBuf, "{\"vols\":%d,\"enca\":%d,\"irtl\":%d,\"skipup\":%d}", 
                                  config.store.volsteps, 
                                  config.store.encacc, 
                                  config.store.irtlp,
                                  config.store.skipPlaylistUpDown); 
                                  break;
      case DSPON:         sprintf (wsBuf, "{\"dspontrue\":%d}", 1); break;
      case STATION:       requestOnChange(STATIONNAME, clientId); requestOnChange(ITEM, clientId); break;
      case STATIONNAME:   sprintf (wsBuf, "{\"payload\":[{\"id\":\"nameset\", \"value\": \"%s\"}]}", config.station.name); break;
      case ITEM:          sprintf (wsBuf, "{\"current\": %d}", config.lastStation()); break;
      case TITLE:         sprintf (wsBuf, "{\"payload\":[{\"id\":\"meta\", \"value\": \"%s\"}]}", config.station.title); telnet.printf("##CLI.META#: %s\n> ", config.station.title); break;
      case VOLUME:        sprintf (wsBuf, "{\"payload\":[{\"id\":\"volume\", \"value\": %d}]}", config.store.volume); telnet.printf("##CLI.VOL#: %d\n", config.store.volume); break;
      case NRSSI:         sprintf (wsBuf, "{\"payload\":[{\"id\":\"rssi\", \"value\": %d}, {\"id\":\"heap\", \"value\": %d}]}", rssi, (player.isRunning() && config.store.audioinfo)?(int)(100*player.inBufferFilled()/playerBufMax):0); /*rssi = 255;*/ break;
      case SDPOS:         sprintf (wsBuf, "{\"sdpos\": %lu,\"sdend\": %lu,\"sdtpos\": %lu,\"sdtend\": %lu}", 
                                  player.getFilePos(), 
                                  player.getFileSize(), 
                                  player.getAudioCurrentTime(), 
                                  player.getAudioFileDuration()); 
                                  break;
      case SDLEN:         sprintf (wsBuf, "{\"sdmin\": %lu,\"sdmax\": %lu}", player.sd_min, player.sd_max); break;
      case SDSNUFFLE:     sprintf (wsBuf, "{\"snuffle\": %d}", config.store.sdsnuffle); break;
      case BITRATE:       sprintf (wsBuf, "{\"payload\":[{\"id\":\"bitrate\", \"value\": %d}, {\"id\":\"fmt\", \"value\": \"%s\"}]}", config.station.bitrate, getFormat(config.configFmt)); break;
      case MODE:          sprintf (wsBuf, "{\"payload\":[{\"id\":\"playerwrap\", \"value\": \"%s\"}]}", player.status() == PLAYING ? "playing" : "stopped"); telnet.info(); break;
      case EQUALIZER:     sprintf (wsBuf, "{\"payload\":[{\"id\":\"bass\", \"value\": %d}, {\"id\": \"middle\", \"value\": %d}, {\"id\": \"trebble\", \"value\": %d}]}", config.store.bass, config.store.middle, config.store.trebble); break;
      case BALANCE:       sprintf (wsBuf, "{\"payload\":[{\"id\": \"balance\", \"value\": %d}]}", config.store.balance); break;
      case SDINIT:        sprintf (wsBuf, "{\"sdinit\": %d}", SDC_CS!=255); break;
      case GETPLAYERMODE: sprintf (wsBuf, "{\"playermode\": \"%s\"}", config.getMode()==PM_SDCARD?"modesd":"modeweb"); break;
      #ifdef USE_SD
        case CHANGEMODE:    config.changeMode(config.newConfigMode); return; break;
      #endif
      default:          break;
    }
    if (strlen(wsBuf) > 0) {
      if (clientId == 0) { websocket.textAll(wsBuf); }else{ websocket.text(clientId, wsBuf); }
  #ifdef MQTT_ROOT_TOPIC
      if (clientId == 0 && (request.type == STATION || request.type == ITEM || request.type == TITLE || request.type == MODE)) mqttPublishStatus();
      if (clientId == 0 && request.type == VOLUME) mqttPublishVolume();
  #endif
    }
  }
}

void NetServer::loop() {
  if(network.status==SDREADY) return;
  if (shouldReboot) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
  processQueue();
  websocket.cleanupClients();
  switch (importRequest) {
    case IMPL:    importPlaylist();  importRequest = IMDONE; break;
    case IMWIFI:  config.saveWifi(); importRequest = IMDONE; break;
    default:      break;
  }
  //processQueue();
}

#if IR_PIN!=255
void NetServer::irToWs(const char* protocol, uint64_t irvalue) {
  wsBuf[0]='\0';
  sprintf (wsBuf, "{\"ircode\": %llu, \"protocol\": \"%s\"}", irvalue, protocol);
  websocket.textAll(wsBuf);
}
void NetServer::irValsToWs() {
  if (!irRecordEnable) return;
  wsBuf[0]='\0';
  sprintf (wsBuf, "{\"irvals\": [%llu, %llu, %llu]}", config.ircodes.irVals[config.irindex][0], config.ircodes.irVals[config.irindex][1], config.ircodes.irVals[config.irindex][2]);
  websocket.textAll(wsBuf);
}
#endif

void NetServer::onWsMessage(void *arg, uint8_t *data, size_t len, uint8_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (config.parseWsCommand((const char*)data, _wscmd, _wsval, 65)) {
      if (strcmp(_wscmd, "ping") == 0) {
        websocket.text(clientId, "{\"pong\": 1}");
        return;
      }
      if (strcmp(_wscmd, "trebble") == 0) {
        int8_t valb = atoi(_wsval);
        config.setTone(config.store.bass, config.store.middle, valb);
        return;
      }
      if (strcmp(_wscmd, "middle") == 0) {
        int8_t valb = atoi(_wsval);
        config.setTone(config.store.bass, valb, config.store.trebble);
        return;
      }
      if (strcmp(_wscmd, "bass") == 0) {
        int8_t valb = atoi(_wsval);
        config.setTone(valb, config.store.middle, config.store.trebble);
        return;
      }
      if (strcmp(_wscmd, "submitplaylistdone") == 0) {
#ifdef MQTT_ROOT_TOPIC
        //mqttplaylistticker.attach(5, mqttplaylistSend);
        timekeeper.waitAndDo(5, mqttplaylistSend);
#endif
        if (player.isRunning()) player.sendCommand({PR_PLAY, -config.lastStation()});
        return;
      }
      
      if(cmd.exec(_wscmd, _wsval, clientId)){
        return;
      }
    }
  }
}

void NetServer::getPlaylist(uint8_t clientId) {
  sprintf(nsBuf, "{\"file\": \"http://%s%s\"}", config.ipToStr(WiFi.localIP()), PLAYLIST_PATH);
  if (clientId == 0) { websocket.textAll(nsBuf); } else { websocket.text(clientId, nsBuf); }
}

int NetServer::_readPlaylistLine(File &file, char * line, size_t size){
  int bytesRead = file.readBytesUntil('\n', line, size);
  if(bytesRead>0){
    line[bytesRead] = 0;
    if(line[bytesRead-1]=='\r') line[bytesRead-1]=0;
  }
  return bytesRead;
}

bool NetServer::importPlaylist() {
  if(config.getMode()==PM_SDCARD) return false;
  //player.sendCommand({PR_STOP, 0});
  File tempfile = SPIFFS.open(TMP_PATH, "r");
  if (!tempfile) {
    return false;
  }
  char sName[BUFLEN], sUrl[BUFLEN], linePl[BUFLEN*3];
  int sOvol;
  // Read first non-empty line
  String firstLine;
  size_t firstPos = tempfile.position();
  while (tempfile.available()) {
    _readPlaylistLine(tempfile, linePl, sizeof(linePl)-1);
    firstLine = String(linePl); firstLine.trim();
    if (firstLine.length() > 0) break;
    firstPos = tempfile.position();
  }
  tempfile.seek(firstPos); // rewind to first non-empty line
  // Detect minified JSON array (single line, starts with [)
  bool isJsonArray = firstLine.startsWith("[");
  bool foundAny = false;
  File playlistfile = SPIFFS.open(TMP2_PATH, "w");
  if (isJsonArray) {
    // Read the whole file into a String
    String jsonStr;
    tempfile.seek(0);
    while (tempfile.available()) {
      _readPlaylistLine(tempfile, linePl, sizeof(linePl)-1);
      jsonStr += String(linePl);
    }
    jsonStr.trim();
    // Remove leading/trailing brackets if present
    if (jsonStr.startsWith("[")) jsonStr = jsonStr.substring(1);
    if (jsonStr.endsWith("]")) jsonStr = jsonStr.substring(0, jsonStr.length()-1);
    // Robustly extract each {...} object using brace counting
    int len = jsonStr.length();
    int i = 0;
    while (i < len) {
      // Skip whitespace and commas
      while (i < len && (jsonStr[i] == ' ' || jsonStr[i] == '\n' || jsonStr[i] == '\r' || jsonStr[i] == ',')) i++;
      if (i >= len) break;
      if (jsonStr[i] != '{') { i++; continue; }
      int start = i;
      int brace = 1;
      i++;
      while (i < len && brace > 0) {
        if (jsonStr[i] == '{') brace++;
        else if (jsonStr[i] == '}') brace--;
        i++;
      }
      if (brace == 0) {
        String objStr = jsonStr.substring(start, i);
        objStr.trim();
        if (objStr.length() == 0) continue;
        strncpy(linePl, objStr.c_str(), sizeof(linePl)-1);
        if (config.parseJSON(linePl, sName, sUrl, sOvol)) {
          snprintf(linePl, sizeof(linePl)-1, "%s\t%s\t%d", sName, sUrl, sOvol);
          playlistfile.print(String(linePl) + "\r\n");
          foundAny = true;
        }
      }
    }
  } else {
    // Not a minified array: process line by line
    tempfile.seek(0);
    while (tempfile.available()) {
      _readPlaylistLine(tempfile, linePl, sizeof(linePl)-1);
      String trimmed = String(linePl); trimmed.trim();
      if (trimmed.length() == 0 || trimmed == "[" || trimmed == "]" || trimmed == ",") continue;
      // Only treat as JSON if line starts with '{' and ends with '}'
      if (trimmed.startsWith("{") && trimmed.endsWith("}")) {
        if (config.parseJSON(linePl, sName, sUrl, sOvol)) {
          snprintf(linePl, sizeof(linePl)-1, "%s\t%s\t%d", sName, sUrl, sOvol);
          playlistfile.print(String(linePl) + "\r\n");
          foundAny = true;
        }
      } else {
        // Only treat as CSV if not JSON
        if (config.parseCSVimport(linePl, sName, sUrl, sOvol)) {
          snprintf(linePl, sizeof(linePl)-1, "%s\t%s\t%d", sName, sUrl, sOvol);
          playlistfile.print(String(linePl) + "\r\n");
          foundAny = true;
        }
      }
    }
  }
  playlistfile.flush();
  playlistfile.close();
  tempfile.close();
  if (foundAny) {
    SPIFFS.remove(PLAYLIST_PATH);
    SPIFFS.rename(TMP2_PATH, PLAYLIST_PATH);
    requestOnChange(PLAYLISTSAVED, 0);
    return true;
  }
  SPIFFS.remove(TMP_PATH);
  SPIFFS.remove(TMP2_PATH);
  return false;
}

void NetServer::requestOnChange(requestType_e request, uint8_t clientId) {
  if(nsQueue==NULL) return;
  nsRequestParams_t nsrequest;
  nsrequest.type = request;
  nsrequest.clientId = clientId;
  xQueueSend(nsQueue, &nsrequest, NSQ_SEND_DELAY);
}

void NetServer::resetQueue(){
  if(nsQueue!=NULL) xQueueReset(nsQueue);
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  static int freeSpace = 0;
  if(request->url()=="/upload"){
    if (!index) {
      if(filename!="tempwifi.csv"){
        //player.sendCommand({PR_STOP, 0});
        if(SPIFFS.exists(PLAYLIST_PATH)) SPIFFS.remove(PLAYLIST_PATH);
        if(SPIFFS.exists(INDEX_PATH)) SPIFFS.remove(INDEX_PATH);
        if(SPIFFS.exists(PLAYLIST_SD_PATH)) SPIFFS.remove(PLAYLIST_SD_PATH);
        if(SPIFFS.exists(INDEX_SD_PATH)) SPIFFS.remove(INDEX_SD_PATH);
      }
      freeSpace = (float)SPIFFS.totalBytes()/100*68-SPIFFS.usedBytes();
      request->_tempFile = SPIFFS.open(TMP_PATH , "w");
    }else{
      
    }
    if (len) {
      if(freeSpace>index+len){
        request->_tempFile.write(data, len);
      }
    }
    if (final) {
      request->_tempFile.close();
      freeSpace = 0;
    }
  }else if(request->url()=="/update"){
    if (!index) {
      int target = (request->getParam("updatetarget", true)->value() == "spiffs") ? U_SPIFFS : U_FLASH;
      Serial.printf("Update Start: %s\n", filename.c_str());
      player.sendCommand({PR_STOP, 0});
      display.putRequest(NEWMODE, UPDATING);
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, target)) {
        Update.printError(Serial);
        request->send(200, "text/html", updateError());
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
        request->send(200, "text/html", updateError());
      }
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %uB\n", index + len);
      } else {
        Update.printError(Serial);
        request->send(200, "text/html", updateError());
      }
    }
  }else{ // "/webboard"
    DBGVB("File: %s, size:%u bytes, index: %u, final: %s\n", filename.c_str(), len, index, final?"true":"false");
    if (!index) {
      player.sendCommand({PR_STOP, 0});
      String spath = "/www/";
      if(filename=="playlist.csv" || filename=="wifi.csv") spath = "/data/";
      request->_tempFile = SPIFFS.open(spath + filename , "w");
    }
    if (len) {
      request->_tempFile.write(data, len);
    }
    if (final) {
      request->_tempFile.close();
      if(filename=="playlist.csv") config.indexPlaylist();
    }
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT: /*netserver.requestOnChange(STARTUP, client->id()); */if (config.store.audioinfo) Serial.printf("[WEBSOCKET] client #%lu connected from %s\n", client->id(), config.ipToStr(client->remoteIP())); break;
    case WS_EVT_DISCONNECT: if (config.store.audioinfo) Serial.printf("[WEBSOCKET] client #%lu disconnected\n", client->id()); break;
    case WS_EVT_DATA: netserver.onWsMessage(arg, data, len, client->id()); break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// Helper to select and randomize radio-browser servers
void selectRadioBrowserServer() {
  size_t arr_size = sizeof(g_ipv4_servers) / sizeof(g_ipv4_servers[0]);
  for (size_t i = 0; i < arr_size; ++i) g_ipv4_servers[i] = "";
  File serversFile = SPIFFS.open("/www/rb_srvrs.json", "r");
  if (!serversFile) {
    Serial.println("[Search] [Error] Failed to open /www/rb_srvrs.json - will try to get IP of all.api.radio-browser.info instead.");
    goto useIP;
  } else {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, serversFile);
    serversFile.close();
    if (error) {
      Serial.print(F("[Search] [Error] deserializeJson() failed: "));
      Serial.println(error.c_str());
      goto useIP; // get out of the else
    }
    JsonArray servers = doc.as<JsonArray>();
    if (servers.isNull() || servers.size() == 0) {
      Serial.println("[Search] [Error] JSON is not a valid or is an empty array.");
      goto useIP; //get out of the else
    }
    // Collect unique IPv4 server names
    size_t count = 0;
    for (JsonObject server_obj : servers) {
      const char* ip = server_obj["ip"];
      if (ip && strchr(ip, '.')) { // It's an IPv4
        bool duplicate = false;
        for (size_t j = 0; j < count; ++j) {
          if (g_ipv4_servers[j] == ip) {
            duplicate = true;
            break;
          }
        }
        if (!duplicate && count < arr_size) {
          g_ipv4_servers[count++] = ip;
        }
      }
    }
    // Shuffle
    for (size_t i = count - 1; i > 0; --i) {
      size_t j = random(i + 1);
      String temp = g_ipv4_servers[i];
      g_ipv4_servers[i] = g_ipv4_servers[j];
      g_ipv4_servers[j] = temp;
    }

    IPAddress serverIP;
    if (WiFi.hostByName("all.api.radio-browser.info", serverIP)) {
      Serial.printf("Resolved IP: %s\n", serverIP.toString().c_str());
      g_ipv4_servers[0] = serverIP.toString();
    }
  }
  return;
useIP:
  IPAddress serverIP;
  if (WiFi.hostByName("all.api.radio-browser.info", serverIP)) {
    Serial.printf("Resolved IP: %s\n", serverIP.toString().c_str());
    g_ipv4_servers[0] = serverIP.toString();
  }
}

void vTaskSearchRadioBrowser(void *pvParameters) {
  char* search_str = (char*)pvParameters;
  Serial.printf("[Search] Starting radio browser search. Search: %s\n", search_str);
  SPIFFS.remove("/www/searchresults.json");
  // Check SPIFFS free space
  size_t freeSpace = SPIFFS.totalBytes() - SPIFFS.usedBytes();
  if (freeSpace < (FS_REQUIRED_FREE_SPACE * 1024)) {
    Serial.printf("[Search] [Error] Not enough free SPIFFS space: %u bytes. Aborting.\n", freeSpace);
    netserver.requestOnChange(SEARCH_FAILED, 0);
    delete[] search_str;
    g_searchTaskHandle = NULL;
    vTaskDelete(NULL);
    return;
  }
  // Count non-empty servers from our global persistent list
  size_t arr_size = sizeof(g_ipv4_servers) / sizeof(g_ipv4_servers[0]);
  int server_count = 0;
  for (size_t i = 0; i < arr_size; ++i) {
    if (g_ipv4_servers[i].length() > 0) server_count++;
  }
  // If the list is empty, it's the first run or all servers failed previously. Let's (re)populate it.
  if (server_count == 0) {
    Serial.println("[Search] Server list is empty, repopulating from file.");
    selectRadioBrowserServer();
    // Recount after filling
    server_count = 0;
    for (size_t i = 0; i < arr_size; ++i) {
      if (g_ipv4_servers[i].length() > 0) server_count++;
    }
  }
  // If still no servers, then the API source is likely down or unreachable.
  if (server_count == 0) {
    Serial.println("[Search] [Error] No IPv4 servers available after attempting to select.");
    netserver.requestOnChange(SEARCH_FAILED, 0);
    delete[] search_str;
    g_searchTaskHandle = NULL;
    vTaskDelete(NULL);
    return;
  }
  ESPFileUpdater searchResultsFetch(SPIFFS);
  searchResultsFetch.setUserAgent(ESPFILEUPDATER_USERAGENT);
  const char* localPath = "/www/searchresults.json";
  bool success = false;
  bool server_retried = false;
  bool json_valid = false;
  for (size_t i = 0; i < arr_size; ++i) {
    if (g_ipv4_servers[i].length() == 0) continue;
    String server = g_ipv4_servers[i];
    // Compose the URL using the full search string
    String url = "http://" + server + "/json/stations/search?" + String(search_str);
    Serial.printf("[Search] Attempting to download from: %s\n", url.c_str());
    auto status = searchResultsFetch.checkAndUpdate(localPath, url, ESPFILEUPDATER_VERBOSE);
    if (status == ESPFileUpdater::UPDATED) {
      Serial.printf("[Search] Successfully downloaded from %s\n", server.c_str());
      // Check if the downloaded file ends with ']'
      File jsonFile = SPIFFS.open(localPath, "r");
      if (jsonFile) {
        int fileSize = jsonFile.size();
        char lastChar = 0;
        if (fileSize > 0) {
          for (int pos = fileSize - 1; pos >= 0; --pos) {
            jsonFile.seek(pos, SeekSet);
            char c = jsonFile.read();
            if (!isspace((unsigned char)c)) {
              lastChar = c;
              break;
            }
          }
        }
        jsonFile.close();
        if (lastChar != ']') {
          if (server_retried == true) {
            Serial.printf("[Search] [Warning] searchresults.json is incomplete. Not retrying.\n");
            server_retried = false;
          } else {
            Serial.printf("[Search] [Warning] searchresults.json is incomplete. Retrying same server.\n");
            server_retried = true;
            --i;
          }
          continue;
        } else {
          json_valid = true;
        }
      } else {
        if (server_retried == true) {
          Serial.println("[Search] [Error] Could not open searchresults.json for validation. Not retrying.\n");
          server_retried = false;
        } else {
          Serial.println("[Search] [Error] Could not open searchresults.json for validation. Retrying same server.\n");
          server_retried = true;
          --i;
        }
        continue;
      }
      if (json_valid) {
        // Write /www/search.txt with the actual search string (single line)
        File file = SPIFFS.open("/www/search.txt", "w");
        if (file) {
          file.printf("%s\n", search_str);
          file.close();
        } else {
          Serial.println("[Search] [Error] Failed to open search.txt for writing.");
        }
        success = true;
        break;
      } else {
        Serial.printf("[Search] [Error] Invalid JSON from %s. Removing from list.\n", server.c_str());
        g_ipv4_servers[i] = "";
        server_retried = false;
      }
    } else {
      Serial.printf("[Search] [Error] Failed to download from %s. Removing from persistent list.\n", server.c_str());
      g_ipv4_servers[i] = "";
      server_retried = false;
    }
  }
  if (success) {
    netserver.requestOnChange(SEARCH_DONE, 0);
  } else {
    Serial.println("[Search] [Error] Failed to download from all available servers.");
    netserver.requestOnChange(SEARCH_FAILED, 0);
  }
  delete[] search_str;
  search_str = nullptr;
  g_searchTaskHandle = NULL;
  vTaskDelete(NULL);
}

void launchPlaybackTask(const String& url, const String& name) {
  if (name.length() > 0 && name.length() < sizeof(config.station.name)) {
    strlcpy(config.station.name, name.c_str(), sizeof(config.station.name));
  } else {
    strlcpy(config.station.name, "Playing", sizeof(config.station.name));
  }
  player.sendCommand({PR_STOP, 0}); // Stop any current playback first
  display.putRequest(NEWSTATION, 0);
  Serial.println("[netserver] Creating a dedicated task for playback.");
  // Use a lambda to capture the URL and pass it to the task
  String* url_copy = new String(url);
  if (url_copy) {
    // Use a larger stack for HTTPS, as it requires more memory for SSL/TLS.
    UBaseType_t stackSize = url.startsWith("https://") ? 8192 : 4096;
    xTaskCreate(
        [](void* pvParameters) {
          String* urlToPlay = (String*)pvParameters;
          vTaskDelay(pdMS_TO_TICKS(100)); // A small delay can help the network stack release resources
          Serial.printf("[PlaybackTask] Starting playback for URL: %s. Free heap: %u\n", urlToPlay->c_str(), ESP.getFreeHeap());
          player.playUrl(urlToPlay->c_str());
          delete urlToPlay; // Free the string
          vTaskDelete(NULL);
        },
        "playbackTask",
        stackSize,
        (void*)url_copy,
        1,
        NULL
    );
  } else {
    Serial.println("[netserver] ERROR: Failed to allocate memory for playback task URL.");
  }
}

#ifdef UPDATEURL
  void checkForOnlineUpdate() {
    const char* versionUrl = CHECKUPDATEURL;
    WiFiClientSecure client;
    client.setInsecure(); // skip server cert validation
    HTTPClient http;
    http.begin(client, versionUrl);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("User-Agent", ESPFILEUPDATER_USERAGENT);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      WiFiClient* stream = http.getStreamPtr();
      String line;
      String remoteVer;
      while (stream->connected() || stream->available()) {
        if (stream->available()) {
          char c = stream->read();
          if (c == '\n') {
            if (line.startsWith(VERSIONSTRING)) {
              int q1 = line.indexOf('"');
              int q2 = line.indexOf('"', q1 + 1);
              if (q1 > 0 && q2 > q1) {
                remoteVer = line.substring(q1 + 1, q2);
                break;
              }
            }
            line.clear();
          } else {
            line += c;
          }
        }
      }
      http.end();
      if (remoteVer.length() == 0) {
        websocket.textAll("{\"onlineupdateerror\": \"Remote YOVERSION not found\"}");
        return;
      }
      char msgBuf[128];
      if (remoteVer != String(YOVERSION)) {
        snprintf(msgBuf, sizeof(msgBuf), "{\"onlineupdateavailable\":true,\"remoteVersion\":\"%s\"}", remoteVer.c_str());
      } else {
        snprintf(msgBuf, sizeof(msgBuf), "{\"onlineupdateavailable\":false,\"remoteVersion\":\"%s\"}", remoteVer.c_str());
      }
      websocket.textAll(msgBuf);
    } else {
      websocket.textAll(String("{\"onlineupdateerror\": \"HTTP code ") + httpCode + "\"}");
      http.end();
    }
  }

  void startOnlineUpdate() {
    String updateUrl = String(UPDATEURL) + String(FIRMWARE);
    Serial.printf("[Online Update] Online Update download URL: %s\n", updateUrl.c_str());
    WiFiClientSecure client;
    client.setInsecure(); // skip server cert validation
    HTTPClient http;
    http.begin(client, updateUrl);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("User-Agent", ESPFILEUPDATER_USERAGENT);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      int contentLength = http.getSize();
      Serial.printf("[Online Update] Content-Length: %d\n", contentLength);
      if (contentLength > 0) {
        bool canBegin = Update.begin(contentLength);
        if (canBegin) {
          player.sendCommand({PR_STOP, 0});
          display.putRequest(NEWMODE, UPDATING);
          WiFiClient* stream = http.getStreamPtr();
          size_t written = 0;
          const size_t bufSize = 512;
          uint8_t buf[bufSize];
          unsigned long lastProgressTime = millis();
          while (written < contentLength) {
            int len = stream->read(buf, bufSize);
            if (len <= 0) {
              if (!stream->connected()) break;
              vTaskDelay(pdMS_TO_TICKS(10));
              continue;
            }
            size_t w = Update.write(buf, len);
            written += w;
            int percent = (written * 100) / contentLength;
            unsigned long now = millis();
            if (percent == 100 || now - lastProgressTime >= 1000) {
              lastProgressTime = now;
              char progMsg[64];
              snprintf(progMsg, sizeof(progMsg), "{\"onlineupdateprogress\":%d}", percent);
              websocket.textAll(progMsg);
            }
          }
          bool ended = Update.end();
          Serial.printf("[Online Update] Written %u bytes, expected %d, end() returned %s\n", written, contentLength, ended?"true":"false");
          if (written == contentLength && ended) {
            File markerFile = SPIFFS.open(ONLINEUPDATE_MARKERFILE, "w");
            if (markerFile) markerFile.close();
            websocket.textAll("{\"onlineupdatestatus\": \"Update successful, rebooting...\"}");
            delay(1000);
            ESP.restart();
          } else {
            websocket.textAll("{\"onlineupdateerror\": \"Update failed or incomplete\"}");
          }
        } else {
         websocket.textAll("{\"onlineupdateerror\": \"Cannot begin update (reboot then try again)\"}");
        }
      } else {
        websocket.textAll("{\"onlineupdateerror\": \"Invalid firmware size\"}");
      }
    } else {
      websocket.textAll("{\"onlineupdateerror\": \"Failed to download firmware\"}");
    }
    http.end();
  }
#endif //#ifdef UPDATEURL

void handleNotFound(AsyncWebServerRequest * request) {
#if defined(HTTP_USER) && defined(HTTP_PASS)
  if(network.status == CONNECTED)
    if (request->url() == "/logout") {
      request->send(401);
      return;
    }
    if (!request->authenticate(HTTP_USER, HTTP_PASS)) {
      return request->requestAuthentication();
    }
#endif
  if(request->url()=="/emergency") { request->send_P(200, "text/html", emergency_form); return; }
  if(request->method() == HTTP_POST && request->url()=="/webboard" && config.emptyFS) { request->redirect("/"); ESP.restart(); return; }
  if(request->method() == HTTP_GET && request->url() == "/search") { handleSearch(request); return; }
  if (request->method() == HTTP_POST && request->url() == "/search") { handleSearchPost(request); return; }

  #ifdef UPDATEURL
    if (request->method() == HTTP_GET && request->url() == "/onlineupdatecheck") {
      xTaskCreate([](void*) { checkForOnlineUpdate(); vTaskDelete(NULL); }, "checkForOnlineUpdateTask", 8096, nullptr, 1, nullptr);
      request->send(200, "text/plain", "Update check started"); return;
    }
    if (request->method() == HTTP_GET && request->url() == "/onlineupdatestart") {
      xTaskCreate([](void*) { startOnlineUpdate(); vTaskDelete(NULL); }, "startOnlineUpdateTask", 16384, nullptr, 3, nullptr);
      request->send(200, "text/plain", "Update started"); return;
    }
  #endif

  if (request->method() == HTTP_GET) {
    DBGVB("[%s] client ip=%s request of %s", __func__, config.ipToStr(request->client()->remoteIP()), request->url().c_str());
    if (strcmp(request->url().c_str(), PLAYLIST_PATH) == 0 || 
        strcmp(request->url().c_str(), SSIDS_PATH) == 0 || 
        strcmp(request->url().c_str(), INDEX_PATH) == 0 || 
        strcmp(request->url().c_str(), TMP_PATH) == 0 || 
        strcmp(request->url().c_str(), PLAYLIST_SD_PATH) == 0 || 
        strcmp(request->url().c_str(), INDEX_SD_PATH) == 0) {
#ifdef MQTT_ROOT_TOPIC
      if (strcmp(request->url().c_str(), PLAYLIST_PATH) == 0) while (mqttplaylistblock) vTaskDelay(5);
#endif
      if(strcmp(request->url().c_str(), PLAYLIST_PATH) == 0 && config.getMode()==PM_SDCARD){
        netserver.chunkedHtmlPage("application/octet-stream", request, PLAYLIST_SD_PATH);
      }else{
        netserver.chunkedHtmlPage("application/octet-stream", request, request->url().c_str());
      }
      return;
    }// if (strcmp(request->url().c_str(), PLAYLIST_PATH) == 0 || 
  }// if (request->method() == HTTP_GET)
  
  if (request->method() == HTTP_POST) {
    if(request->url()=="/webboard"){ request->redirect("/"); return; } // <--post files from /data/www
    if(request->url()=="/upload"){ // <--upload playlist.csv or wifi.csv
      if (request->hasParam("plfile", true, true)) {
        netserver.importRequest = IMPL;
        request->send(200);
      } else if (request->hasParam("wifile", true, true)) {
        netserver.importRequest = IMWIFI;
        request->send(200);
      } else {
        request->send(404);
      }
      return;
    }
    if(request->url()=="/update"){ // <--upload firmware
      shouldReboot = !Update.hasError();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : updateError());
      response->addHeader("Connection", "close");
      request->send(response);
      return;
    }
  }// if (request->method() == HTTP_POST)
  
  if (request->url() == "/favicon.ico") {
    request->send(200, "image/x-icon", "data:,");
    return;
  }
  if (request->url() == "/variables.js") {
    const char* onlineCapable =
    #ifdef UPDATEURL
      "true";
    #else
      "false";
    #endif
    snprintf(netserver.nsBuf, sizeof(netserver.nsBuf),
      "var yoVersion='%s';\n"
      "var formAction='%s';\n"
      "var playMode='%s';\n"
      "var onlineupdatecapable=%s;\n",
      YOVERSION,
      (network.status == CONNECTED && !config.emptyFS) ? "webboard" : "",
      (network.status == CONNECTED) ? "player" : "ap",
      onlineCapable
    );
    request->send(200, "application/javascript", netserver.nsBuf);
    return;
  }
  if (strcmp(request->url().c_str(), "/settings.html") == 0 || strcmp(request->url().c_str(), "/update.html") == 0 || strcmp(request->url().c_str(), "/ir.html") == 0){
    //request->send_P(200, "text/html", index_html);
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html);
    response->addHeader("Cache-Control","max-age=3600");
    request->send(response);
    return;
  }
  if (request->method() == HTTP_GET && request->url() == "/webboard") {
    request->send_P(200, "text/html", emptyfs_html);
    return;
  }
  Serial.print("Not Found: ");
  Serial.println(request->url());
  request->send(404, "text/plain", "Not found");
}

void handleIndex(AsyncWebServerRequest * request) {
  if(config.emptyFS){
    if(request->url()=="/" && request->method() == HTTP_GET ) { request->send_P(200, "text/html", emptyfs_html); return; }
    if(request->url()=="/" && request->method() == HTTP_POST) {
      if(request->arg("ssid")!="" && request->arg("pass")!=""){
        netserver.nsBuf[0]='\0';
        snprintf(netserver.nsBuf, sizeof(netserver.nsBuf), "%s\t%s", request->arg("ssid").c_str(), request->arg("pass").c_str());
        request->redirect("/");
        config.saveWifiFromNextion(netserver.nsBuf);
        return;
      }
      request->redirect("/"); 
      ESP.restart();
      return;
    }
    Serial.print("Not Found: ");
    Serial.println(request->url());
    request->send(404, "text/plain", "Not found");
    return;
  } // end if(config.emptyFS)
#if defined(HTTP_USER) && defined(HTTP_PASS)
  if(network.status == CONNECTED)
    if (!request->authenticate(HTTP_USER, HTTP_PASS)) {
      return request->requestAuthentication();
    }
#endif
  if (strcmp(request->url().c_str(), "/") == 0 && request->params() == 0) {
    if(network.status == CONNECTED) {
      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html);
      response->addHeader("Cache-Control","max-age=3600");
      request->send(response);
      //request->send_P(200, "text/html", index_html); 
    } else request->redirect("/settings.html");
    return;
  }
  if(network.status == CONNECTED){
    int paramsNr = request->params();
    if(paramsNr==1){
      AsyncWebParameter* p = request->getParam(0);
      if(cmd.exec(p->name().c_str(),p->value().c_str())) {
        if(p->name()=="reset" || p->name()=="clearspiffs") request->redirect("/");
        if(p->name()=="clearspiffs") { delay(100); ESP.restart(); }
        request->send(200, "text/plain", "");
        return;
      }
    }
    if (request->hasArg("trebble") && request->hasArg("middle") && request->hasArg("bass")) {
      config.setTone(request->getParam("bass")->value().toInt(), request->getParam("middle")->value().toInt(), request->getParam("trebble")->value().toInt());
      request->send(200, "text/plain", "");
      return;
    }
    if (request->hasArg("sleep")) {
      int sford = request->getParam("sleep")->value().toInt();
      int safterd = request->hasArg("after")?request->getParam("after")->value().toInt():0;
      if(sford > 0 && safterd >= 0){ request->send(200, "text/plain", ""); config.sleepForAfter(sford, safterd); return; }
    }
    request->send(404, "text/plain", "Not found");
    
  }else{
    request->send(404, "text/plain", "Not found");
  }
}
