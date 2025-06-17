#include "network.h"
#include "display.h"
#include "options.h"
#include "config.h"
#include "telnet.h"
#include "netserver.h"
#include "player.h"
#include "mqtt.h"
#include "../ESPFileUpdater/ESPFileUpdater.h"

#ifndef WIFI_ATTEMPTS
  #define WIFI_ATTEMPTS  16
#endif

MyNetwork network;

TaskHandle_t syncTaskHandle;
//TaskHandle_t reconnectTaskHandle;

bool getWeather(char *wstr);
void doSync(void * pvParameters);

bool wasUpdated(ESPFileUpdater::UpdateStatus status) { return status == ESPFileUpdater::UPDATED; }
ESPFileUpdater updater(SPIFFS);

void ticks() {
  if(!display.ready()) return; //waiting for SD is ready
  pm.on_ticker();
  static const uint16_t weatherSyncInterval=1800;
  //static const uint16_t weatherSyncIntervalFail=10;
#if RTCSUPPORTED
  static const uint32_t timeSyncInterval=86400;
  static uint32_t timeSyncTicks = 0;
#else
  static const uint16_t timeSyncInterval=3600;
  static uint16_t timeSyncTicks = 0;
#endif
  static uint16_t weatherSyncTicks = 0;
  static bool divrssi;
  timeSyncTicks++;
  weatherSyncTicks++;
  divrssi = !divrssi;
  if(network.status == CONNECTED){
    if(network.forceTimeSync || network.forceWeather){
      xTaskCreatePinnedToCore(doSync, "doSync", 1024 * 4, NULL, 0, &syncTaskHandle, 0);
    }
    // check at :01s mark (fix network clock not matching system clock after Daylight Savings Time changes)
    if (network.timeinfo.tm_sec == 1) {
      time_t now = time(NULL);
      struct tm localNow;
      localtime_r(&now, &localNow);
      if ((network.timeinfo.tm_min != localNow.tm_min) || (network.timeinfo.tm_hour != localNow.tm_hour)) {
        timeSyncTicks = 0;
        network.forceTimeSync = true;
      }
    }
    if(timeSyncTicks >= timeSyncInterval){
      timeSyncTicks=0;
      network.forceTimeSync = true;
    }
    if(weatherSyncTicks >= weatherSyncInterval){
      weatherSyncTicks=0;
      network.forceWeather = true;
    }
  }
#ifndef DSP_LCD
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
    if(config.screensaverPlayingTicks > config.store.screensaverPlayingTimeout+SCREENSAVERSTARTUPDELAY){
      if(config.store.screensaverPlayingBlank){
        display.putRequest(NEWMODE, SCREENBLANK);
      }else{
        display.putRequest(NEWMODE, SCREENSAVER);
      }
    }
  }
#endif
#if RTCSUPPORTED
  if(config.isRTCFound()){
    rtc.getTime(&network.timeinfo);
    mktime(&network.timeinfo);
    display.putRequest(CLOCK);
  }
#else
  if(network.timeinfo.tm_year>100 || network.status == SDREADY) {
    network.timeinfo.tm_sec++;
    mktime(&network.timeinfo);
    display.putRequest(CLOCK);
  }
#endif
  if(player.isRunning() && config.getMode()==PM_SDCARD) netserver.requestOnChange(SDPOS, 0);
  if(divrssi) {
    if(network.status == CONNECTED){
      netserver.setRSSI(WiFi.RSSI());
      netserver.requestOnChange(NRSSI, 0);
      display.putRequest(DSPRSSI, netserver.getRSSI());
    }
#ifdef USE_SD
    if(display.mode()!=SDCHANGE) player.sendCommand({PR_CHECKSD, 0});
#endif
    player.sendCommand({PR_VUTONUS, 0});
  }
}

void MyNetwork::WiFiReconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  network.beginReconnect = false;
  player.lockOutput = false;
  delay(100);
  display.putRequest(NEWMODE, PLAYER);
  if(config.getMode()==PM_SDCARD) {
    network.status=CONNECTED;
    display.putRequest(NEWIP, 0);
  }else{
    display.putRequest(NEWMODE, PLAYER);
    if (network.lostPlaying) player.sendCommand({PR_PLAY, config.lastStation()});
  }
  #ifdef MQTT_ROOT_TOPIC
    connectToMqtt();
  #endif
}

