#include "config.h"
#include <EEPROM.h>
#include <SPIFFS.h>
#include "display.h"
#include "player.h"

Config config;

#if DSP_HSPI || TS_HSPI || VS_HSPI
SPIClass  SPI2(HSPI);
#endif

void u8fix(char *src){
  char last = src[strlen(src)-1]; 
  if ((uint8_t)last >= 0xC2) src[strlen(src)-1]='\0';
}

void Config::init() {
  EEPROM.begin(EEPROM_SIZE);
#if IR_PIN!=255
    irindex=-1;
#endif
  eepromRead(EEPROM_START, store);
  if (store.config_set != 4262) setDefaults();
  //if (!SPIFFS.begin(false, "/spiffs", 30)) {
  if (!SPIFFS.begin(false)) {
    return;
  }
  loadTheme();
  ssidsCount = 0;
  initPlaylist();
  if (store.lastStation == 0 && store.countStation > 0) {
    store.lastStation = 1;
    save();
  }
  loadStation(store.lastStation);
#if IR_PIN!=255
  eepromRead(EEPROM_START_IR, ircodes);
  if(ircodes.ir_set!=4224){
    ircodes.ir_set=4224;
    memset(ircodes.irVals, 0, sizeof(ircodes.irVals));
  }
#endif
#if BRIGHTNESS_PIN!=255
  pinMode(BRIGHTNESS_PIN, OUTPUT);
  setBrightness(false);
#endif
}

uint16_t Config::color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void Config::loadTheme(){
  theme.background    = color565(COLOR_BACKGROUND);
  theme.meta          = color565(COLOR_STATION_NAME);
  theme.metabg        = color565(COLOR_STATION_BG);
  theme.metafill      = color565(COLOR_STATION_FILL);
  theme.title1        = color565(COLOR_SNG_TITLE_1);
  theme.title2        = color565(COLOR_SNG_TITLE_2);
  theme.digit         = color565(COLOR_DIGITS);
  theme.div           = color565(COLOR_DIVIDER);
  theme.weather       = color565(COLOR_WEATHER);
  theme.vumax         = color565(COLOR_VU_MAX);
  theme.vumin         = color565(COLOR_VU_MIN);
  theme.clock         = color565(COLOR_CLOCK);
  theme.seconds       = color565(COLOR_SECONDS);
  theme.dow           = color565(COLOR_DAY_OF_W);
  theme.date          = color565(COLOR_DATE);
  theme.heap          = color565(COLOR_HEAP);
  theme.buffer        = color565(COLOR_BUFFER);
  theme.ip            = color565(COLOR_IP);
  theme.vol           = color565(COLOR_VOLUME_VALUE);
  theme.rssi          = color565(COLOR_RSSI);
  theme.bitrate       = color565(COLOR_BITRATE);
  theme.volbarout     = color565(COLOR_VOLBAR_OUT);
  theme.volbarin      = color565(COLOR_VOLBAR_IN);
  theme.plcurrent     = color565(COLOR_PL_CURRENT);
  theme.plcurrentbg   = color565(COLOR_PL_CURRENT_BG);
  theme.plcurrentfill = color565(COLOR_PL_CURRENT_FILL);
  theme.playlist[0]   = color565(COLOR_PLAYLIST_0);
  theme.playlist[1]   = color565(COLOR_PLAYLIST_1);
  theme.playlist[2]   = color565(COLOR_PLAYLIST_2);
  theme.playlist[3]   = color565(COLOR_PLAYLIST_3);
  theme.playlist[4]   = color565(COLOR_PLAYLIST_4);
}

