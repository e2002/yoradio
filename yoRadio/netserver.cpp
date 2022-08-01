#include "netserver.h"
#include <SPIFFS.h>

#include "config.h"
#include "player.h"
#include "telnet.h"
#include "display.h"
#include "options.h"
#include "network.h"
#include "mqtt.h"
#include <Update.h>

#ifndef MIN_MALLOC
#define MIN_MALLOC 24112
#endif

NetServer netserver;

AsyncWebServer webserver(80);
AsyncWebSocket websocket("/ws");
AsyncUDP udp;

String processor(const String& var);
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleHTTPPost(AsyncWebServerRequest * request);

byte  ssidCount;
bool  shouldReboot  = false;

char* updateError(){
  static char ret[140] = {0};
  sprintf(ret, "Update failed with error (%d)<br /> %s", (int)Update.getError(), Update.errorString());
  return ret;
}

bool NetServer::begin() {
  importRequest = false;
  irRecordEnable = false;
  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    ssidCount = 0;
    int mcb = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    int mci = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    (void)mci;
    log_i("[yoradio] webserver.on / - MALLOC_CAP_INTERNAL=%d, MALLOC_CAP_8BIT=%d", mci, mcb);
    netserver.resumePlay = mcb < MIN_MALLOC;
    if (netserver.resumePlay) {
      player.toggle();
      while (player.isRunning()) {
        delay(10);
      }
    }
    request->send(SPIFFS, "/www/index.html", String(), false, processor);
  });

  webserver.serveStatic("/", SPIFFS, "/www/").setCacheControl("max-age=31536000");

  webserver.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
    handleHTTPPost(request);
  });
  webserver.on(PLAYLIST_PATH, HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, PLAYLIST_PATH, "application/octet-stream");
  });
  webserver.on(INDEX_PATH, HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, INDEX_PATH, "application/octet-stream");
  });
  webserver.on(SSIDS_PATH, HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, SSIDS_PATH, "application/octet-stream");
  });
  webserver.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
    //request->send(200);
  }, handleUpload);
  webserver.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/update.html", String(), false, processor);
  });
#if IR_PIN!=255
  webserver.on("/ir", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/ir.html", String(), false, processor);
  });
#endif
  webserver.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK": updateError());
    response->addHeader("Connection", "close");
    request->send(response);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      int target = (request->getParam("updatetarget", true)->value() == "spiffs") ? U_SPIFFS : U_FLASH;
      Serial.printf("Update Start: %s\n", filename.c_str());
      player.mode = STOPPED;
      display.putRequest({NEWMODE, UPDATING});
      if(!Update.begin(UPDATE_SIZE_UNKNOWN, target)){
        Update.printError(Serial);
        request->send(200, "text/html", updateError());
      }
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
        request->send(200, "text/html", updateError());
      }
    }
    if(final){
      if(Update.end(true)){
        Serial.printf("Update Success: %uB\n", index+len);
      } else {
        Update.printError(Serial);
        request->send(200, "text/html", updateError());
      }
    }
  });
  webserver.begin();
  websocket.onEvent(onWsEvent);
  webserver.addHandler(&websocket);

  //echo -n "helle?" | socat - udp-datagram:255.255.255.255:44490,broadcast
  if (udp.listen(44490)) {
    udp.onPacket([](AsyncUDPPacket packet) {
      if (strcmp((char*)packet.data(), "helle?") == 0)
        packet.println(WiFi.localIP());
    });
  }
  return true;
}

void NetServer::loop() {
  if(shouldReboot){
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
  websocket.cleanupClients();
  if (playlistrequest > 0) {
    requestOnChange(PLAYLIST, playlistrequest);
    playlistrequest = 0;
  }
  if (importRequest) {
    if (importPlaylist()) {
      requestOnChange(PLAYLIST, 0);
    }
    importRequest = false;
  }
  if (rssi < 255) {
    requestOnChange(NRSSI, 0);
  }
}
#if IR_PIN!=255
void NetServer::irToWs(const char* protocol, uint64_t irvalue) {
  char buf[BUFLEN] = { 0 };
  sprintf (buf, "{\"ircode\": %llu, \"protocol\": \"%s\"}", irvalue, protocol);
  websocket.textAll(buf);
}
void NetServer::irValsToWs(){
  if(!irRecordEnable) return;
  char buf[BUFLEN] = { 0 };
  sprintf (buf, "{\"irvals\": [%llu, %llu, %llu]}", config.ircodes.irVals[config.irindex][0], config.ircodes.irVals[config.irindex][1], config.ircodes.irVals[config.irindex][2]);
  websocket.textAll(buf);
}
#endif
void NetServer::onWsMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    char cmd[15], val[15];
    if (config.parseWsCommand((const char*)data, cmd, val, 15)) {
      if (strcmp(cmd, "volume") == 0) {
        byte v = atoi(val);
        player.setVol(v, false);
      }
#if IR_PIN!=255
      if (strcmp(cmd, "irbtn") == 0) {
        config.irindex=atoi(val);
        irRecordEnable=(config.irindex>=0);
        config.irchck=0;
        irValsToWs();
        if(config.irindex<0) config.saveIR();
      }
      if (strcmp(cmd, "chkid") == 0) {
        config.irchck=atoi(val);
      }
      if (strcmp(cmd, "irclr") == 0) {
        byte cl = atoi(val);
        config.ircodes.irVals[config.irindex][cl]=0;
      }
#endif
    }
  }
}

