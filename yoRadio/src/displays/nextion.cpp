#include "../../options.h"

#if NEXTION_RX!=255 && NEXTION_TX!=255
#include "nextion.h"
#include "../../config.h"

#include "../../player.h"
#include "../../controls.h"
#include "../../netserver.h"
#include "../../network.h"

#ifndef CORE_STACK_SIZE
#define CORE_STACK_SIZE  1024*3
#endif

HardwareSerial hSerial(1); // use UART1
Ticker weatherticker;
//char weather[254] = { 0 };
//bool weatherRequest = false;


const char *ndow[7] = {"воскресенье","понедельник","вторник","среда","четверг","пятница","суббота"};
const char *nmnths[12] = {"января","февраля","марта","апреля","мая","июня","июля","августа","сентября","октября","ноября","декабря"};

#ifdef DUMMYDISPLAY
void ticks() {
  network.timeinfo.tm_sec ++;
  mktime(&network.timeinfo);
  nextion.putRequest({CLOCK,0});
  if(nextion.mode==TIMEZONE) nextion.localTime(network.timeinfo);
  if(nextion.mode==INFO)     nextion.rssi();
  if(nextion.dt){
    int rssi = WiFi.RSSI();
    netserver.setRSSI(rssi);
  }
  nextion.dt=!nextion.dt;
}
#endif

Nextion::Nextion() {

}

void nextionCore0( void * pvParameters ){
  delay(500);
  while(true){
//    if(displayQueue==NULL) break;
    nextion.loop();
    vTaskDelay(5);
  }
  vTaskDelete( NULL );
}

void Nextion::createCore0Task(){
  xTaskCreatePinnedToCore(
      nextionCore0,               /* Task function. */
      "TaskCore0",                /* name of task. */
      CORE_STACK_SIZE,            /* Stack size of task */
      NULL,                       /* parameter of the task */
      4,                          /* no one flies higher than the Toruk */
      &_TaskCore0,                /* Task handle to keep track of created task */
      !xPortGetCoreID());         /* pin task to core 0 */
}