template <class T> int Config::eepromWrite(int ee, const T& value) {
  const byte* p = (const byte*)(const void*)&value;
  int i;
  for (i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
  EEPROM.commit();
  return i;
}

template <class T> int Config::eepromRead(int ee, T& value) {
  byte* p = (byte*)(void*)&value;
  int i;;
  for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
  return i;
}

void Config::setDefaults() {
  store.config_set = 4262;
  store.volume = 12;
  store.balance = 0;
  store.trebble = 0;
  store.middle = 0;
  store.bass = 0;
  store.lastStation = 0;
  store.countStation = 0;
  store.lastSSID = 0;
  store.audioinfo = false;
  store.smartstart = 2;
  store.tzHour = 3;
  store.tzMin = 0;
  store.timezoneOffset = 0;

  store.vumeter=false;
  store.softapdelay=0;
  store.flipscreen=false;
  store.invertdisplay=false;
  store.numplaylist=false;
  store.fliptouch=false;
  store.dbgtouch=false;
  store.dspon=true;
  store.brightness=100;
  store.contrast=55;
  strlcpy(store.sntp1,"pool.ntp.org", 35);
  strlcpy(store.sntp2,"1.ru.pool.ntp.org", 35);
  store.showweather=false;
  strlcpy(store.weatherlat,"55.7512", 10);
  strlcpy(store.weatherlon,"37.6184", 10);
  strlcpy(store.weatherkey,"", 64);
  store.volsteps = 1;
  store.encacc = 200;
  store.irto = 80;
  store.irtlp = 35;
  store.btnpullup = true;
  store.btnlongpress = 200;
  store.btnclickticks = 300;
  store.btnpressticks = 500;
  store.encpullup = false;
  store.enchalf = false;
  store.enc2pullup = false;
  store.enc2half = false;
  store.forcemono = false;
  store.i2sinternal = false;
  store.rotate90 = false;
}

void Config::setTimezone(int8_t tzh, int8_t tzm) {
  store.tzHour = tzh;
  store.tzMin = tzm;
  save();
}

void Config::setTimezoneOffset(uint16_t tzo) {
  store.timezoneOffset = tzo;
  save();
}

uint16_t Config::getTimezoneOffset() {
  return 0; // TODO
}

void Config::save() {
  eepromWrite(EEPROM_START, store);
}

#if IR_PIN!=255
void Config::saveIR(){
  eepromWrite(EEPROM_START_IR, ircodes);
}
#endif

void Config::saveVolume(){
  EEPROM.write(EEPROM_START + sizeof(store.config_set), store.volume);
  EEPROM.commit();
}

byte Config::setVolume(byte val) {
  store.volume = val;
  return store.volume;
}

void Config::setTone(int8_t bass, int8_t middle, int8_t trebble) {
  store.bass = bass;
  store.middle = middle;
  store.trebble = trebble;
  save();
}

void Config::setSmartStart(byte ss) {
  if (store.smartstart < 2) {
    store.smartstart = ss;
    save();
  }
}

void Config::setBalance(int8_t balance) {
  store.balance = balance;
  save();
}

byte Config::setLastStation(byte val) {
  store.lastStation = val;
  save();
  return store.lastStation;
}

byte Config::setCountStation(byte val) {
  store.countStation = val;
  save();
  return store.countStation;
}

byte Config::setLastSSID(byte val) {
  store.lastSSID = val;
  save();
  return store.lastSSID;
}

void Config::setTitle(const char* title) {
  memset(config.station.title, 0, BUFLEN);
  strlcpy(config.station.title, title, BUFLEN);
  u8fix(config.station.title);
  display.putRequest(NEWTITLE);
}

void Config::setStation(const char* station) {
  memset(config.station.name, 0, BUFLEN);
  strlcpy(config.station.name, station, BUFLEN);
  u8fix(config.station.title);
}

void Config::indexPlaylist() {
  File playlist = SPIFFS.open(PLAYLIST_PATH, "r");
  if (!playlist) {
    return;
  }
  char sName[BUFLEN], sUrl[BUFLEN];
  int sOvol;
  File index = SPIFFS.open(INDEX_PATH, "w");
  while (playlist.available()) {
    uint32_t pos = playlist.position();
    if (parseCSV(playlist.readStringUntil('\n').c_str(), sName, sUrl, sOvol)) {
      index.write((byte *) &pos, 4);
    }
  }
  index.close();
  playlist.close();
}

void Config::initPlaylist() {
  store.countStation = 0;
  if (!SPIFFS.exists(INDEX_PATH)) indexPlaylist();

  if (SPIFFS.exists(INDEX_PATH)) {
    File index = SPIFFS.open(INDEX_PATH, "r");
    store.countStation = index.size() / 4;
    index.close();
    save();
  }
}

void Config::loadStation(uint16_t ls) {
  char sName[BUFLEN], sUrl[BUFLEN];
  int sOvol;
  if (store.countStation == 0) {
    memset(station.url, 0, BUFLEN);
    memset(station.name, 0, BUFLEN);
    strncpy(station.name, "ёRadio", BUFLEN);
    station.ovol = 0;
    return;
  }
  if (ls > store.countStation) {
    ls = 1;
  }
  File playlist = SPIFFS.open(PLAYLIST_PATH, "r");

  File index = SPIFFS.open(INDEX_PATH, "r");
  index.seek((ls - 1) * 4, SeekSet);
  uint32_t pos;

  index.readBytes((char *) &pos, 4);

  index.close();
  playlist.seek(pos, SeekSet);
  if (parseCSV(playlist.readStringUntil('\n').c_str(), sName, sUrl, sOvol)) {
    memset(station.url, 0, BUFLEN);
    memset(station.name, 0, BUFLEN);
    strncpy(station.name, sName, BUFLEN);
    strncpy(station.url, sUrl, BUFLEN);
    station.ovol = sOvol;
    setLastStation(ls);
  }
  playlist.close();
}

void Config::fillPlMenu(char plmenu[][40], int from, byte count, bool removeNum) {
  int ls = from;
  byte c = 0;
  bool finded = false;
  char sName[BUFLEN], sUrl[BUFLEN];
  int sOvol;
  if (store.countStation == 0) {
    return;
  }
  File playlist = SPIFFS.open(PLAYLIST_PATH, "r");
  File index = SPIFFS.open(INDEX_PATH, "r");
  while (true) {
    if (ls < 1) {
      ls++;
      c++;
      continue;
    }
    if (!finded) {
      index.seek((ls - 1) * 4, SeekSet);
      uint32_t pos;
      index.readBytes((char *) &pos, 4);
      finded = true;
      index.close();
      playlist.seek(pos, SeekSet);
    }
    while (playlist.available()) {
      if (parseCSV(playlist.readStringUntil('\n').c_str(), sName, sUrl, sOvol)) {
        if(config.store.numplaylist){
          if(removeNum){
            strlcpy(plmenu[c], sName, 39);
          }else{
            char buf[BUFLEN+10];
            sprintf(buf, "%d %s", (int)(from+c), sName);
            strlcpy(plmenu[c], buf, 39);
          }
        }else{
          strlcpy(plmenu[c], sName, 39);
        }
        c++;
      }
      if (c >= count) break;
    }
    break;
  }
  playlist.close();
}

bool Config::parseCSV(const char* line, char* name, char* url, int &ovol) {
  char *tmpe;
  const char* cursor = line;
  char buf[5];
  tmpe = strstr(cursor, "\t");
  if (tmpe == NULL) return false;
  strlcpy(name, cursor, tmpe - cursor + 1);
  if (strlen(name) == 0) return false;
  cursor = tmpe + 1;
  tmpe = strstr(cursor, "\t");
  if (tmpe == NULL) return false;
  strlcpy(url, cursor, tmpe - cursor + 1);
  if (strlen(url) == 0) return false;
  cursor = tmpe + 1;
  if (strlen(cursor) == 0) return false;
  strlcpy(buf, cursor, 4);
  ovol = atoi(buf);
  return true;
}

bool Config::parseJSON(const char* line, char* name, char* url, int &ovol) {
  char* tmps, *tmpe;
  const char* cursor = line;
  char port[8], host[246], file[254];
  tmps = strstr(cursor, "\":\"");
  if (tmps == NULL) return false;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) return false;
  strlcpy(name, tmps + 3, tmpe - tmps - 3 + 1);
  if (strlen(name) == 0) return false;
  cursor = tmpe + 3;
  tmps = strstr(cursor, "\":\"");
  if (tmps == NULL) return false;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) return false;
  strlcpy(host, tmps + 3, tmpe - tmps - 3 + 1);
  if (strlen(host) == 0) return false;
  if (strstr(host, "http://") == NULL && strstr(host, "https://") == NULL) {
    sprintf(file, "http://%s", host);
    strlcpy(host, file, strlen(file) + 1);
  }
  cursor = tmpe + 3;
  tmps = strstr(cursor, "\":\"");
  if (tmps == NULL) return false;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) return false;
  strlcpy(file, tmps + 3, tmpe - tmps - 3 + 1);
  cursor = tmpe + 3;
  tmps = strstr(cursor, "\":\"");
  if (tmps == NULL) return false;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) return false;
  strlcpy(port, tmps + 3, tmpe - tmps - 3 + 1);
  int p = atoi(port);
  if (p > 0) {
    sprintf(url, "%s:%d%s", host, p, file);
  } else {
    sprintf(url, "%s%s", host, file);
  }
  cursor = tmpe + 3;
  tmps = strstr(cursor, "\":\"");
  if (tmps == NULL) return false;
  tmpe = strstr(tmps, "\"}");
  if (tmpe == NULL) return false;
  strlcpy(port, tmps + 3, tmpe - tmps - 3 + 1);
  ovol = atoi(port);
  return true;
}

