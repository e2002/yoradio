#include "options.h"
#include "Arduino.h"
#include "timekeeper.h"
#include "config.h"
#include "network.h"
#include "display.h"
#include "player.h"
#include "netserver.h"
#include "rtcsupport.h"
#include "../displays/tools/l10n.h"
#include "../pluginsManager/pluginsManager.h"
#ifdef USE_NEXTION
#include "../displays/nextion.h"
#endif
#if DSP_MODEL==DSP_DUMMY
#define DUMMYDISPLAY
#endif

#if RTCSUPPORTED
  //#define TIME_SYNC_INTERVAL  24*60*60*1000
  #define TIME_SYNC_INTERVAL  config.store.timeSyncIntervalRTC*60*60*1000
#else
  #define TIME_SYNC_INTERVAL  config.store.timeSyncInterval*60*1000
#endif
#define WEATHER_SYNC_INTERVAL config.store.weatherSyncInterval*60*1000

#define SYNC_STACK_SIZE       1024 * 4
#define SYNC_TASK_CORE        0
#define SYNC_TASK_PRIORITY    3
#define WEATHER_STRING_L      254

#ifdef HEAP_DBG
  void printHeapFragmentationInfo(const char* title){
    size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    float fragmentation = 100.0 * (1.0 - ((float)largestBlock / (float)freeHeap));
    Serial.printf("\n****** %s ******\n", title);
    Serial.printf("* Free heap: %u bytes\n", freeHeap);
    Serial.printf("* Largest free block: %u bytes\n", largestBlock);
    Serial.printf("* Fragmentation: %.2f%%\n", fragmentation);
    Serial.printf("*************************************\n\n");
  }
  #define HEAP_INFO() printHeapFragmentationInfo(__PRETTY_FUNCTION__)
#else
  #define HEAP_INFO()
#endif

TimeKeeper timekeeper;

void _syncTask(void *pvParameters) {
  if (timekeeper.forceWeather && timekeeper.forceTimeSync) {
    timekeeper.timeTask();
    timekeeper.weatherTask();
  } 
  else if (timekeeper.forceWeather) {
    timekeeper.weatherTask();
  }
  else if (timekeeper.forceTimeSync) {
    timekeeper.timeTask();
  }
  timekeeper.busy = false;
  vTaskDelete(NULL);
}

TimeKeeper::TimeKeeper(){
  busy          = false;
  forceWeather  = true;
  forceTimeSync = true;
  _returnPlayerTime = _doAfterTime = 0;
  weatherBuf=NULL;
  #if (DSP_MODEL!=DSP_DUMMY || defined(USE_NEXTION)) && !defined(HIDE_WEATHER)
    weatherBuf = (char *) malloc(sizeof(char) * WEATHER_STRING_L);
    memset(weatherBuf, 0, WEATHER_STRING_L);
  #endif
}

bool TimeKeeper::loop0(){ // core0 (display)
  if (network.status != CONNECTED) return true;
  uint32_t currentTime = millis();
  static uint32_t _last1s = 0;
  static uint32_t _last2s = 0;
  static uint32_t _last5s = 0;
  if (currentTime - _last1s >= 1000) { // 1sec
    _last1s = currentTime;
//#ifndef DUMMYDISPLAY
#if !defined(DUMMYDISPLAY) || defined(USE_NEXTION)
  #ifndef UPCLOCK_CORE1
    _upClock();
  #endif
#endif
  }
  if (currentTime - _last2s >= 2000) { // 2sec
    _last2s = currentTime;
    _upRSSI();
  }
  if (currentTime - _last5s >= 5000) { // 5sec
    _last5s = currentTime;
    //HEAP_INFO();
  }

  return true; // just in case
}

