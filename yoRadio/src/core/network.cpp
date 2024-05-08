#include "network.h"
#include "display.h"
#include "options.h"
#include "config.h"
#include "telnet.h"
#include "netserver.h"
#include "player.h"
#include "mqtt.h"
#include <ArduinoJson.h>

#ifndef WIFI_ATTEMPTS
	#define WIFI_ATTEMPTS	16
#endif

YoNetwork network;

TaskHandle_t syncTaskHandle;
//TaskHandle_t reconnectTaskHandle;

bool getWeather(char *wstr);
void doSync(void * pvParameters);

void ticks() {
  if(!display.ready()) return; //waiting for SD is ready
  
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
    if(timeSyncTicks >= timeSyncInterval){
      timeSyncTicks=0;
      network.forceTimeSync = true;
    }
    if(weatherSyncTicks >= weatherSyncInterval){
      weatherSyncTicks=0;
      network.forceWeather = true;
    }
  }
#if RTCSUPPORTED
	rtc.getTime(&network.timeinfo);
	mktime(&network.timeinfo);
  display.putRequest(CLOCK);
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
  }
}

void YoNetwork::WiFiReconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  network.beginReconnect = false;
  player.lockOutput = false;
  delay(100);
  display.putRequest(NEWMODE, PLAYER);
  if(config.getMode()==PM_SDCARD) {
  	network.status=CONNECTED;
  	display.putRequest(NEWIP, 0);
  }else{
		display.putRequest(NEWMODE, PLAYER);
		if (network.lostPlaying) player.sendCommand({PR_PLAY, config.store.lastStation});
  }
  #ifdef MQTT_ROOT_TOPIC
    connectToMqtt();
  #endif
}

void YoNetwork::WiFiLostConnection(WiFiEvent_t event, WiFiEventInfo_t info){
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

bool YoNetwork::wifiBegin(bool silent){
  uint8_t ls = (config.store.lastSSID == 0 || config.store.lastSSID > config.ssidsCount) ? 0 : config.store.lastSSID - 1;
  uint8_t startedls = ls;
  uint8_t errcnt = 0;
  WiFi.mode(WIFI_STA);
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
      if(YO_LED_BUILTIN!=255 && !silent) digitalWrite(YO_LED_BUILTIN, !digitalRead(YO_LED_BUILTIN));
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

void YoNetwork::begin() {
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
  if(YO_LED_BUILTIN!=255) digitalWrite(YO_LED_BUILTIN, LOW);
  
#if RTCSUPPORTED
	rtc.getTime(&network.timeinfo);
	mktime(&network.timeinfo);
  display.putRequest(CLOCK);
#endif
  ctimer.attach(1, ticks);
  if (network_on_connect) network_on_connect();
}

void YoNetwork::setWifiParams(){
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
    configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), config.store.sntp1, config.store.sntp2);
  }else if(strlen(config.store.sntp1)>0){
    configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), config.store.sntp1);
  }
}

void YoNetwork::requestTimeSync(bool withTelnetOutput, uint8_t clientId) {
  if (withTelnetOutput) {
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    if (config.store.tzHour < 0) {
      telnet.printf(clientId, "##SYS.DATE#: %s%03d:%02d\n> ", timeStringBuff, config.store.tzHour, config.store.tzMin);
    } else {
      telnet.printf(clientId, "##SYS.DATE#: %s+%02d:%02d\n> ", timeStringBuff, config.store.tzHour, config.store.tzMin);
    }
  }
}

void rebootTime() {
  ESP.restart();
}

void YoNetwork::raiseSoftAP() {
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

void YoNetwork::requestWeatherSync(){
  display.putRequest(NEWWEATHER);
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
  //https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&current=temperature_2m,relative_humidity_2m,apparent_temperature,surface_pressure,wind_speed_10m,wind_direction_10m&wind_speed_unit=ms&timezone=GMT
  const char* host  = "api.open-meteo.com";
  
  if (!client.connect(host, 80)) {
    Serial.println("##WEATHER###: connection  failed");
    return false;
  }
  char httpget[300] = {0};
  int j;
  j = snprintf(httpget, sizeof httpget, "GET /v1/forecast?latitude=%s&longitude=%s&current=temperature_2m,relative_humidity_2m,apparent_temperature,surface_pressure,wind_speed_10m,wind_direction_10m&wind_speed_unit=ms&timezone=GMT HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.store.weatherlat, config.store.weatherlon, host);
  if (j >= sizeof httpget) {
    Serial.printf("##WEATHER###: httpget buffer length exceeded; increase size of httpget.\n");
    return false;
  }
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
      if (strstr(line.c_str(), "\"temperature_2m\"") != NULL) {
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
  if (strstr(line.c_str(), "\"temperature_2m\"") == NULL) {
    Serial.println("##WEATHER###: weather not found !");
    return false;
  }

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    Serial.print("##WEATHER###: deserializeJson() failed: ");
    Serial.println(error.c_str());
    Serial.println(line);
    return false;
  }

  JsonObject current = doc["current"];

  float tempf = current["temperature_2m"];
  String hum = current["relative_humidity_2m"];
  float tempfl = current["apparent_temperature"];
  int pressi = current["surface_pressure"];
  float wind_speed = current["wind_speed_10m"];
  int wind_deg = (int)((int)current["wind_direction_10m"]/22.5);
  if(wind_deg<0) wind_deg = 16+wind_deg;

  #ifdef WEATHER_FMT_SHORT
    sprintf(wstr, (const char*)&weatherFmt[3], tempf, pressi, hum.c_str());
  #else
    #if EXT_WEATHER
      sprintf(wstr, (const char*)&weatherFmt[3], tempf, tempfl, pressi, hum.c_str(), wind_speed, wind[wind_deg]);
    #else
      sprintf(wstr, (const char*)&weatherFmt[3], tempf, pressi, hum.c_str());
    #endif
  #endif
  Serial.printf("##WEATHER###: %s\n", wstr);
  network.requestWeatherSync();
  return true;
#endif // if (DSP_MODEL!=DSP_DUMMY || defined(USE_NEXTION)) && !defined(HIDE_WEATHER)
  return false;
}