bool Config::parseWsCommand(const char* line, char* cmd, char* val, byte cSize) {
  char *tmpe;
  tmpe = strstr(line, "=");
  if (tmpe == NULL) return false;
  memset(cmd, 0, cSize);
  strlcpy(cmd, line, tmpe - line + 1);
  if (strlen(tmpe + 1) == 0) return false;
  memset(val, 0, cSize);
  strlcpy(val, tmpe + 1, strlen(line) - strlen(cmd) + 1);
  return true;
}

bool Config::parseSsid(const char* line, char* ssid, char* pass) {
  char *tmpe;
  tmpe = strstr(line, "\t");
  if (tmpe == NULL) return false;
  uint16_t pos = tmpe - line;
  if (pos > 19 || strlen(line) > 61) return false;
  memset(ssid, 0, 20);
  strlcpy(ssid, line, pos + 1);
  memset(pass, 0, 40);
  strlcpy(pass, line + pos + 1, strlen(line) - pos);
  return true;
}

bool Config::saveWifiFromNextion(const char* post){
  File file = SPIFFS.open(SSIDS_PATH, "w");
  if (!file) {
    return false;
  } else {
    file.print(post);
    file.close();
    ESP.restart();
    return true;
  }
}

bool Config::saveWifi() {
  if (!SPIFFS.exists(TMP_PATH)) return false;
  SPIFFS.remove(SSIDS_PATH);
  SPIFFS.rename(TMP_PATH, SSIDS_PATH);
  ESP.restart();
  return true;
}