bool TimeKeeper::loop1(){ // core1 (player)
  uint32_t currentTime = millis();
  static uint32_t _last1s = 0;
  static uint32_t _last2s = 0;
  if (currentTime - _last1s >= 1000) { // 1sec
    pm.on_ticker();
    _last1s = currentTime;
//#ifndef DUMMYDISPLAY
#if !defined(DUMMYDISPLAY) || defined(USE_NEXTION)
  #ifdef UPCLOCK_CORE1
    _upClock();
  #endif
#endif
    _upScreensaver();
    _upSDPos();
    _returnPlayer();
    _doAfterWait();
  }
  if (currentTime - _last2s >= 2000) { // 2sec
    _last2s = currentTime;
  }

  //#ifdef DUMMYDISPLAY
  #if defined(DUMMYDISPLAY) && !defined(USE_NEXTION)
  return true;
  #endif
  // Sync weather & time
  static uint32_t lastWeatherTime = 0;
  if (currentTime - lastWeatherTime >= WEATHER_SYNC_INTERVAL) {
    lastWeatherTime = currentTime;
    forceWeather = true;
  }
  static uint32_t lastTimeTime = 0;
  if (currentTime - lastTimeTime >= TIME_SYNC_INTERVAL) {
    lastTimeTime = currentTime;
    forceTimeSync = true;
  }
  if (!busy && (forceWeather || forceTimeSync) && network.status == CONNECTED) {
    busy = true;
    //config.setTimeConf();
    xTaskCreatePinnedToCore(
      _syncTask,
      "syncTask",
      SYNC_STACK_SIZE,
      NULL,           // Params
      SYNC_TASK_PRIORITY,
      NULL,           // Descriptor
      SYNC_TASK_CORE
    );
  }
  
  return true; // just in case
}

void TimeKeeper::waitAndReturnPlayer(uint8_t time_s){
  _returnPlayerTime = millis()+time_s*1000;
}
void TimeKeeper::_returnPlayer(){
  if(_returnPlayerTime>0 && millis()>=_returnPlayerTime){
    _returnPlayerTime = 0;
    display.putRequest(NEWMODE, PLAYER);
  }
}

void TimeKeeper::waitAndDo(uint8_t time_s, void (*callback)()){
  _doAfterTime = millis()+time_s*1000;
  _aftercallback = callback;
}
void TimeKeeper::_doAfterWait(){
  if(_doAfterTime>0 && millis()>=_doAfterTime){
    _doAfterTime = 0;
    _aftercallback();
  }
}

void TimeKeeper::_upClock(){
#if RTCSUPPORTED
  if(config.isRTCFound()) rtc.getTime(&network.timeinfo);
#else
  if(network.timeinfo.tm_year>100 || network.status == SDREADY) {
    network.timeinfo.tm_sec++;
    mktime(&network.timeinfo);
  }
#endif
  if(display.ready()) display.putRequest(CLOCK);
}

void TimeKeeper::_upScreensaver(){
#ifndef DSP_LCD
  if(!display.ready()) return;
  if(config.store.screensaverEnabled && display.mode()==PLAYER && !player.isRunning()){
    config.screensaverTicks++;
    if(config.screensaverTicks > config.store.screensaverTimeout+SCREENSAVERSTARTUPDELAY){
      if(config.store.screensaverBlank){
        display.putRequest(NEWMODE, SCREENBLANK);
      }else{
        display.putRequest(NEWMODE, SCREENSAVER);
      }
      config.screensaverTicks=SCREENSAVERSTARTUPDELAY;
    }
  }
  if(config.store.screensaverPlayingEnabled && display.mode()==PLAYER && player.isRunning()){
    config.screensaverPlayingTicks++;
    if(config.screensaverPlayingTicks > config.store.screensaverPlayingTimeout*60+SCREENSAVERSTARTUPDELAY){
      if(config.store.screensaverPlayingBlank){
        display.putRequest(NEWMODE, SCREENBLANK);
      }else{
        display.putRequest(NEWMODE, SCREENSAVER);
      }
      config.screensaverPlayingTicks=SCREENSAVERSTARTUPDELAY;
    }
  }
#endif
}