void MyNetwork::WiFiLostConnection(WiFiEvent_t event, WiFiEventInfo_t info){
  if(!network.beginReconnect){
    Serial.printf("Lost connection, reconnecting to %s...\n", config.ssids[config.store.lastSSID-1].ssid);
    if(config.getMode()==PM_SDCARD) {
      network.status=SDREADY;
      display.putRequest(NEWIP, 0);
    }else{
      network.lostPlaying = player.isRunning();
      if (network.lostPlaying) { player.lockOutput = true; player.sendCommand({PR_STOP, 0}); }
      display.putRequest(NEWMODE, LOST);
    }
  }
  network.beginReconnect = true;
  WiFi.reconnect();
}

bool MyNetwork::wifiBegin(bool silent){
  uint8_t ls = (config.store.lastSSID == 0 || config.store.lastSSID > config.ssidsCount) ? 0 : config.store.lastSSID - 1;
  uint8_t startedls = ls;
  uint8_t errcnt = 0;
  WiFi.mode(WIFI_STA);
  /*
  char buf[MDNS_LENGTH];
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  if(strlen(config.store.mdnsname)>0){
    WiFi.setHostname(config.store.mdnsname);
  }else{
    snprintf(buf, MDNS_LENGTH, "yoradio-%x", config.getChipId());
    WiFi.setHostname(buf);
  }
  */
  while (true) {
    if(!silent){
      Serial.printf("##[BOOT]#\tAttempt to connect to %s\n", config.ssids[ls].ssid);
      Serial.print("##[BOOT]#\t");
      display.putRequest(BOOTSTRING, ls);
    }
    WiFi.begin(config.ssids[ls].ssid, config.ssids[ls].password);
    while (WiFi.status() != WL_CONNECTED) {
      if(!silent) Serial.print(".");
      delay(500);
      if(REAL_LEDBUILTIN!=255 && !silent) digitalWrite(REAL_LEDBUILTIN, !digitalRead(REAL_LEDBUILTIN));
      errcnt++;
      if (errcnt > WIFI_ATTEMPTS) {
        errcnt = 0;
        ls++;
        if (ls > config.ssidsCount - 1) ls = 0;
        if(!silent) Serial.println();
        break;
      }
    }
    if (WiFi.status() != WL_CONNECTED && ls == startedls) {
      return false; break;
    }
    if (WiFi.status() == WL_CONNECTED) {
      config.setLastSSID(ls + 1);
      return true; break;
    }
  }
  return false;
}

void searchWiFi(void * pvParameters){
  if(!network.wifiBegin(true)){
    delay(10000);
    xTaskCreatePinnedToCore(searchWiFi, "searchWiFi", 1024 * 4, NULL, 0, NULL, 0);
  }else{
    network.status = CONNECTED;
    netserver.begin(true);
    telnet.begin(true);
    network.setWifiParams();
    display.putRequest(NEWIP, 0);
  }
  vTaskDelete( NULL );
}

#define DBGAP false

void MyNetwork::begin() {
  BOOTLOG("network.begin");
  config.initNetwork();
  ctimer.detach();
  forceTimeSync = forceWeather = true;
  if (config.ssidsCount == 0 || DBGAP) {
    raiseSoftAP();
    return;
  }
  if(config.getMode()!=PM_SDCARD){
    if(!wifiBegin()){
      raiseSoftAP();
      Serial.println("##[BOOT]#\tdone");
      return;
    }
    Serial.println(".");
    status = CONNECTED;
    setWifiParams();
  }else{
    status = SDREADY;
    xTaskCreatePinnedToCore(searchWiFi, "searchWiFi", 1024 * 4, NULL, 0, NULL, 0);
  }
  
  Serial.println("##[BOOT]#\tdone");
  if(REAL_LEDBUILTIN!=255) digitalWrite(REAL_LEDBUILTIN, LOW);
  
#if RTCSUPPORTED
  if(config.isRTCFound()){
    rtc.getTime(&network.timeinfo);
    mktime(&network.timeinfo);
    display.putRequest(CLOCK);
  }
#endif
  ctimer.attach(1, ticks);
  if (network_on_connect) network_on_connect();
  pm.on_connect();
}

