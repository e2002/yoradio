#include "../core/options.h"

#if NEXTION_RX!=255 && NEXTION_TX!=255
#include "nextion.h"
#include "../core/config.h"

#include "../core/player.h"
#include "../core/controls.h"
#include "../core/netserver.h"
#include "../core/network.h"

#ifndef CORE_STACK_SIZE
  #define CORE_STACK_SIZE  1024*3
#endif

HardwareSerial hSerial(1); // use UART1

Nextion::Nextion() {

}

void nextionCore0( void * pvParameters ){
  delay(500);
  while(true){
    nextion.loop();
    vTaskDelay(5);
  }
  vTaskDelete( NULL );
}

void Nextion::begin(bool dummy) {
  _dummyDisplay=dummy;
  mode=LOST;
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
  delay(300);
  putcmd("");
  putcmd("bkcmd=0");
//  putcmd("page boot");
  
  _displayQueue = xQueueCreate( 10, sizeof( requestParams_t ) );
  if(dummy) {
    xTaskCreatePinnedToCore(nextionCore0, "TaskCore0", CORE_STACK_SIZE, NULL, 4, &_TaskCore0, !xPortGetCoreID());
  }
}

void Nextion::start(){
	Serial.print("##[BOOT]#\tNextion.start\t");
	delay(100);
  if (network.status != CONNECTED) {
    apScreen();
    return;
  }
#ifdef DUMMYDISPLAY
  display.mode(PLAYER);
  config.setTitle(const_PlReady);
#endif
  putRequest({NEWMODE, PLAYER});
  putRequest({NEWSTATION, 0});
  putRequest({NEWTITLE, 0});
  putRequest({DRAWVOL, 0});
  Serial.println("done");
}

void Nextion::apScreen() {
  putcmd("apscreenlock=1");
  putcmd("page settings_wifi");
}

void Nextion::putRequest(requestParams_t request){
  if(_displayQueue==NULL) return;
  xQueueSend(_displayQueue, &request, portMAX_DELAY);
}

#ifndef NEXTION_QUEUE_TICKS
  #define NEXTION_QUEUE_TICKS 8
#endif

void Nextion::processQueue(){
  if(_displayQueue==NULL) return;
  requestParams_t request;
  if(xQueueReceive(_displayQueue, &request, NEXTION_QUEUE_TICKS)){
    switch (request.type){
      case NEWMODE: swichMode((displayMode_e)request.payload); break;
      case CLOCK: printClock(network.timeinfo); break;
      case DSPRSSI: rssi(); break;
      case NEWTITLE: newTitle(config.station.title); break;
      case BOOTSTRING: {
        char buf[50];
        snprintf(buf, 50, bootstrFmt, config.ssids[request.payload].ssid);
        bootString(buf);
        break;
      }
      case NEWSTATION: {
        newNameset(config.station.name);
        bitrate(config.station.bitrate);
        bitratePic(ICON_NA);
        break;
      }
      case SHOWWEATHER:   weatherVisible(strlen(config.store.weatherkey)>0 && config.store.showweather); break;
      case NEXTSTATION:   drawNextStationNum(request.payload); break;
      case DRAWPLAYLIST:  drawPlaylist(request.payload); break;
      case DRAWVOL: {
        if(!_volInside){
          setVol(config.store.volume, mode == VOL);
        }
        _volInside=false;
        break;
      }
      default: break;
    }
  }
#ifdef DUMMYDISPLAY
  if(mode==VOL || mode==STATIONS || mode==NUMBERS ){
    if (millis() - _volDelay > (mode==VOL?3000:30000)) {
      _volDelay = millis();
      swichMode(PLAYER);
    }
  }
#endif
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
          if(strcmp(scanBuf, "player") == 0) display.putRequest(NEWMODE, PLAYER);
          if(strcmp(scanBuf, "playlist") == 0) display.putRequest(NEWMODE, STATIONS);
          if(strcmp(scanBuf, "info") == 0) {
            putcmd("yoversion.txt", YOVERSION);
            putcmd("espcore.txt", _espcoreversion);
            putcmd("ipaddr.txt", WiFi.localIP().toString().c_str());
            putcmd("ssid.txt", WiFi.SSID().c_str());
            display.putRequest(NEWMODE, INFO);
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
            display.putRequest(NEWMODE, SETTINGS);
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
              display.putRequest(NEWMODE, WIFI);
            }
          }
          if(strcmp(scanBuf, "time") == 0) {
            putcmdf("tzHourText.txt=\"%02d\"", config.store.tzHour);
            putcmd("tzHour.val", config.store.tzHour);
            putcmdf("tzMinText.txt=\"%02d\"", config.store.tzMin);
            putcmd("tzMin.val", config.store.tzMin);
            display.putRequest(NEWMODE, TIMEZONE);
          }
          if(strcmp(scanBuf, "sys") == 0) {
            putcmd("smartstart.val", config.store.smartstart==2?0:1);
            putcmd("audioinfo.val", config.store.audioinfo);
            display.putRequest(NEWMODE, SETTINGS);
          }
        }
        if (sscanf(rxbuf, "ctrls=%s", scanBuf) == 1){
          if(strcmp(scanBuf, "up") == 0) {
            display.resetQueue();
            int p = display.currentPlItem - 1;
            if (p < 1) p = config.store.countStation;
            display.currentPlItem = p;
            display.putRequest(DRAWPLAYLIST, p);
          }
          if(strcmp(scanBuf, "dn") == 0) {
            display.resetQueue();
            int p = display.currentPlItem + 1;
            if (p > config.store.countStation) p = 1;
            display.currentPlItem = p;
            display.putRequest(DRAWPLAYLIST, p);
          }
          if(strcmp(scanBuf, "go") == 0) {
            display.putRequest(NEWMODE, PLAYER);
            player.sendCommand({PR_PLAY, display.currentPlItem});
          }
          if(strcmp(scanBuf, "toggle") == 0) {
            player.toggle();
          }
        }
        if (sscanf(rxbuf, "vol=%d", &scanDigit) == 1){
          _volInside = true;
          player.sendCommand({PR_VOL, scanDigit});
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
          if(strlen(config.store.sntp1)>0 && strlen(config.store.sntp2)>0){
            configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), config.store.sntp1, config.store.sntp2);
          }else if(strlen(config.store.sntp1)>0){
            configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), config.store.sntp1);
          }
          network.forceTimeSync = true;
        }
        if (sscanf(rxbuf, "tzmin=%d", &scanDigit) == 1){
          config.setTimezone(config.store.tzHour, (int8_t)scanDigit);
          if(strlen(config.store.sntp1)>0 && strlen(config.store.sntp2)>0){
            configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), config.store.sntp1, config.store.sntp2);
          }else if(strlen(config.store.sntp1)>0){
            configTime(config.store.tzHour * 3600 + config.store.tzMin * 60, config.getTimezoneOffset(), config.store.sntp1);
          }
          network.forceTimeSync = true;
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
          config.saveWifiFromNextion(wifisettings.c_str());
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
    putcmd("player.bitrate.txt=\" \"");
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

