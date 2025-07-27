#include "timekeeper.h"
#include "options.h"
#include "config.h"
#include "network.h"
#include "display.h"
#include "player.h"
#include "netserver.h"
#include "rtcsupport.h"

#if RTCSUPPORTED
  //#define TIME_SYNC_INTERVAL  24*60*60*1000
  #define TIME_SYNC_INTERVAL  config.store.timeSyncIntervalRTC*60*60*1000
#else
  #define TIME_SYNC_INTERVAL  config.store.timeSyncInterval*60*1000
#endif
#define WEATHER_SYNC_INTERVAL config.store.weatherSyncInterval*60*1000

#define SYNC_STACK_SIZE       1024 * 4
#define SYNC_TASK_CORE        0
#define SYNC_TASK_PRIORITY    1

TimeKeeper timekeeper;

void _syncTask(void *pvParameters) {
  if (timekeeper.forceWeather && timekeeper.forceTimeSync) {
    timekeeper.weatherTask();
    timekeeper.timeTask();
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

bool TimeKeeper::loop0(){ // core0 (display)
  uint32_t currentTime = millis();
  static uint32_t _last1s = 0;
  static uint32_t _last2s = 0;
  static uint32_t _last5s = 0;
  if (currentTime - _last1s >= 1000) { // 1sec
    _last1s = currentTime;
#ifndef DUMMYDISPLAY
  #ifndef UPCLOCK_CORE1
    _upClock();
  #endif
#endif
  }
  if (currentTime - _last2s >= 2000) { // 2sec
    _last2s = currentTime;
    _upRSSI();
  }
  if (currentTime - _last5s >= 5000) { // 2sec
    _last5s = currentTime;
    //HEAP_INFO();
  }
  #ifdef DUMMYDISPLAY
  return true;
  #endif
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

bool TimeKeeper::loop1(){ // core1 (player)
  uint32_t currentTime = millis();
  static uint32_t _last1s = 0;
  static uint32_t _last2s = 0;
  if (currentTime - _last1s >= 1000) { // 1sec
    pm.on_ticker();
    _last1s = currentTime;
#ifndef DUMMYDISPLAY
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
  if(config.isRTCFound()){
    rtc.getTime(&network.timeinfo);
    mktime(&network.timeinfo);
    if(display.ready()) display.putRequest(CLOCK);
  }
#else
  if(network.timeinfo.tm_year>100 || network.status == SDREADY) {
    network.timeinfo.tm_sec++;
    mktime(&network.timeinfo);
    if(display.ready()) display.putRequest(CLOCK);
  }
#endif
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
  if(!network.weatherBuf || strlen(config.store.weatherkey)==0 || !config.store.showweather) return;
  forceWeather = false;
  _getWeather(network.weatherBuf);
}

bool _getWeather(char *wstr) {
#if (DSP_MODEL!=DSP_DUMMY || defined(USE_NEXTION)) && !defined(HIDE_WEATHER)

  WiFiClient client;
  const char* host  = "api.openweathermap.org";
  if (!client.connect(host, 80)) {
    Serial.println("##WEATHER###: connection  failed");
    return false;
  }
  char httpget[250] = {0};
  sprintf(httpget, "GET /data/2.5/weather?lat=%s&lon=%s&units=%s&lang=%s&appid=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.store.weatherlat, config.store.weatherlon, weatherUnits, weatherLang, config.store.weatherkey, host);
  client.print(httpget);
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 2000UL) {
      Serial.println("##WEATHER###: client available timeout !");
      client.stop();
      return false;
    }
  }
  timeout = millis();
  String line = "";
  if (client.connected()) {
    while (client.available())
    {
      line = client.readStringUntil('\n');
      if (strstr(line.c_str(), "\"temp\"") != NULL) {
        client.stop();
        break;
      }
      if ((millis() - timeout) > 500)
      {
        client.stop();
        Serial.println("##WEATHER###: client read timeout !");
        return false;
      }
    }
  }
  if (strstr(line.c_str(), "\"temp\"") == NULL) {
    Serial.println("##WEATHER###: weather not found !");
    return false;
  }
  char *tmpe;
  char *tmps;
  char *tmpc;
  const char* cursor = line.c_str();
  char desc[120], temp[20], hum[20], press[20], icon[5];

  tmps = strstr(cursor, "\"description\":\"");
  if (tmps == NULL) { Serial.println("##WEATHER###: description not found !"); return false;}
  tmps += 15;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: description not found !"); return false;}
  strlcpy(desc, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  
  // "ясно","icon":"01d"}],
  tmps = strstr(cursor, "\"icon\":\"");
  if (tmps == NULL) { Serial.println("##WEATHER###: icon not found !"); return false;}
  tmps += 8;
  tmpe = strstr(tmps, "\"}");
  if (tmpe == NULL) { Serial.println("##WEATHER###: icon not found !"); return false;}
  strlcpy(icon, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  
  tmps = strstr(cursor, "\"temp\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: temp not found !"); return false;}
  tmps += 7;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: temp not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + 1);
  cursor = tmpe + 1;
  float tempf = atof(temp);

  tmps = strstr(cursor, "\"feels_like\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: feels_like not found !"); return false;}
  tmps += 13;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: feels_like not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  float tempfl = atof(temp); (void)tempfl;

  tmps = strstr(cursor, "\"pressure\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: pressure not found !"); return false;}
  tmps += 11;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: pressure not found !"); return false;}
  strlcpy(press, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  int pressi = (float)atoi(press) / 1.333;
  
  tmps = strstr(cursor, "humidity\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: humidity not found !"); return false;}
  tmps += 10;
  tmpe = strstr(tmps, ",\"");
  tmpc = strstr(tmps, "}");
  if (tmpe == NULL) { Serial.println("##WEATHER###: humidity not found !"); return false;}
  strlcpy(hum, tmps, tmpe - tmps + (tmpc>tmpe?1:0));
  
  tmps = strstr(cursor, "\"grnd_level\":");
  bool grnd_level_pr = (tmps != NULL);
  if(grnd_level_pr){
    tmps += 13;
    tmpe = strstr(tmps, ",\"");
    if (tmpe == NULL) { Serial.println("##WEATHER###: grnd_level not found !"); return false;}
    strlcpy(press, tmps, tmpe - tmps + 1);
    cursor = tmpe + 2;
    pressi = (float)atoi(press) / 1.333;
  }
  
  tmps = strstr(cursor, "\"speed\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: wind speed not found !"); return false;}
  tmps += 8;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: wind speed not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + 1);
  cursor = tmpe + 1;
  float wind_speed = atof(temp); (void)wind_speed;
  
  tmps = strstr(cursor, "\"deg\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: wind deg not found !"); return false;}
  tmps += 6;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: wind deg not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + 1);
  cursor = tmpe + 1;
  int wind_deg = atof(temp)/22.5;
  if(wind_deg<0) wind_deg = 16+wind_deg;
  
  
  #ifdef USE_NEXTION
    nextion.putcmdf("press_txt.txt=\"%dmm\"", pressi);
    nextion.putcmdf("hum_txt.txt=\"%d%%\"", atoi(hum));
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
  
  Serial.printf("##WEATHER###: description: %s, temp:%.1f C, pressure:%dmmHg, humidity:%s%%\n", desc, tempf, pressi, hum);
  #ifdef WEATHER_FMT_SHORT
  sprintf(wstr, weatherFmt, tempf, pressi, hum);
  #else
    #if EXT_WEATHER
      sprintf(wstr, weatherFmt, desc, tempf, tempfl, pressi, hum, wind_speed, wind[wind_deg]);
    #else
      sprintf(wstr, weatherFmt, desc, tempf, pressi, hum);
    #endif
  #endif
  display.putRequest(NEWWEATHER);
  return true;
#endif // if (DSP_MODEL!=DSP_DUMMY || defined(USE_NEXTION)) && !defined(HIDE_WEATHER)
  return false;
}

//******************