void Nextion::begin(bool dummy) {
  _dummyDisplay=dummy;
  hSerial.begin(NEXTION_BAUD, SERIAL_8N1, NEXTION_RX, NEXTION_TX);
  if (!hSerial) {
    Serial.println("Invalid HardwareSerial pin configuration, check config");
    while (1) {
      delay (1000);
    }
  }
  rx_pos = 0;
  _volInside=false;
  snprintf(_espcoreversion, sizeof(_espcoreversion) - 1, "%d.%d.%d", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
  putcmd("");
  putcmd("rest");
  delay(200);
  putcmd("");
  putcmd("bkcmd=0");
//  putcmd("page boot");
  if(dummy) {
    _displayQueue = xQueueCreate( 5, sizeof( requestParams_t ) );
    createCore0Task();
  }
}

void Nextion::start(){
  if (network.status != CONNECTED) {
    apScreen();
    return;
  }
#ifdef DUMMYDISPLAY
  if(_dummyDisplay) _timer.attach_ms(1000, ticks);
  display.mode = PLAYER;
  config.setTitle("[READY]");
#endif
  mode = PLAYER;
  putcmd("page player");
  delay(100);
#ifdef DUMMYDISPLAY
  newNameset(config.station.name);
  newTitle(config.station.title);
#endif
  setVol(config.store.volume, mode == VOL);
}

void Nextion::apScreen() {
  putcmd("apscreenlock=1");
  putcmd("page settings_wifi");
  //char cmd[20];
  /*for(int i=0;i<5;i++){
    snprintf(cmd, sizeof(cmd)-1, "vis b%d,%d", i, 0);
    putcmd(cmd);
  }*/
  //putcmd("vis btnBack,0");
  
}

void Nextion::putRequest(requestParams_t request){
  if(_displayQueue==NULL) return;
  xQueueSend(_displayQueue, &request, portMAX_DELAY);
}

void Nextion::processQueue(){
  if(_displayQueue==NULL) return;
  requestParams_t request;
  if(xQueueReceive(_displayQueue, &request, 20)){
    switch (request.type){
      case NEWMODE: {
        swichMode((displayMode_e)request.payload);
        break;
      }
      case CLOCK: {
        printClock(network.timeinfo);
        break;
      }
      case NEWTITLE: {
        newTitle(config.station.title);
        break;
      }
      case RETURNTITLE: {
        //returnTile();
        break;
      }
      case NEWSTATION: {
        newNameset(config.station.name);
        bitrate(config.station.bitrate);
        bitratePic(ICON_NA);
        break;
      }
      case NEXTSTATION: {
        drawNextStationNum((displayMode_e)request.payload);
        break;
      }
      case DRAWPLAYLIST: {
        int p = request.payload ? display.currentPlItem + 1 : display.currentPlItem - 1;
        if (p < 1) p = config.store.countStation;
        if (p > config.store.countStation) p = 1;
        display.currentPlItem = p;
        drawPlaylist(display.currentPlItem);
        break;
      }
      case DRAWVOL: {
        if(!_volInside){
          setVol(config.store.volume, mode == VOL);
        }
        _volInside=false;
        break;
      }
    }
  }
  switch (mode) {
    case PLAYER: {
        //drawPlayer();
        break;
      }
    case INFO:
    case TIMEZONE: {
        //sendInfo();
        break;
      }
    case VOL: {
        if (millis() - _volDelay > 3000) {
          _volDelay = millis();
          swichMode(PLAYER);
        }
        break;
      }
    case NUMBERS: {
        //meta.loop();
        break;
      }
    case STATIONS: {
        //plCurrent.loop();
        break;
      }
    default:
        break;
  }
}

void Nextion::loop() {
  processQueue();
  drawVU();
  char RxTemp;
  char scanBuf[50];
  int  scanDigit; (void)scanDigit;
  static String wifisettings;
  if (hSerial.available() > 4) {
    RxTemp = hSerial.read();
    if (RxTemp != '^') {
      return;
    }else{
      rx_pos = 0;
      rxbuf[rx_pos] = '\0';
    }
    while (hSerial.available()) {
      RxTemp = hSerial.read();
      if (RxTemp == '^') {
        rx_pos = 0;
        rxbuf[rx_pos] = '\0';
        continue;
      }
      if (RxTemp != '$') {
        rxbuf[rx_pos] = RxTemp;
        rx_pos++;
      } else {
        rxbuf[rx_pos] = '\0';
        rx_pos = 0;
        if (sscanf(rxbuf, "page=%s", scanBuf) == 1){
          if(strcmp(scanBuf, "player") == 0) display.putRequest({NEWMODE, PLAYER});
          if(strcmp(scanBuf, "playlist") == 0) display.putRequest({NEWMODE, STATIONS});
          if(strcmp(scanBuf, "info") == 0) {
            putcmd("yoversion.txt", VERSION);
            putcmd("espcore.txt", _espcoreversion);
            putcmd("ipaddr.txt", WiFi.localIP().toString().c_str());
            putcmd("ssid.txt", WiFi.SSID().c_str());
            display.putRequest({NEWMODE, INFO});
          }
          if(strcmp(scanBuf, "eq") == 0) {
            putcmd("t4.txt", config.store.balance, true);
            putcmd("h0.val", config.store.balance+16);
            putcmd("t5.txt", config.store.trebble, true);
            putcmd("h1.val", config.store.trebble+16);
            putcmd("t6.txt", config.store.middle, true);
            putcmd("h2.val", config.store.middle+16);
            putcmd("t7.txt", config.store.bass, true);
            putcmd("h3.val", config.store.bass+16);
            display.putRequest({NEWMODE, SETTINGS});
          }
          if(strcmp(scanBuf, "wifi") == 0) {
            if(mode != WIFI){
              char cell[10];
              wifisettings="";
              for(int i=0;i<config.ssidsCount;i++){
                snprintf(cell, sizeof(cell) - 1, "t%d.txt", i*2);
                putcmd(cell, config.ssids[i].ssid);
                snprintf(cell, sizeof(cell) - 1, "t%d.txt", i*2+1);
                putcmd(cell, config.ssids[i].password);
              }
              display.putRequest({NEWMODE, WIFI});
            }
          }
          if(strcmp(scanBuf, "time") == 0) {
            putcmdf("tzHourText.txt=\"%02d\"", config.store.tzHour);
            putcmd("tzHour.val", config.store.tzHour);
            putcmdf("tzMinText.txt=\"%02d\"", config.store.tzMin);
            putcmd("tzMin.val", config.store.tzMin);
            display.putRequest({NEWMODE, TIMEZONE});
          }
          if(strcmp(scanBuf, "sys") == 0) {
            putcmd("smartstart.val", config.store.smartstart==2?0:1);
            putcmd("audioinfo.val", config.store.audioinfo);
            display.putRequest({NEWMODE, SETTINGS});
          }
        }
        if (sscanf(rxbuf, "ctrls=%s", scanBuf) == 1){
          if(strcmp(scanBuf, "up") == 0) {
            display.resetQueue();
            display.putRequest({DRAWPLAYLIST, false});
          }
          if(strcmp(scanBuf, "dn") == 0) {
            display.resetQueue();
            display.putRequest({DRAWPLAYLIST, true});
          }
          if(strcmp(scanBuf, "go") == 0) {
            display.putRequest({NEWMODE, PLAYER});
            player.request.station=display.currentPlItem;
          }
          if(strcmp(scanBuf, "toggle") == 0) {
            player.toggle();
          }
        }
        if (sscanf(rxbuf, "vol=%d", &scanDigit) == 1){
          _volInside = true;
          player.request.volume=scanDigit;
          player.request.doSave=true;
        }
        if (sscanf(rxbuf, "balance=%d", &scanDigit) == 1){
          config.setBalance((int8_t)scanDigit);
          player.setBalance(config.store.balance);
          netserver.requestOnChange(BALANCE, 0);
        }
        if (sscanf(rxbuf, "treble=%d", &scanDigit) == 1){
          player.setTone(config.store.bass, config.store.middle, scanDigit);
          config.setTone(config.store.bass, config.store.middle, scanDigit);
          netserver.requestOnChange(EQUALIZER, 0);
        }
        if (sscanf(rxbuf, "middle=%d", &scanDigit) == 1){
          player.setTone(config.store.bass, scanDigit, config.store.trebble);
          config.setTone(config.store.bass, scanDigit, config.store.trebble);
          netserver.requestOnChange(EQUALIZER, 0);
        }
        if (sscanf(rxbuf, "bass=%d", &scanDigit) == 1){
          player.setTone(scanDigit, config.store.middle, config.store.trebble);
          config.setTone(scanDigit, config.store.middle, config.store.trebble);
          netserver.requestOnChange(EQUALIZER, 0);
        }
        if (sscanf(rxbuf, "tzhour=%d", &scanDigit) == 1){
          config.setTimezone((int8_t)scanDigit, config.store.tzMin);
          configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), SNTP_SERVER);
          network.requestTimeSync(true);
        }
        if (sscanf(rxbuf, "tzmin=%d", &scanDigit) == 1){
          config.setTimezone(config.store.tzHour, (int8_t)scanDigit);
          configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), SNTP_SERVER);
          network.requestTimeSync(true);
        }
        if (sscanf(rxbuf, "audioinfo=%d", &scanDigit) == 1){
          config.store.audioinfo = scanDigit;
          config.save();
        }
        if (sscanf(rxbuf, "smartstart=%d", &scanDigit) == 1){
          config.store.smartstart = scanDigit==0?2:1;
          config.save();
        }
        if (sscanf(rxbuf, "addssid=%s", scanBuf) == 1){
          wifisettings+=(String(scanBuf)+"\t");
        }
        if (sscanf(rxbuf, "addpass=%s", scanBuf) == 1){
          wifisettings+=(String(scanBuf)+"\n");
        }
        if (sscanf(rxbuf, "wifidone=%d", &scanDigit) == 1){
          config.saveWifi(wifisettings.c_str());
        }
      }
    }
  }
}