void MyNetwork::setWifiParams(){
  WiFi.setSleep(false);
  WiFi.onEvent(WiFiReconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiLostConnection, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  weatherBuf=NULL;
  trueWeather = false;
  #if (DSP_MODEL!=DSP_DUMMY || defined(USE_NEXTION)) && !defined(HIDE_WEATHER)
    weatherBuf = (char *) malloc(sizeof(char) * WEATHER_STRING_L);
    memset(weatherBuf, 0, WEATHER_STRING_L);
  #endif
  if(strlen(config.store.sntp1)>0 && strlen(config.store.sntp2)>0){
    configTzTime(config.store.tzposix, config.store.sntp1, config.store.sntp2);
  }else if(strlen(config.store.sntp1)>0){
    configTzTime(config.store.tzposix, config.store.sntp1);
  }
}

void MyNetwork::requestTimeSync(bool withTelnetOutput, uint8_t clientId) {
  if (withTelnetOutput) {
    if (strlen(config.store.sntp1) > 0 && strlen(config.store.sntp2) > 0)
      configTzTime(config.store.tzposix, config.store.sntp1, config.store.sntp2);
    else if (strlen(config.store.sntp1) > 0)
      configTzTime(config.store.tzposix, config.store.sntp1);
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    telnet.printf(clientId, "##SYS.DATE#: %s (%s)\n> ", timeStringBuff, config.store.tzposix);
    telnet.printf(clientId, "##SYS.TZNAME#: %s \n> ", config.store.tz_name);
    telnet.printf(clientId, "##SYS.TZPOSIX#: %s \n> ", config.store.tzposix);
  }
}

void rebootTime() {
  ESP.restart();
}

void MyNetwork::raiseSoftAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, apPassword);
  Serial.println("##[BOOT]#");
  BOOTLOG("************************************************");
  BOOTLOG("Running in AP mode");
  BOOTLOG("Connect to AP %s with password %s", apSsid, apPassword);
  BOOTLOG("and go to http:/192.168.4.1/ to configure");
  BOOTLOG("************************************************");
  status = SOFT_AP;
  if(config.store.softapdelay>0)
    rtimer.once(config.store.softapdelay*60, rebootTime);
}

void MyNetwork::requestWeatherSync(){
  display.putRequest(NEWWEATHER);
}

void updateTZjson(void* param) {
  Serial.println("[ESPFileUpdater: Timezones.json] Called by TimeSync");
  ESPFileUpdater* updater = (ESPFileUpdater*)param;
  ESPFileUpdater::UpdateStatus result = updater->checkAndUpdate(
      "/www/timezones.json.gz",
      TIMEZONES_JSON_GZ_URL,
      "1 week", // update once a week at most
      false // verbose logging
  );
  if (result == ESPFileUpdater::UPDATED) {
    Serial.println("[ESPFileUpdater: Timezones.json] Update completed.");
  } else if (result == ESPFileUpdater::NOT_MODIFIED) {
    Serial.println("[ESPFileUpdater: Timezones.json] No update needed.");
  } else {
    Serial.println("[ESPFileUpdater: Timezones.json] Update failed.");
  }
  vTaskDelete(NULL);
}

void doSync( void * pvParameters ) {
  static uint8_t tsFailCnt = 0;
  //static uint8_t wsFailCnt = 0;
  if(network.forceTimeSync){
    network.forceTimeSync = false;
    if(getLocalTime(&network.timeinfo)){
      tsFailCnt = 0;
      network.forceTimeSync = false;
      mktime(&network.timeinfo);
      display.putRequest(CLOCK);
      network.requestTimeSync(true);
      #if RTCSUPPORTED
        if (config.isRTCFound()) rtc.setTime(&network.timeinfo);
      #endif
        xTaskCreate(updateTZjson, "updateTZjson", 8192, &updater, 1, NULL);
    }else{
      if(tsFailCnt<4){
        network.forceTimeSync = true;
        tsFailCnt++;
      }else{
        network.forceTimeSync = false;
        tsFailCnt=0;
      }
    }
  }
  if(network.weatherBuf && (strlen(config.store.weatherkey)!=0 && config.store.showweather) && network.forceWeather){
    network.forceWeather = false;
    network.trueWeather=getWeather(network.weatherBuf);
  }
  vTaskDelete( NULL );
}

