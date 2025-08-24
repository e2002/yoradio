#include "options.h"
#include "Arduino.h"
#include <SPIFFS.h>
#include <Update.h>
#include "config.h"
#include "netserver.h"
#include "player.h"
#include "telnet.h"
#include "display.h"
#include "network.h"
#include "mqtt.h"
#include "controls.h"
#include "commandhandler.h"
#include "timekeeper.h"
#include "../displays/widgets/widgetsconfig.h" //BitrateFormat

#if DSP_MODEL==DSP_DUMMY
#define DUMMYDISPLAY
#endif

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

#ifdef DEBUG_V
#define DBGVB( ... ) { char buf[200]; sprintf( buf, __VA_ARGS__ ) ; Serial.print("[DEBUG]\t"); Serial.println(buf); }
#else
#define DBGVB( ... )
#endif

//#define CORS_DEBUG //Enable CORS policy: 'Access-Control-Allow-Origin' (for testing)

NetServer netserver;

AsyncWebServer webserver(80);
AsyncWebSocket websocket("/ws");

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void handleIndex(AsyncWebServerRequest * request);
void handleNotFound(AsyncWebServerRequest * request);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

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

bool NetServer::begin(bool quiet) {
  if(network.status==SDREADY) return true;
  if(!quiet) Serial.print("##[BOOT]#\tnetserver.begin\t");
  importRequest = IMDONE;
  irRecordEnable = false;
  playerBufMax = psramInit()?300000:1600 * config.store.abuff;
  nsQueue = xQueueCreate( 20, sizeof( nsRequestParams_t ) );
  while(nsQueue==NULL){;}

  webserver.on("/", HTTP_ANY, handleIndex);
  webserver.onNotFound(handleNotFound);
  webserver.onFileUpload(handleUpload);

  webserver.serveStatic("/", SPIFFS, "/www/").setCacheControl("max-age=31536000");
#ifdef CORS_DEBUG
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("content-type"));
#endif
  webserver.begin();
  //if(strlen(config.store.mdnsname)>0)
  //  MDNS.begin(config.store.mdnsname);
  websocket.onEvent(onWsEvent);
  webserver.addHandler(&websocket);
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
  response->addHeader("Cache-Control","max-age=31536000");
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

const char *getFormat(BitrateFormat _format) {
  switch (_format) {
    case BF_MP3:  return "MP3";
    case BF_AAC:  return "AAC";
    case BF_FLAC: return "FLC";
    case BF_OGG:  return "OGG";
    case BF_WAV:  return "WAV";
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
      case GETSCREEN:     sprintf (wsBuf, "{\"flip\":%d,\"inv\":%d,\"nump\":%d,\"tsf\":%d,\"tsd\":%d,\"dspon\":%d,\"br\":%d,\"con\":%d,\"scre\":%d,\"scrt\":%d,\"scrb\":%d,\"scrpe\":%d,\"scrpt\":%d,\"scrpb\":%d}", 
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
                                  config.store.screensaverPlayingBlank);
                                  break;
      case GETTIMEZONE:   sprintf (wsBuf, "{\"tzh\":%d,\"tzm\":%d,\"sntp1\":\"%s\",\"sntp2\":\"%s\", \"timeint\":%d,\"timeintrtc\":%d}", 
                                  config.store.tzHour, 
                                  config.store.tzMin, 
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
  char linePl[BUFLEN*3];
  int sOvol;
  _readPlaylistLine(tempfile, linePl, sizeof(linePl)-1);
  if (config.parseCSV(linePl, nsBuf, nsBuf2, sOvol)) {
    tempfile.close();
    SPIFFS.rename(TMP_PATH, PLAYLIST_PATH);
    requestOnChange(PLAYLISTSAVED, 0);
    return true;
  }
  if (config.parseJSON(linePl, nsBuf, nsBuf2, sOvol)) {
    File playlistfile = SPIFFS.open(PLAYLIST_PATH, "w");
    snprintf(linePl, sizeof(linePl)-1, "%s\t%s\t%d", nsBuf, nsBuf2, 0);
    playlistfile.println(linePl);
    while (tempfile.available()) {
      _readPlaylistLine(tempfile, linePl, sizeof(linePl)-1);
      if (config.parseJSON(linePl, nsBuf, nsBuf2, sOvol)) {
        snprintf(linePl, sizeof(linePl)-1, "%s\t%s\t%d", nsBuf, nsBuf2, 0);
        playlistfile.println(linePl);
      }
    }
    playlistfile.flush();
    playlistfile.close();
    tempfile.close();
    SPIFFS.remove(TMP_PATH);
    requestOnChange(PLAYLISTSAVED, 0);
    return true;
  }
  tempfile.close();
  SPIFFS.remove(TMP_PATH);
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
    sprintf (netserver.nsBuf, "var yoVersion='%s';\nvar formAction='%s';\nvar playMode='%s';\n", YOVERSION, (network.status == CONNECTED && !config.emptyFS)?"webboard":"", (network.status == CONNECTED)?"player":"ap");
    request->send(200, "text/html", netserver.nsBuf);
    return;
  }
  if (strcmp(request->url().c_str(), "/settings.html") == 0 || strcmp(request->url().c_str(), "/update.html") == 0 || strcmp(request->url().c_str(), "/ir.html") == 0){
    //request->send_P(200, "text/html", index_html);
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html);
    response->addHeader("Cache-Control","max-age=31536000");
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
      response->addHeader("Cache-Control","max-age=31536000");
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