void Nextion::audioinfo(const char* info){
  if (strstr(info, "format is aac")  != NULL) bitratePic(ICON_AAC);
  if (strstr(info, "format is flac") != NULL) bitratePic(ICON_FLAC);
  if (strstr(info, "format is mp3")  != NULL) bitratePic(ICON_MP3);
  if (strstr(info, "format is wav")  != NULL) bitratePic(ICON_WAV);
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
  }
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
  char timeStringBuff[100] = { 0 };
  strftime(timeStringBuff, sizeof(timeStringBuff), "player.clock.txt=\"%H:%M\"", &timeinfo);
  putcmd(timeStringBuff);
  putcmdf("player.secText.txt=\"%02d\"", timeinfo.tm_sec);
  snprintf(timeStringBuff, sizeof(timeStringBuff), "player.dateText.txt=\"%s, %d %s %d\"", dowf[timeinfo.tm_wday], timeinfo.tm_mday, mnths[timeinfo.tm_mon], timeinfo.tm_year+1900);
  putcmd(utf8Rus(timeStringBuff, false));
  if(mode==TIMEZONE) localTime(network.timeinfo);
  if(mode==INFO)     rssi();
}

void Nextion::localTime(struct tm timeinfo){
  char timeStringBuff[40] = { 0 };
  strftime(timeStringBuff, sizeof(timeStringBuff), "localTime.txt=\"%H:%M:%S\"", &timeinfo);
  putcmd(timeStringBuff);
}

void Nextion::printPLitem(uint8_t pos, const char* item){
	char cmd[60]={0};
	snprintf(cmd, sizeof(cmd) - 1, "t%d.txt=\"%s\"", pos, nextion.utf8Rus((char*)item, true));
  putcmd(cmd);
}

void Nextion::drawPlaylist(uint16_t currentPlItem){
	mode=STATIONS;
  uint8_t lastPos = config.fillPlMenu(currentPlItem - 3, 7, true);
  if(lastPos<7){
  	for(int i=0;i<7-lastPos;i++){
  		nextion.printPLitem(lastPos+i, "");
  	}
  }
  _volDelay = millis();
}

void Nextion::drawNextStationNum(uint16_t num) {//dialog
  putcmd("dialog.title.txt", utf8Rus(config.stationByNum(num), true));
  putcmd("dialog.text.txt", num, true);
  _volDelay = millis();
}

void Nextion::swichMode(displayMode_e newmode){
  if (newmode == VOL) {
    _volDelay = millis();
  }
  if (newmode == mode) {
  	return;
  }
  mode = newmode;
#ifdef DUMMYDISPLAY
  display.mode(newmode);
#endif
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

void Nextion::sleep(void) { 
  putcmd("sleep=1");
}
void Nextion::wake(void) { 
  putcmd("sleep=0");
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