bool getWeather(char *wstr) {
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

//		  Serial.printf("## OPENWEATHERMAP ###: *\n%s,\n*\n", line.c_str());

  char *tmpe;
  char *tmps;
  char *tmpc;
  int pressi, deg, gusti;
  #ifndef GRND_HEIGHT
    #define GRND_HEIGHT  0
  #endif
  int g_height = (float)(GRND_HEIGHT / 11);
  const char* cursor = line.c_str();
  char desc[120], temp[20], hum[20], press[20], icon[5], gust[20], porv[10], stanc[50];

  tmps = strstr(cursor, "\"description\":\"");
  if (tmps == NULL) { Serial.println("##WEATHER###: description not found !"); return false;}
  tmps += 15;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: description content not found !"); return false;}
  strlcpy(desc, tmps, tmpe - tmps + 1);
  cursor = tmps;
//    Serial.printf("#CONTROL#: descr.: %s,\n", desc);

  // "sky clear","icon":"01d"}],
  tmps = strstr(cursor, "\"icon\":\"");
  if (tmps == NULL) { Serial.println("##WEATHER###: icon not found !"); return false;}
  tmps += 8;
  tmpe = strstr(tmps, "\"}");
  if (tmpe == NULL) { Serial.println("##WEATHER###: icon content not found !"); return false;}
  strlcpy(icon, tmps, tmpe - tmps + 1);
  cursor = tmps;

  tmps = strstr(cursor, "\"temp\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: temp not found !"); return false;}
  tmps += 7;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: temp content not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + 1);
  cursor = tmps;
  float tempf = atof(temp);
//    Serial.printf("#CONTROL#: temp: %+.1fC\n", tempf);

  tmps = strstr(cursor, "\"feels_like\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: feels_like not found !"); return false;}
  tmps += 13;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: feels_like content not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + 1);
  cursor = tmps;
  float tempfl = atof(temp);
//  (void)tempfl;						// ?
//    Serial.printf("#CONTROL#: feels like: %+.0fC\n", tempfl);

  tmps = strstr(cursor, "\"pressure\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: pressure not found !"); return false;}
  tmps += 11;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: pressure content not found !"); return false;}
  strlcpy(press, tmps, tmpe - tmps + 1);
  cursor = tmps;
      pressi = (float)atoi(press) / 1.333 - g_height;		// перевод в мм.рт.ст., ввод в целое число pressi поправки (-21) на выс. местности
//      Serial.printf("#CONTROL#: pres.: %d mmHg\n", pressi);

  tmps = strstr(cursor, "humidity\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: humidity not found !"); return false;}
  tmps += 10;
  tmpe = strstr(tmps, ",\"");
  tmpc = strstr(tmps, "},");
  if (tmpe == NULL) { Serial.println("##WEATHER###: humidity not found !"); return false;}
  cursor = tmps;
  strlcpy(hum, tmps, tmpe - tmps + (tmpc>tmpe?1:(tmpc - tmpe +1)));
//      Serial.printf("#CONTROL#: humidity: %s %%\n", hum);

  tmps = strstr(cursor, "\"grnd_level\":");
  bool grnd_level_pr = (tmps != NULL);
  if(grnd_level_pr){
    tmps += 13;
    tmpe = strstr(tmps, "},");
    tmpc = strstr(tmps, ",\"");						// или адрес до [},]
    if (tmpe == NULL) { Serial.println("##WEATHER###: grnd_level not found ! Use pressure");}
    strlcpy(press, tmps, tmpe - tmps + (tmpc>tmpe?1:(tmpc - tmpe +1)));	// вписали в press строку данных (press="991")
    cursor = tmps;
    pressi = (float)atoi(press) / 1.333;			// преобразовали в целое число, перевели в мм.рт.ст. (pressi=743)
 			 }
//      Serial.printf("#CONTROL#: press. grnd_level: %d mmHg\n", pressi);

  tmps = strstr(cursor, "\"speed\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: wind speed not found !"); return false;}
  tmps += 8;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: wind speed content not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + 1);
  cursor = tmps;
  float wind_speed = atof(temp);