void Nextion::drawVU(){
  //if(mode!=PLAYER) return;
  if(mode!=PLAYER && mode!=VOL) return;
  static uint8_t measL, measR;
  player.getVUlevel();
  uint8_t L = map(player.vuLeft, 0, 255, 0, 100);
  uint8_t R = map(player.vuRight, 0, 255, 0, 100);
  if(player.isRunning()){
    measL=(L<=measL)?measL-5:L;
    measR=(R<=measR)?measR-5:R;
  }else{
    if(measL>0) measL-=5;
    if(measR>0) measR-=5;
  }
  if(measL>100) measL=0;
  if(measR>100) measR=0;
  fillVU(measL, measR);
}

void Nextion::putcmd(const char* cmd) {
  snprintf(txbuf, sizeof(txbuf) - 1, "%s\xFF\xFF\xFF", cmd);
  hSerial.print(txbuf);
}

void Nextion::putcmd(const char* cmd, const char* val, uint16_t dl) {
  snprintf(txbuf, sizeof(txbuf) - 1, "%s=\"%s\"\xFF\xFF\xFF", cmd, val);
  hSerial.print(txbuf);
  if(dl>0) delay(dl);
}

void Nextion::putcmd(const char* cmd, int val, bool toString, uint16_t dl) {
  if(toString){
    snprintf(txbuf, sizeof(txbuf) - 1, "%s=\"%d\"\xFF\xFF\xFF", cmd, val);
  }else{
    snprintf(txbuf, sizeof(txbuf) - 1, "%s=%d\xFF\xFF\xFF", cmd, val);
  }
  hSerial.print(txbuf);
  if(dl>0) delay(dl);
}