void TimeKeeper::_upRSSI(){
  if(network.status == CONNECTED){
    netserver.setRSSI(WiFi.RSSI());
    netserver.requestOnChange(NRSSI, 0);
    if(display.ready()) display.putRequest(DSPRSSI, netserver.getRSSI());
  }
#ifdef USE_SD
  if(display.mode()!=SDCHANGE) player.sendCommand({PR_CHECKSD, 0});
#endif
  player.sendCommand({PR_VUTONUS, 0});
}

void TimeKeeper::_upSDPos(){
  if(player.isRunning() && config.getMode()==PM_SDCARD) netserver.requestOnChange(SDPOS, 0);
}

void TimeKeeper::timeTask(){
  static uint8_t tsFailCnt = 0;
  config.waitConnection();
  if(getLocalTime(&network.timeinfo)){
    tsFailCnt = 0;
    forceTimeSync = false;
    mktime(&network.timeinfo);
    display.putRequest(CLOCK);
    network.requestTimeSync(true);
    #if RTCSUPPORTED
      if (config.isRTCFound()) rtc.setTime(&network.timeinfo);
    #endif
  }else{
    if(tsFailCnt<4){
      forceTimeSync = true;
      tsFailCnt++;
    }else{
      forceTimeSync = false;
      tsFailCnt=0;
    }
  }
}
void TimeKeeper::weatherTask(){
  forceWeather = false;
  if(!weatherBuf || strlen(config.store.weatherkey)==0 || !config.store.showweather) return;
  _getWeather();
}

