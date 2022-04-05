#include "netserver.h"
#include <SPIFFS.h>

#include "config.h"
#include "player.h"
#include "telnet.h"
#include "display.h"
#include "options.h"
#include "network.h"
#include "mqtt.h"

NetServer netserver;

AsyncWebServer webserver(80);
AsyncWebSocket websocket("/ws");
AsyncUDP udp;

String processor(const String& var);
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleHTTPPost(AsyncWebServerRequest * request);

byte ssidCount;

bool NetServer::begin() {
  importRequest = false;

  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    ssidCount = 0;
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
  if(rssi<255){
    requestOnChange(NRSSI, 0);
  }
  yield();
}

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
    }
  }
}

void NetServer::setRSSI(int val) {
  rssi = val;
  //requestOnChange(NRSSI, 0);
}

void NetServer::getPlaylist(uint8_t clientId) {
  String dataString = "";
  File file = SPIFFS.open(PLAYLIST_PATH, "r");
  if (!file || file.isDirectory()) {
    return;
  }
  char sName[BUFLEN], sUrl[BUFLEN], pOvol[30];
  int sOvol;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (config.parseCSV(line.c_str(), sName, sUrl, sOvol)) {
      sprintf(pOvol, "%d", sOvol);
      dataString += "{\"name\":\"" + String(sName) + "\",\"url\":\"" + String(sUrl) + "\",\"ovol\":" + String(pOvol) + "},";
    }
  }
  if (dataString.length() > 0) {
    if (clientId == 0) {
      websocket.textAll("{\"file\": [" + dataString.substring(0, dataString.length() - 1) + "]}");
    } else {
      websocket.text(clientId, "{\"file\": [" + dataString.substring(0, dataString.length() - 1) + "]}");
    }
  }
  file.close();
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
        rssi=255;
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
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
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
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
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