void Nextion::putcmdf(const char* fmt, int val, uint16_t dl) {
  snprintf(txbuf, sizeof(txbuf) - 1, fmt, val);
  hSerial.print(txbuf);
  hSerial.print("\xFF\xFF\xFF");
  if(dl>0) delay(dl);
}

void Nextion::bitrate(int bpm){
  if(bpm>0){
    putcmd("player.bitrate.txt", bpm, true);
  }else{
    putcmd("player.bitrate.txt=\"und\"");
  }
}

void Nextion::rssi(){
  putcmdf("rssi.txt=\"%d dBm\"", WiFi.RSSI());
}

void Nextion::weatherVisible(uint8_t vis){
  putcmd("weatherVisible", vis, false, 20);
  putcmdf("vis press_img,%d", vis, 20);
  putcmdf("vis press_txt,%d", vis, 20);
  putcmdf("vis hum_img,%d", vis, 20);
  putcmdf("vis hum_txt,%d", vis, 20);
  putcmdf("vis temp_img,%d", vis, 20);
  putcmdf("vis temp_txt,%d", vis, 20);
  putcmdf("vis cond_img,%d", vis, 20);
}

void Nextion::bitratePic(uint8_t pic){
  putcmd("player.bitrate.pic", pic);
}

void Nextion::bootString(const char* bs) {
  char buf[50] = { 0 };
  strlcpy(buf, bs, 50);
  putcmd("boot.bootstring.txt", utf8Rus(buf, false));
}

void Nextion::newNameset(const char* meta){
  char newnameset[59] = { 0 };
  strlcpy(newnameset, meta, 59);
  putcmd("player.meta.txt", utf8Rus(newnameset, true));
}