bool Config::initNetwork() {
  File file = SPIFFS.open(SSIDS_PATH, "r");
  if (!file || file.isDirectory()) {
    return false;
  }
  char ssidval[20], passval[40];
  byte c = 0;
  while (file.available()) {
    if (parseSsid(file.readStringUntil('\n').c_str(), ssidval, passval)) {
      strlcpy(ssids[c].ssid, ssidval, 20);
      strlcpy(ssids[c].password, passval, 40);
      ssidsCount++;
      c++;
    }
  }
  file.close();
  return true;
}

void Config::setBrightness(bool dosave){
#if BRIGHTNESS_PIN!=255
  if(!store.dspon && dosave) {
    display.wakeup();
  }
  //analogWrite(BRIGHTNESS_PIN, config.store.dspon?map(store.brightness, 0, 100, 0, 255):0);
  analogWrite(BRIGHTNESS_PIN, map(store.brightness, 0, 100, 0, 255));
  if(!store.dspon) store.dspon = true;
  if(dosave) save();
#endif
#ifdef USE_NEXTION
//  if(!store.dspon && dosave) {
    nextion.wake();
//  }
  char cmd[15];
  snprintf(cmd, 15, "dims=%d", store.brightness);
  nextion.putcmd(cmd);
  if(!store.dspon) store.dspon = true;
  if(dosave) save();
#endif
}

void Config::setDspOn(bool dspon){
  store.dspon = dspon;
  save();
#ifdef USE_NEXTION
  if(!dspon) nextion.sleep();
  else nextion.wake();
#endif
  if(!dspon){
#if BRIGHTNESS_PIN!=255
  analogWrite(BRIGHTNESS_PIN, 0);
#endif
    display.deepsleep();
  }else{
    display.wakeup();
#if BRIGHTNESS_PIN!=255
  analogWrite(BRIGHTNESS_PIN, map(store.brightness, 0, 100, 0, 255));
#endif
  }
}

void Config::doSleep(){
  if(BRIGHTNESS_PIN!=255) analogWrite(BRIGHTNESS_PIN, 0);
  display.deepsleep();
#ifdef USE_NEXTION
  nextion.sleep();
#endif
  if(WAKE_PIN!=255) esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, LOW);
  esp_sleep_enable_timer_wakeup(config.sleepfor * 60 * 1000000ULL);
  esp_deep_sleep_start();
}

void Config::sleepForAfter(uint16_t sf, uint16_t sa){
  sleepfor = sf;
  if(sa > 0) _sleepTimer.attach(sa * 60, doSleep);
  else doSleep();
}
