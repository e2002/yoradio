#include "netserver.h"
#include <SPIFFS.h>

#include "config.h"
#include "player.h"
#include "telnet.h"
#include "display.h"
#include "options.h"
#include "network.h"
#include "mqtt.h"
#include "controls.h"
#include <Update.h>

#ifndef MIN_MALLOC
#define MIN_MALLOC 24112
#endif

//#define CORS_DEBUG

NetServer netserver;

AsyncWebServer webserver(80);
AsyncWebSocket websocket("/ws");
AsyncUDP udp;

String processor(const String& var);
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleHTTPPost(AsyncWebServerRequest * request);

bool  shouldReboot  = false;

char* updateError(){
  static char ret[140] = {0};
  sprintf(ret, "Update failed with error (%d)<br /> %s", (int)Update.getError(), Update.errorString());
  return ret;
}

void NetServer::takeMallocDog(){
  int mcb = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  int mci = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  (void)mci;
  DBGVB("MALLOC_CAP_8BIT=%d, MALLOC_CAP_INTERNAL=%d", mcb, mci);
  resumePlay = mcb < MIN_MALLOC;
  if (resumePlay) {
    player.toggle();
    vTaskDelay(150);
    xSemaphoreTake(player.playmutex, portMAX_DELAY);
  }
}

void NetServer::giveMallocDog(){
  if (resumePlay) {
    resumePlay = false;
    vTaskDelay(150);
    xSemaphoreGive(player.playmutex);
    player.toggle();
  }
}

bool NetServer::begin() {
  importRequest = false;
  irRecordEnable = false;
  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (network.status == CONNECTED) {
      netserver.htmlPath = PINDEX;
      netserver.chunkedHtmlPage(String(), request);
    }else{
      netserver.htmlPath = PSETTINGS;
      netserver.chunkedHtmlPage(String(), request);
    }
  });

  webserver.serveStatic("/", SPIFFS, "/www/").setCacheControl("max-age=31536000");

  webserver.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
    handleHTTPPost(request);
  });
  webserver.on(PLAYLIST_PATH, HTTP_GET, [](AsyncWebServerRequest * request) {
    netserver.takeMallocDog();
    request->send(SPIFFS, PLAYLIST_PATH, "application/octet-stream");
    netserver.giveMallocDog();
    DBGVB("PLAYLIST_PATH client ip=%s", request->client()->remoteIP().toString().c_str());
    /*netserver.htmlPath = PPLAYLIST; // TODO
    netserver.chunkedHtmlPage("application/octet-stream", request);
    netserver.giveMallocDog();*/
  });
  webserver.on(INDEX_PATH, HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, INDEX_PATH, "application/octet-stream");
  });
  webserver.on(SSIDS_PATH, HTTP_GET, [](AsyncWebServerRequest * request) {
    netserver.htmlPath = PSSIDS;
    netserver.chunkedHtmlPage("application/octet-stream", request);
  });
  webserver.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
    //request->send(200);
    
  }, handleUpload);
  webserver.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
      netserver.htmlPath = PUPDATE;
      netserver.chunkedHtmlPage(String(), request);
  });
  webserver.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    netserver.htmlPath = PSETTINGS;
    netserver.chunkedHtmlPage(String(), request);
  });
  