void Nextion::setVol(uint8_t vol, bool dialog){
  if(dialog){
    putcmd("dialog.text.txt", vol, true);
  }/*else{
    putcmd("player.volText.txt", vol, true);
    putcmd("player.volumeSlider.val", vol);
  }*/
  putcmd("player.volText.txt", vol, true);
  putcmd("player.volumeSlider.val", vol);
}

void Nextion::fillVU(uint8_t LC, uint8_t RC){
  putcmd("player.vul.val", LC);
  putcmd("player.vur.val", RC);
}

void Nextion::newTitle(const char* title){
  char ttl[50] = { 0 };
  char sng[50] = { 0 };
  if (strlen(title) > 0) {
    char* ici;
    if ((ici = strstr(title, " - ")) != NULL) {
      strlcpy(sng, ici + 3, 50);
      strlcpy(ttl, title, strlen(title) - strlen(ici) + 1);
    } else {
      strlcpy(ttl, title, 50);
      sng[0] = '\0';
    }
    putcmd("player.title1.txt", utf8Rus(ttl, true));
    putcmd("player.title2.txt", utf8Rus(sng, true));
  }
}

void Nextion::printClock(struct tm timeinfo){
  char timeStringBuff[70] = { 0 };
  strftime(timeStringBuff, sizeof(timeStringBuff), "player.clock.txt=\"%H:%M\"", &timeinfo);
  putcmd(timeStringBuff);
  putcmdf("player.secText.txt=\"%02d\"", timeinfo.tm_sec);
  snprintf(timeStringBuff, sizeof(timeStringBuff), "player.dateText.txt=\"%s, %d %s %d\"", ndow[timeinfo.tm_wday], timeinfo.tm_mday, nmnths[timeinfo.tm_mon], timeinfo.tm_year+1900);
  putcmd(utf8Rus(timeStringBuff, false));
}

void Nextion::localTime(struct tm timeinfo){
  char timeStringBuff[40] = { 0 };
  strftime(timeStringBuff, sizeof(timeStringBuff), "localTime.txt=\"%H:%M:%S\"", &timeinfo);
  putcmd(timeStringBuff);
}

void Nextion::drawPlaylist(uint16_t currentPlItem){
  char plMenu[7][40];
  for (byte i = 0; i < 7; i++) {
    plMenu[i][0] = '\0';
  }
  config.fillPlMenu(plMenu, currentPlItem - 3, 7);
  char cmd[60]={0};
  for (byte i = 0; i < 7; i++) {
    snprintf(cmd, sizeof(cmd) - 1, "t%d.txt=\"%s\"", i, nextion.utf8Rus(plMenu[i], true));
    putcmd(cmd);
  }
}

void Nextion::drawNextStationNum(uint16_t num) {//dialog
  char plMenu[1][40];
  char currentItemText[40] = {0};
  config.fillPlMenu(plMenu, num, 1, true);
  strlcpy(currentItemText, plMenu[0], 39);
  //meta.setText(dsp.utf8Rus(currentItemText, true));
  putcmd("dialog.title.txt", utf8Rus(currentItemText, true));
  putcmd("dialog.text.txt", num, true);
  //dsp.drawNextStationNum(num);
}

void Nextion::swichMode(displayMode_e newmode){
  if (newmode == VOL) {
    _volDelay = millis();
  }
  if (newmode == mode) return;
  mode = newmode;
#ifdef DUMMYDISPLAY
  display.mode = newmode;
#endif
/*  if (newmode != STATIONS) {
    ip();
    volume();
  }*/
  if (newmode == PLAYER) {
    putcmd("page player");
    putcmd("dialog.title.txt", "");
    putcmd("dialog.text.txt", "");
  }
  if (newmode == VOL) {
    putcmd("dialog.title.txt", "VOLUME");
    putcmd("page dialog");
    putcmd("icon.pic", 65);
  }
  if (newmode == LOST) {
    putcmd("page lost");
  }
  if (newmode == UPDATING) {
    putcmd("page updating");
  }
  if (newmode == NUMBERS) {
    putcmd("page dialog");
    putcmd("icon.pic", 63);
  }
  if (newmode == STATIONS) {
    putcmd("page playlist");
#ifdef DUMMYDISPLAY
    display.currentPlItem = config.store.lastStation;
#endif
    drawPlaylist(config.store.lastStation);
  }
}