//  (void)wind_speed;					// ?
//    Serial.printf("#CONTROL#: wind: %.0f m/s\n", wind_speed);
  
  tmps = strstr(cursor, "\"deg\":");
  if (tmps == NULL) { Serial.println("##WEATHER###: wind deg not found !"); return false;}
  tmps += 6;
  tmpe = strstr(tmps, ",\"");
  tmpc = strstr(tmps, "},");				// или адрес до[},]
  if (tmpe == NULL) { Serial.println("## WEATHER ###: deg content not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + (tmpc>tmpe?1:(tmpc - tmpe +1)));	// вписали в temp строку данных (temp="316")
  cursor = tmps;
      deg = atof(temp);
  int wind_deg = atof(temp)/22.5;		// преобр. в целое число и перевели в полурумбы (wind_deg=14) 
//  if(wind_deg<0) wind_deg = 16+wind_deg;			//отрицательным не бывает
//    Serial.printf("#CONTROL#: wind deg: %d rumbs (*%d*)\n", wind_deg, deg);
  
  		// Проверяем наличие ["gust":13.09}] и добавляем его целое текстовое в строку gust
  tmps = strstr(cursor, "\"gust\":");			// поиск ["gust":] 7
  strlcpy(gust, const_getWeather, sizeof(gust));	// вписали в gust ("")
  if (tmps == NULL) { Serial.println("## WEATHER ###: gust not found !\n");}
  else {
	  tmps += 7;						// добавили 7
	  tmpe = strstr(tmps, "},");				// до [},]
	  if (tmpe == NULL) { Serial.println("## WEATHER ###: gust content not found !");}
	  else {
		  strlcpy(temp, tmps, tmpe - tmps + 1);	// вписали в temp текстовую строку (temp="13.09")
		      gusti = (float)atoi(temp);		// преобразовали в целое число (gusti=13)
		  if (gusti == 0) { Serial.println("## WEATHER ###: gust content is 0 !");}
		  else {
			  strlcpy(gust, prv, sizeof(gust));	// вписали в gust константу *prv (", ПОРЫВЫ ")
			  itoa(gusti, porv, 10);				// преобразовали gusti в текстовую строку (porv="13")
			  strlcat(gust, porv, sizeof(gust));		// добавили к gust текстовую строку porv (", ПОРЫВЫ 13")
			  }
		  cursor = tmps;
		  }
	 }
//    Serial.printf("#CONTROL#: gusts: %s m/s\n", gust);

  tmps = strstr(cursor, "\"name\":\"");
  if (tmps == NULL) { Serial.println("##WEATHER###: name station not found !"); return false;}
  tmps += 8;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) { Serial.println("##WEATHER###: name station not found !"); return false;}
  strlcpy(stanc, tmps, tmpe - tmps + 1);		// вписали в stanc метеостанцию
//    Serial.printf("#CONTROL#: station: %s\n", stanc);
  
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
  
  Serial.printf("##WEATHER###: descr.: %s, temp.: %+.1f*C (feels like %+.0f*C) \007 press.: %d mm \007 hum.: %s%% \007 wind %s %.0f%s m/s (st. %s)\n", desc, tempf, tempfl, pressi, hum, wind[wind_deg], wind_speed, gust, stanc);
//  Serial.printf("##WEATHER###: description: %s, temp:%+.1f C, pressure:%dmmHg, humidity:%s%%\n", desc, tempf, pressi, hum);
  #ifdef WEATHER_FMT_SHORT
  sprintf(wstr, weatherFmt, tempf, pressi, hum);
  #else
    #if EXT_WEATHER
      sprintf(wstr, weatherFmt, desc, tempf, tempfl, pressi, hum, wind[wind_deg], wind_speed, gust, stanc);
    #else
      sprintf(wstr, weatherFmt, desc, tempf, pressi, hum);
    #endif
  #endif
  network.requestWeatherSync();
  return true;
#endif // if (DSP_MODEL!=DSP_DUMMY || defined(USE_NEXTION)) && !defined(HIDE_WEATHER)
  return false;
}