#if IR_PIN!=255
  webserver.on("/ir", HTTP_GET, [](AsyncWebServerRequest *request){
      netserver.htmlPath = PIR;
      netserver.chunkedHtmlPage(String(), request);
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
#ifdef CORS_DEBUG
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("content-type"));
#endif
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

void NetServer::chunkedHtmlPage(const String& contentType, AsyncWebServerRequest *request){
  max = heap_caps_get_free_size(MALLOC_CAP_8BIT) / 32;   
  htmlpos = 0;
  theend  = false;
  DBGVB("chunkedHtmlPage client ip=%s", request->client()->remoteIP().toString().c_str());
  AsyncWebServerResponse *response = request->beginChunkedResponse(contentType, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
    if(netserver.theend) return 0;
    File htmlpage;
    switch(netserver.htmlPath){
      case PINDEX: {
        htmlpage = SPIFFS.open("/www/index.html", "r");
        break;
      }
      case PSETTINGS: {
        htmlpage = SPIFFS.open("/www/settings.html", "r");
        break;
      }
      case PUPDATE: {
        htmlpage = SPIFFS.open("/www/update.html", "r");
        break;
      }
      case PIR: {
        htmlpage = SPIFFS.open("/www/ir.html", "r");
        break;
      }
      case PPLAYLIST: {
        htmlpage = SPIFFS.open(PLAYLIST_PATH, "r");
        DBGVB("SPIFFS.open(PLAYLIST_PATH)");
        break;
      }
      case PSSIDS: {
        htmlpage = SPIFFS.open(SSIDS_PATH, "r");
        break;
      }
      default: {
        return 0;
        break;
      }
    }
    if(!htmlpage) return 0;
    uint32_t htmlpagesize = htmlpage.size();
    uint32_t len =  htmlpagesize - netserver.htmlpos;
    if (len > maxLen) len = maxLen;
    if (len > netserver.max) len = netserver.max;
    if (len + netserver.htmlpos > htmlpagesize) {
      netserver.theend = true;
      len = htmlpagesize - netserver.htmlpos;
    }
    if (len > 0) {
      DBGVB("seek to %d in %s and read %d bytes", netserver.htmlpos, htmlpage.name(), len);
      htmlpage.seek(netserver.htmlpos, SeekSet);
      htmlpage.read(buffer, len);
      netserver.htmlpos = netserver.htmlpos + len;
    }
    if(htmlpage) htmlpage.close();
    return len;
  }, processor); // AsyncWebServerResponse
  request->send(response);
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
void NetServer::onWsMessage(void *arg, uint8_t *data, size_t len, uint8_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    char cmd[65], val[65];
    if (config.parseWsCommand((const char*)data, cmd, val, 65)) {
      if (strcmp(cmd, "getmode") == 0) {
        requestOnChange(GETMODE,clientId);
        return;
      }
      if (strcmp(cmd, "getindex") == 0) {
        requestOnChange(GETINDEX,clientId);
        return;
      }
      if (strcmp(cmd, "getsystem") == 0) {
        requestOnChange(GETSYSTEM,clientId);
        return;
      }
      if (strcmp(cmd, "getscreen") == 0) {
        requestOnChange(GETSCREEN,clientId);
        return;
      }
      if (strcmp(cmd, "gettimezone") == 0) {
        requestOnChange(GETTIMEZONE,clientId);
        return;
      }
      if (strcmp(cmd, "getcontrols") == 0) {
        requestOnChange(GETCONTROLS,clientId);
        return;
      }
      if (strcmp(cmd, "getweather") == 0) {
        requestOnChange(GETWEATHER,clientId);
        return;
      }
      if (strcmp(cmd, "getactive") == 0) {
        requestOnChange(GETACTIVE,clientId);
        return;
      }

      if (strcmp(cmd, "smartstart") == 0) {
        byte valb=atoi(val);
        config.store.smartstart=valb==1?1:2;
        if(!player.isRunning() && config.store.smartstart==1) config.store.smartstart=0;
        config.save();
        return;
      }
      if (strcmp(cmd, "audioinfo") == 0) {
        byte valb=atoi(val);
        config.store.audioinfo=valb;
        config.save();
        display.putRequest({NEWMODE, CLEAR});
        display.putRequest({NEWMODE, PLAYER});
        return;
      }
      if (strcmp(cmd, "vumeter") == 0) {
        byte valb=atoi(val);
        config.store.vumeter=valb;
        config.save();
        display.putRequest({NEWMODE, CLEAR});
        display.putRequest({NEWMODE, PLAYER});
        return;
      }
      if (strcmp(cmd, "softap") == 0) {
        byte valb=atoi(val);
        config.store.softapdelay=valb;
        config.save();
        return;
      }
      if (strcmp(cmd, "invertdisplay") == 0) {
        byte valb=atoi(val);
        config.store.invertdisplay=valb;
        config.save();
        display.invert();
        return;
      }
      if (strcmp(cmd, "numplaylist") == 0) {
        byte valb=atoi(val);
        config.store.numplaylist=valb;
        config.save();
        display.putRequest({NEWMODE, CLEAR});
        display.putRequest({NEWMODE, PLAYER});
        return;
      }
      
      if (strcmp(cmd, "fliptouch") == 0) {
        byte valb=atoi(val);
        config.store.fliptouch=valb==1;
        config.save();
        flipTS();
        return;
      }
      if (strcmp(cmd, "dbgtouch") == 0) {
        byte valb=atoi(val);
        config.store.dbgtouch=valb==1;
        config.save();
        return;
      }
      if (strcmp(cmd, "flipscreen") == 0) {
        byte valb=atoi(val);
        config.store.flipscreen=valb;
        config.save();
        display.flip();
        display.putRequest({NEWMODE, CLEAR});
        display.putRequest({NEWMODE, PLAYER});
        return;
      }
      if (strcmp(cmd, "brightness") == 0) {
        byte valb=atoi(val);
        if(!config.store.dspon) requestOnChange(DSPON, 0);
        config.store.brightness=valb;
        //display.setContrast();
        config.setBrightness(true);
        return;
      }
      if (strcmp(cmd, "screenon") == 0) {
        byte valb=atoi(val);
        //config.store.dspon=valb==1;
        //config.setBrightness(true);
        config.setDspOn(valb==1);
        return;
      }
      if (strcmp(cmd, "contrast") == 0) {
        byte valb=atoi(val);
        config.store.contrast=valb;
        config.save();
        display.setContrast();
        return;
      }
      if (strcmp(cmd, "tzh") == 0) {
        int vali=atoi(val);
        config.store.tzHour=vali;
        return;
      }
      if (strcmp(cmd, "tzm") == 0) {
        int vali=atoi(val);
        config.store.tzMin=vali;
        return;
      }
      if (strcmp(cmd, "sntp2") == 0) {
        strlcpy(config.store.sntp2,val, 35);
        return;
      }
      if (strcmp(cmd, "sntp1") == 0) {
        strlcpy(config.store.sntp1,val, 35);
        bool tzdone=false;
        if(strlen(config.store.sntp1)>0 && strlen(config.store.sntp2)>0){
          configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), config.store.sntp1, config.store.sntp2);
          tzdone=true;
        }else if(strlen(config.store.sntp1)>0){
          configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), config.store.sntp1);
          tzdone=true;
        }
        if(tzdone){
          network.requestTimeSync(true);
          config.save();
        }
        return;
      }

      if (strcmp(cmd, "volsteps") == 0) {
        uint8_t valb=atoi(val);
        config.store.volsteps=valb;
        config.save();
        return;
      }
      if (strcmp(cmd, "encacceleration") == 0) {
        uint16_t valb=atoi(val);
        setEncAcceleration(valb);
        config.store.encacc=valb;
        config.save();
        return;
      }
      if (strcmp(cmd, "irtlp") == 0) {
        uint8_t valb=atoi(val);
        setIRTolerance(valb);
        return;
      }
      if (strcmp(cmd, "showweather") == 0) {
        uint8_t valb=atoi(val);
        config.store.showweather=valb==1;
        config.save();
        display.showWeather();
#ifdef USE_NEXTION
        nextion.startWeather();
#endif
        display.putRequest({NEWMODE, CLEAR});
        display.putRequest({NEWMODE, PLAYER});
        return;
      }
      if (strcmp(cmd, "lat") == 0) {
        strlcpy(config.store.weatherlat,val, 10);
        return;
      }
      if (strcmp(cmd, "lon") == 0) {
        strlcpy(config.store.weatherlon,val, 10);
        return;
      }
      if (strcmp(cmd, "key") == 0) {
        strlcpy(config.store.weatherkey,val, 64);
        config.save();
        display.showWeather();
#ifdef USE_NEXTION
        nextion.startWeather();
#endif
        display.putRequest({NEWMODE, CLEAR});
        display.putRequest({NEWMODE, PLAYER});
        return;
      }
      /*  RESETS  */
      if (strcmp(cmd, "reset") == 0) {
        if (strcmp(val, "system") == 0) {
          config.store.smartstart=2;
          config.store.audioinfo=false;
          config.store.vumeter=false;
          config.store.softapdelay=0;
          config.save();
          display.putRequest({NEWMODE, CLEAR});
          display.putRequest({NEWMODE, PLAYER});
          requestOnChange(GETSYSTEM,clientId);
          return;
        }
        if (strcmp(val, "screen") == 0) {
          config.store.flipscreen=false;
          display.flip();
          config.store.invertdisplay=false;
          display.invert();
          config.store.dspon=true;
          config.store.brightness=100;
          config.setBrightness(false);
          config.store.contrast=55;
          display.setContrast();
          config.store.numplaylist=false;
          config.save();
          display.putRequest({NEWMODE, CLEAR});
          display.putRequest({NEWMODE, PLAYER});
          requestOnChange(GETSCREEN,clientId);
          return;
        }
        if (strcmp(val, "timezone") == 0) {
          config.store.tzHour=3;
          config.store.tzMin=0;
          strlcpy(config.store.sntp1,"pool.ntp.org", 35);
          strlcpy(config.store.sntp2,"0.ru.pool.ntp.org", 35);
          config.save();
          configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), config.store.sntp1, config.store.sntp2);
          network.requestTimeSync(true);
          requestOnChange(GETTIMEZONE,clientId);
          return;
        }
        if (strcmp(val, "weather") == 0) {
          config.store.showweather=0;
          strlcpy(config.store.weatherlat,"55.7512", 10);
          strlcpy(config.store.weatherlon,"37.6184", 10);
          strlcpy(config.store.weatherkey,"", 64);
          config.save();
          display.showWeather();
#ifdef USE_NEXTION
          nextion.startWeather();
#endif
          display.putRequest({NEWMODE, CLEAR});
          display.putRequest({NEWMODE, PLAYER});
          requestOnChange(GETWEATHER,clientId);
          return;
        }
        if (strcmp(val, "controls") == 0) {
          config.store.volsteps=1;
          config.store.fliptouch=false;
          config.store.dbgtouch=false;
          setEncAcceleration(200);
          setIRTolerance(40);
          requestOnChange(GETCONTROLS,clientId);
          return;
        }
      } /*  RESETS  */
      if (strcmp(cmd, "volume") == 0) {
        byte v = atoi(val);
        player.setVol(v, false);
      }

      /* REMOVE FROM POST
       *   if (request->hasParam("trebble", true)) {
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
       */
      if (strcmp(cmd, "balance") == 0) {
        int8_t valb=atoi(val);
        player.setBalance(valb);
        config.setBalance(valb);
        netserver.requestOnChange(BALANCE, 0);
        return;
      }
      if (strcmp(cmd, "treble") == 0) {
        int8_t valb=atoi(val);
        player.setTone(config.store.bass, config.store.middle, valb);
        config.setTone(config.store.bass, config.store.middle, valb);
        netserver.requestOnChange(EQUALIZER, 0);
        return;
      }
      if (strcmp(cmd, "middle") == 0) {
        int8_t valb=atoi(val);
        player.setTone(config.store.bass, valb, config.store.trebble);
        config.setTone(config.store.bass, valb, config.store.trebble);
        netserver.requestOnChange(EQUALIZER, 0);
        return;
      }
      if (strcmp(cmd, "bass") == 0) {
        int8_t valb=atoi(val);
        player.setTone(valb, config.store.middle, config.store.trebble);
        config.setTone(valb, config.store.middle, config.store.trebble);
        netserver.requestOnChange(EQUALIZER, 0);
        return;
      }
      if (strcmp(cmd, "submitplaylist") == 0) {
        if(player.isRunning()){
          player.toggle();
          while (player.isRunning()) {
            vTaskDelay(10);
          }
          vTaskDelay(50);
          resumePlay=true;
        }
        return;
      }
      if (strcmp(cmd, "submitplaylistdone") == 0) {
        if(resumePlay){
          vTaskDelay(100);
          player.toggle();
          resumePlay=false;
        }
#ifdef MQTT_HOST
        mqttPublishPlaylist();
#endif
        return;
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
    vTaskDelay(150);
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
  char buf[BUFLEN * 2] = { 0 };
  switch (request) {
    case PLAYLIST: {
        getPlaylist(clientId);
        break;
      }
    case PLAYLISTSAVED: {
        config.indexPlaylist();
        config.initPlaylist();
        getPlaylist(clientId);
/*#ifdef MQTT_HOST
        mqttPublishPlaylist();
#endif*/
        break;
      }
    case GETACTIVE: {
        bool dbgact = false;
        String act="\"group_wifi\",";
        if (network.status == CONNECTED) {
          act+="\"group_system\",";
          if(BRIGHTNESS_PIN!=255 || DSP_FLIPPED==1 || DSP_MODEL==DSP_NOKIA5110 || dbgact){
            act+="\"group_display\",";
          }
#ifdef USE_NEXTION
          act+="\"group_nextion\",";
          if (WEATHER_READY==0 || dbgact){
            act+="\"group_weather\",";
          }
#endif
#if defined(LCD_I2C) || DSP_OLED
          act+="\"group_oled\",";
#endif
          if(VU_READY==1 || dbgact){
            act+="\"group_vu\",";
          }
          if(BRIGHTNESS_PIN!=255 || dbgact){
            act+="\"group_brightness\",";
          }
          if(DSP_FLIPPED==1 || dbgact){
            act+="\"group_tft\",";
          }
          if(TS_CS!=255 || dbgact){
            act+="\"group_touch\",";
          }
          if(DSP_MODEL==DSP_NOKIA5110){
            act+="\"group_nokia\",";
          }
          if(DSP_MODEL!=DSP_DUMMY || dbgact){
            act+="\"group_timezone\",";
            
          }
          if (WEATHER_READY==1 || dbgact){
            act+="\"group_weather\",";
          }
          act+="\"group_controls\",";
          if(ENC_BTNL!=255 || ENC2_BTNL!=255 || dbgact){
            act+="\"group_encoder\",";
          }
          if(IR_PIN!=255 || dbgact){
            act+="\"group_ir\",";
          }
        }
        act = act.substring(0, act.length()-1);
        sprintf (buf, "{\"act\":[%s]}", act.c_str());
        break;
    }
    case GETMODE: {
        sprintf (buf, "{\"pmode\":\"%s\"}", network.status == CONNECTED?"player":"ap");
        break;
    }
    case GETINDEX: {
        requestOnChange(STATION, clientId);
        requestOnChange(TITLE, clientId);
        requestOnChange(VOLUME, clientId);
        requestOnChange(EQUALIZER, clientId);
        requestOnChange(BALANCE, clientId);
        requestOnChange(BITRATE, clientId);
        requestOnChange(MODE, clientId);
        //playlistrequest = clientId; /* Cleanup this */
        return;
        break;
    }
    case GETSYSTEM: {
        sprintf (buf, "{\"sst\":%d,\"aif\":%d,\"vu\":%d,\"softr\":%d}", config.store.smartstart!=2, config.store.audioinfo, config.store.vumeter, config.store.softapdelay);
        break;
    }
    case GETSCREEN: {
        sprintf (buf, "{\"flip\":%d,\"inv\":%d,\"nump\":%d,\"tsf\":%d,\"tsd\":%d,\"dspon\":%d,\"br\":%d,\"con\":%d}", config.store.flipscreen, config.store.invertdisplay, config.store.numplaylist, config.store.fliptouch, config.store.dbgtouch, config.store.dspon, config.store.brightness, config.store.contrast);
        break;
    }
    case GETTIMEZONE: {
        sprintf (buf, "{\"tzh\":%d,\"tzm\":%d,\"sntp1\":\"%s\",\"sntp2\":\"%s\"}", config.store.tzHour, config.store.tzMin, config.store.sntp1, config.store.sntp2);
        break;
    }
    case GETWEATHER: {
        sprintf (buf, "{\"wen\":%d,\"wlat\":\"%s\",\"wlon\":\"%s\",\"wkey\":\"%s\"}", config.store.showweather, config.store.weatherlat, config.store.weatherlon, config.store.weatherkey);
        break;
    }
    case GETCONTROLS: {
        sprintf (buf, "{\"vols\":%d,\"enca\":%d,\"irtl\":%d}", config.store.volsteps, config.store.encacc, config.store.irtlp);
        break;
    }
    case DSPON: {
        sprintf (buf, "{\"dspontrue\":%d}", 1);
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
  return String();
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    netserver.takeMallocDog();
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
      if(config.store.audioinfo) Serial.printf("[WEBSOCKET] client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      /*netserver.requestOnChange(STATION, client->id());
      netserver.requestOnChange(TITLE, client->id());
      netserver.requestOnChange(VOLUME, client->id());
      netserver.requestOnChange(EQUALIZER, client->id());
      netserver.requestOnChange(BALANCE, client->id());
      netserver.requestOnChange(BITRATE, client->id());
      netserver.requestOnChange(MODE, client->id());
      netserver.playlistrequest = client->id();*/

      break;
    case WS_EVT_DISCONNECT:
      if(config.store.audioinfo) Serial.printf("[WEBSOCKET] client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      netserver.onWsMessage(arg, data, len, client->id());
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