bool Nextion::getForecast(){
  WiFiClient client;
  const char* host  = "api.openweathermap.org";
  if (!client.connect(host, 80)) {
    Serial.println("## OPENWEATHERMAP ###: connection  failed");
    return false;
  }
  char httpget[250] = {0};
  sprintf(httpget, "GET /data/2.5/weather?lat=%s&lon=%s&units=metric&lang=ru&appid=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", NEXTION_WEATHER_LAT, NEXTION_WEATHER_LON, NEXTION_WEATHER_KEY, host);
  client.print(httpget);
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 2000UL) {
      Serial.println("## OPENWEATHERMAP ###: client available timeout !");
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
        Serial.println("## OPENWEATHERMAP ###: client read timeout !");
        return false;
      }
    }
  }
  if (strstr(line.c_str(), "\"temp\"") == NULL) {
    Serial.println("## OPENWEATHERMAP ###: weather not found !");
    return false;
  }
  char *tmpe;
  char *tmps;
  const char* cursor = line.c_str();
  char desc[120], temp[20], hum[20], press[20], icon[5];

  tmps = strstr(cursor, "\"description\":\"");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: description not found !"); return false;}
  tmps += 15;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: description not found !"); return false;}
  strlcpy(desc, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  
  // "ясно","icon":"01d"}],
  tmps = strstr(cursor, "\"icon\":\"");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: icon not found !"); return false;}
  tmps += 8;
  tmpe = strstr(tmps, "\"}");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: icon not found !"); return false;}
  strlcpy(icon, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  
  tmps = strstr(cursor, "\"temp\":");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: temp not found !"); return false;}
  tmps += 7;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: temp not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  float tempf = atof(temp);
  tmps = strstr(cursor, "\"pressure\":");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: pressure not found !"); return false;}
  tmps += 11;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: pressure not found !"); return false;}
  strlcpy(press, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  int pressi = (float)atoi(press) / 1.333;

  tmps = strstr(cursor, "humidity\":");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: humidity not found !"); return false;}
  tmps += 10;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: humidity not found !"); return false;}
  strlcpy(hum, tmps, tmpe - tmps + 1);

  if(config.store.audioinfo) Serial.printf("## OPENWEATHERMAP ###: description: %s, temp:%.1f C, pressure:%dmmHg, humidity:%s%%\n", desc, tempf, pressi, hum);

  putcmdf("press_txt.txt=\"%dmm\"", pressi);
  putcmdf("hum_txt.txt=\"%d%%\"", atoi(hum));
  char cmd[30];
  snprintf(cmd, sizeof(cmd)-1,"temp_txt.txt=\"%.1f\"", tempf);
  putcmd(cmd);
  int iconofset;
  if(strstr(icon,"01")!=NULL){
    iconofset = 0;
  }else if(strstr(icon,"02")!=NULL){
    iconofset = 1;
  }else if(strstr(icon,"03")!=NULL){
    iconofset = 2;
  }else if(strstr(icon,"04")!=NULL){
    iconofset = 3;
  }else if(strstr(icon,"09")!=NULL){
    iconofset = 4;
  }else if(strstr(icon,"10")!=NULL){
    iconofset = 5;
  }else if(strstr(icon,"11")!=NULL){
    iconofset = 6;
  }else if(strstr(icon,"13")!=NULL){
    iconofset = 7;
  }else if(strstr(icon,"50")!=NULL){
    iconofset = 8;
  }else{
    iconofset = 9;
  }
  putcmd("cond_img.pic", 50+iconofset);
  weatherVisible(1);
  return true;
}

void Nextion::getWeather(void * pvParameters){
  delay(200);
  if (nextion.getForecast()) {
//    nextion.weatherRequest = true;
    weatherticker.detach();
    weatherticker.attach(WEATHER_REQUEST_INTERVAL, nextion.updateWeather);
  } else {
    weatherticker.detach();
    weatherticker.attach(WEATHER_REQUEST_INTERVAL_FAULTY, nextion.updateWeather);
  }
  vTaskDelete( NULL );
}

void Nextion::updateWeather() {
  xTaskCreatePinnedToCore(
    nextion.getWeather,               /* Task function. */
    "nextiongetWeather",              /* name of task. */
    1024 * 4,                         /* Stack size of task */
    NULL,                             /* parameter of the task */
    0,                                /* priority of the task */
    &nextion.weatherUpdateTaskHandle, /* Task handle to keep track of created task */
    0);                               /* pin task to core CORE_FOR_LOOP_CONTROLS */
}

void Nextion::startWeather(){
  if(strlen(NEXTION_WEATHER_KEY)==0) {
    Serial.println("## OPENWEATHERMAP ###: ERROR: NEXTION_WEATHER_KEY not configured");
    return;
  }
  updateWeather();                            /* pin task to core CORE_FOR_LOOP_CONTROLS */
}

/*
  По мотивам https://forum.amperka.ru/threads/%D0%94%D0%B8%D1%81%D0%BF%D0%BB%D0%B5%D0%B9-nextion-%D0%B0%D0%B7%D1%8B-arduino-esp8266.9204/page-18#post-173442
*/
char* Nextion::utf8Rus(char* str, bool uppercase) {
  int index = 0;
  static char out[BUFLEN];
  bool E = false;
  memset(out, 0, sizeof(out));
  if (uppercase) {
    bool next = false;
    for (char *iter = str; *iter != '\0'; ++iter)
    {
      if (E) {
        E = false;
        continue;
      }
      byte rus = (byte) * iter;
      if (rus == 208 && (byte) * (iter + 1) == 129) {
        *iter = (char)209;
        *(iter + 1) = (char)145;
        E = true;
        continue;
      }
      if (rus == 209 && (byte) * (iter + 1) == 145) {
        *iter = (char)209;
        *(iter + 1) = (char)145;
        E = true;
        continue;
      }
      if (next) {
        if (rus >= 128 && rus <= 143) *iter = (char)(rus + 32);
        if (rus >= 176 && rus <= 191) *iter = (char)(rus - 32);
        next = false;
      }
      if (rus == 208) next = true;
      if (rus == 209) {
        *iter = (char)208;
        next = true;
      }
      *iter = toupper(*iter);
    }
  }
  uint32_t codepoint = 0;
  while (str[index])
  {
    uint8_t ch = (uint8_t) (str[index]);
    if (ch <= 0x7f)
      codepoint = ch;
    else if (ch <= 0xbf)
      codepoint = (codepoint << 6) | (ch & 0x3f);
    else if (ch <= 0xdf)
      codepoint = ch & 0x1f;
    else if (ch <= 0xef)
      codepoint = ch & 0x0f;
    else
      codepoint = ch & 0x07;
    ++index;
    if (((str[index] & 0xc0) != 0x80) && (codepoint <= 0x10ffff))
    {
      if (codepoint <= 255)
      {
        out[strlen(out)]=(uint8_t)codepoint;
      }
      else
      {
        if(codepoint > 0x400){
          out[strlen(out)]=(uint8_t)(codepoint - 0x360);
        }
      }
    }
  }
  out[strlen(out)+1]=0;
  return out;
}

#endif //NEXTION_RX!=255 && NEXTION_TX!=255