void NetServer::setRSSI(int val) {
  rssi = val;
  //requestOnChange(NRSSI, 0);
}

void NetServer::getPlaylist(uint8_t clientId) {
  char buf[160] = {0};
  sprintf(buf, "{\"file\": \"http://%s%s\"}", WiFi.localIP().toString().c_str(), PLAYLIST_PATH);
  if (clientId == 0) {
    websocket.textAll(buf);
  } else {
    websocket.text(clientId, buf);
  }
  if (resumePlay) {
    resumePlay = false;
    player.toggle();
  }
}

bool NetServer::savePlaylist(const char* post) {
  File file = SPIFFS.open(PLAYLIST_PATH, "w");
  if (!file) {
    return false;
  } else {
    file.print(post);
    file.close();
    netserver.requestOnChange(PLAYLISTSAVED, 0);
    return true;
  }
}

bool NetServer::importPlaylist() {
  File tempfile = SPIFFS.open(TMP_PATH, "r");
  if (!tempfile) {
    return false;
  }
  char sName[BUFLEN], sUrl[BUFLEN];
  int sOvol;
  String line = tempfile.readStringUntil('\n');
  if (config.parseCSV(line.c_str(), sName, sUrl, sOvol)) {
    File playlistfile = SPIFFS.open(PLAYLIST_PATH, "w");
    playlistfile.println(line);
    while (tempfile.available()) {
      line = tempfile.readStringUntil('\n');
      if (config.parseCSV(line.c_str(), sName, sUrl, sOvol)) {
        playlistfile.println(line);
      }
    }
    playlistfile.close();
    tempfile.close();
    SPIFFS.remove(TMP_PATH);
    requestOnChange(PLAYLISTSAVED, 0);
    return true;
  }
  if (config.parseJSON(line.c_str(), sName, sUrl, sOvol)) {
    File playlistfile = SPIFFS.open(PLAYLIST_PATH, "w");
    String wline = String(sName) + "\t" + String(sUrl) + "\t" + String(sOvol);
    playlistfile.println(wline);
    while (tempfile.available()) {
      line = tempfile.readStringUntil('\n');
      if (config.parseJSON(line.c_str(), sName, sUrl, sOvol)) {
        wline = String(sName) + "\t" + String(sUrl) + "\t" + String(sOvol);
        playlistfile.println(wline);
      }
    }
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
  char buf[BUFLEN + 50] = { 0 };
  switch (request) {
    case PLAYLIST: {
        getPlaylist(clientId);
        break;
      }
    case PLAYLISTSAVED: {
        config.indexPlaylist();
        config.initPlaylist();
        getPlaylist(clientId);
#ifdef MQTT_HOST
        mqttPublishPlaylist();
#endif
        break;
      }
    case STATION: {
        sprintf (buf, "{\"nameset\": \"%s\"}", config.station.name);
        requestOnChange(ITEM, clientId);
        break;
      }
    case ITEM: {
        sprintf (buf, "{\"current\": %d}", config.store.lastStation);
        break;
      }
    case TITLE: {
        sprintf (buf, "{\"meta\": \"%s\"}", config.station.title);
        if (player.requestToStart) {
          telnet.info();
          player.requestToStart = false;
        } else {
          telnet.printf("##CLI.META#: %s\n> ", config.station.title);
        }
        break;
      }
    case VOLUME: {
        sprintf (buf, "{\"vol\": %d}", config.store.volume);
#ifdef MQTT_HOST
        if (clientId == 0) mqttPublishVolume();
#endif
        break;
      }
    case NRSSI: {
        sprintf (buf, "{\"rssi\": %d}", rssi);
        rssi = 255;
        break;
      }
    case BITRATE: {
        sprintf (buf, "{\"bitrate\": %d}", config.station.bitrate);
        break;
      }
    case MODE: {
        sprintf (buf, "{\"mode\": \"%s\"}", player.mode == PLAYING ? "playing" : "stopped");
        break;
      }
    case EQUALIZER: {
        sprintf (buf, "{\"bass\": %d, \"middle\": %d, \"trebble\": %d}", config.store.bass, config.store.middle, config.store.trebble);
        break;
      }
    case BALANCE: {
        sprintf (buf, "{\"balance\": %d}", config.store.balance);
        break;
      }
  }
  if (strlen(buf) > 0) {
    if (clientId == 0) {
      websocket.textAll(buf);
#ifdef MQTT_HOST
      if (request == STATION || request == ITEM || request == TITLE || request == MODE) mqttPublishStatus();
#endif
    } else {
      websocket.text(clientId, buf);
    }
  }
}

String processor(const String& var) { // %Templates%
  if (var == "VERSION") {
    return VERSION;
  }
  if (var == "SSID") {
    ssidCount++;
    return String(config.ssids[ssidCount - 1].ssid);
  }
  if (var == "PASS") {
    return String(config.ssids[ssidCount - 1].password);
  }
  if (var == "APMODE") {
    return network.status == CONNECTED ? "" : " style=\"display: none!important\"";
  }
  if (var == "NOTAPMODE") {
    return network.status == CONNECTED ? " hidden" : "";
  }
  if (var == "IRMODE") {
    return IR_PIN == 255 ? "" : " ir";
  }
  return String();
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    request->_tempFile = SPIFFS.open(TMP_PATH , "w");
  }
  if (len) {
    request->_tempFile.write(data, len);
    //TODO check index+len size
  }
  if (final) {
    request->_tempFile.close();
    netserver.importRequest = true;
    request->send(200);
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      if(config.store.audioinfo) Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());

      netserver.requestOnChange(STATION, client->id());
      netserver.requestOnChange(TITLE, client->id());
      netserver.requestOnChange(VOLUME, client->id());
      netserver.requestOnChange(EQUALIZER, client->id());
      netserver.requestOnChange(BALANCE, client->id());
      netserver.requestOnChange(BITRATE, client->id());
      netserver.requestOnChange(MODE, client->id());
      netserver.playlistrequest = client->id();

      break;
    case WS_EVT_DISCONNECT:
      if(config.store.audioinfo) Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      netserver.onWsMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleHTTPPost(AsyncWebServerRequest * request) {
  if (request->hasParam("wifisettings", true)) {
    AsyncWebParameter* p = request->getParam("wifisettings", true);
    if (p->value() != "") {
      config.saveWifi(p->value().c_str());
    }
    request->send(200);
    return;
  }
  if (request->hasParam("playlist", true)) {
    AsyncWebParameter* p = request->getParam("playlist", true);
    netserver.savePlaylist(p->value().c_str());
    request->send(200);
    return;
  }
  if (network.status != CONNECTED) {
    request->send(404);
    return;
  }
  if (request->hasParam("start", true)) {
    player.request.station = config.store.lastStation;
    request->send(200);
    return;
  }
  if (request->hasParam("stop", true)) {
    player.mode = STOPPED;
    //display.title("[stopped]");
    config.setTitle("[stopped]");
    request->send(200);
    return;
  }
  if (request->hasParam("prev", true)) {
    player.prev();
    request->send(200);
    return;
  }
  if (request->hasParam("next", true)) {
    player.next();
    request->send(200);
    return;
  }
  if (request->hasParam("volm", true)) {
    player.stepVol(false);
    request->send(200);
    return;
  }
  if (request->hasParam("volp", true)) {
    player.stepVol(true);
    request->send(200);
    return;
  }
  if (request->hasParam("vol", true)) {
    AsyncWebParameter* p = request->getParam("vol", true);
    int v = atoi(p->value().c_str());
    if (v < 0) v = 0;
    if (v > 254) v = 254;
    player.setVol(v, false);
    request->send(200);
    return;
  }
  if (request->hasParam("trebble", true)) {
    AsyncWebParameter* pt = request->getParam("trebble", true);
    AsyncWebParameter* pm = request->getParam("middle", true);
    AsyncWebParameter* pb = request->getParam("bass", true);
    int t = atoi(pt->value().c_str());
    int m = atoi(pm->value().c_str());
    int b = atoi(pb->value().c_str());
    //setTone(int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass)
    player.setTone(b, m, t);
    config.setTone(b, m, t);
    netserver.requestOnChange(EQUALIZER, 0);
    request->send(200);
    return;
  }
  if (request->hasParam("ballance", true)) {
    AsyncWebParameter* p = request->getParam("ballance", true);
    int b = atoi(p->value().c_str());
    player.setBalance(b);
    config.setBalance(b);
    netserver.requestOnChange(BALANCE, 0);
    request->send(200);
    return;
  }
  if (request->hasParam("playstation", true)) {
    AsyncWebParameter* p = request->getParam("playstation", true);
    int id = atoi(p->value().c_str());
    if (id < 1) id = 1;
    if (id > config.store.countStation) id = config.store.countStation;
    player.request.station = id;
    player.request.doSave = true;
    request->send(200);
    return;
  }

  request->send(404);
}