bool _getWeather() {
#if (DSP_MODEL!=DSP_DUMMY || defined(USE_NEXTION)) && !defined(HIDE_WEATHER)
  static AsyncClient * weatherClient = NULL;
  static const char* host = "api.openweathermap.org";
  if(weatherClient) return false;
  weatherClient = new AsyncClient();
  if(!weatherClient) return false;

  weatherClient->onError([](void * arg, AsyncClient * client, int error){
    Serial.println("##WEATHER###: connection error");
    weatherClient = NULL;
    delete client;
  }, NULL);

  weatherClient->onConnect([](void * arg, AsyncClient * client){
    weatherClient->onError(NULL, NULL);
    weatherClient->onDisconnect([](void * arg, AsyncClient * c){ weatherClient = NULL; delete c; }, NULL);
    
    char httpget[250] = {0};
    sprintf(httpget, "GET /data/2.5/weather?lat=%s&lon=%s&units=%s&lang=%s&appid=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.store.weatherlat, config.store.weatherlon, LANG::weatherUnits, LANG::weatherLang, config.store.weatherkey, host);
    client->write(httpget);
    
    client->onData([](void * arg, AsyncClient * c, void * data, size_t len){
      uint8_t * d = (uint8_t*)data;
      const char *bodyStart = strstr((const char*)d, "\r\n\r\n");
      if (bodyStart != NULL) {
        bodyStart += 4;
        size_t bodyLen = len - (bodyStart - (const char*)d);
        char line[bodyLen+1];
        memcpy(line, bodyStart, bodyLen);
        line[bodyLen] = '\0';
        /* parse it */
        char *cursor;
        char desc[120], icon[5];
        float tempf, tempfl, wind_speed;
        int hum, press, wind_deg;
        bool result = true;

        cursor = strstr(line, "\"description\":\"");
        if (cursor) { sscanf(cursor, "\"description\":\"%119[^\"]", desc); }else{ Serial.println("##WEATHER###: description not found !"); result=false; }
        cursor = strstr(line, "\"icon\":\"");
        if (cursor) { sscanf(cursor, "\"icon\":\"%4[^\"]", icon); }else{ Serial.println("##WEATHER###: icon not found !"); result=false; }
        cursor = strstr(line, "\"temp\":");
        if (cursor) { sscanf(cursor, "\"temp\":%f", &tempf); }else{ Serial.println("##WEATHER###: temp not found !"); result=false; }
        cursor = strstr(line, "\"pressure\":");
        if (cursor) { sscanf(cursor, "\"pressure\":%d", &press); }else{ Serial.println("##WEATHER###: pressure not found !"); result=false; }
        cursor = strstr(line, "\"humidity\":");
        if (cursor) { sscanf(cursor, "\"humidity\":%d", &hum); }else{ Serial.println("##WEATHER###: humidity not found !"); result=false; }
        cursor = strstr(line, "\"feels_like\":");
        if (cursor) { sscanf(cursor, "\"feels_like\":%f", &tempfl); }else{ Serial.println("##WEATHER###: feels_like not found !"); result=false; }
        cursor = strstr(line, "\"grnd_level\":");
        if (cursor) { sscanf(cursor, "\"grnd_level\":%d", &press); }
        cursor = strstr(line, "\"speed\":");
        if (cursor) { sscanf(cursor, "\"speed\":%f", &wind_speed); }else{ Serial.println("##WEATHER###: wind speed not found !"); result=false; }
        cursor = strstr(line, "\"deg\":");
        if (cursor) { sscanf(cursor, "\"deg\":%d", &wind_deg); }else{ Serial.println("##WEATHER###: wind deg not found !"); result=false; }
        press = press / 1.333;

        if(!result) return;

        #ifdef USE_NEXTION
          nextion.putcmdf("press_txt.txt=\"%dmm\"", press);
          nextion.putcmdf("hum_txt.txt=\"%d%%\"", hum);
          char cmd[30];
          snprintf(cmd, sizeof(cmd)-1,"temp_txt.txt=\"%.1f\"", tempf);
          nextion.putcmd(cmd);
          int iconofset;
          if(strstr(icon,"01")!=NULL)      iconofset = 0;
          else if(strstr(icon,"02")!=NULL) iconofset = 1;
          else if(strstr(icon,"03")!=NULL) iconofset = 2;
          else if(strstr(icon,"04")!=NULL) iconofset = 3;
          else if(strstr(icon,"09")!=NULL) iconofset = 4;
          else if(strstr(icon,"10")!=NULL) iconofset = 5;
          else if(strstr(icon,"11")!=NULL) iconofset = 6;
          else if(strstr(icon,"13")!=NULL) iconofset = 7;
          else if(strstr(icon,"50")!=NULL) iconofset = 8;
          else                             iconofset = 9;
          nextion.putcmd("cond_img.pic", 50+iconofset);
          nextion.weatherVisible(1);
        #endif
        
        Serial.printf("##WEATHER###: description: %s, temp:%.1f C, pressure:%dmmHg, humidity:%d%%, wind: %d\n", desc, tempf, press, hum, (int)(wind_deg/22.5));
        #ifdef WEATHER_FMT_SHORT
        sprintf(timekeeper.weatherBuf, weatherFmt, tempf, press, hum);
        #else
          #if EXT_WEATHER
            sprintf(timekeeper.weatherBuf, LANG::weatherFmt, desc, tempf, tempfl, press, hum, wind_speed, LANG::wind[(int)(wind_deg/22.5)]);
          #else
            sprintf(timekeeper.weatherBuf, LANG::weatherFmt, desc, tempf, press, hum);
          #endif
        #endif
        display.putRequest(NEWWEATHER);
      } else {
        Serial.println("##WEATHER###: weather not found !");
      }
    }, NULL); // <-- client->onData
  }, NULL); // <-- weatherClient->onConnect
  config.waitConnection();
  if(!weatherClient->connect(host, 80)){
    Serial.println("##WEATHER###: connection failed");
    AsyncClient * client = weatherClient;
    weatherClient = NULL;
    delete client;
  }

  return true;
#endif // if (DSP_MODEL!=DSP_DUMMY || defined(USE_NEXTION)) && !defined(HIDE_WEATHER)
  return false;
}

//******************
