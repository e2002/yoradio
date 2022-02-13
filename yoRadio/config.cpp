#include "config.h"
#include <EEPROM.h>
#include <SPIFFS.h>

Config config;

void Config::init() {
  eepromRead(EEPROM_START, store);
  if (store.tz_set!=57){ // update to v0.4.200
    store.tz_set = 57;
    store.tzHour = 3;
    store.tzMin = 0;
    store.timezoneOffset = 0;
  }
  if (store.config_set != 4256) setDefaults();
  //if (!SPIFFS.begin(false, "/spiffs", 30)) {
  if (!SPIFFS.begin(false)) {
    return;
  }
  ssidsCount = 0;
  initPlaylist();
  if (store.lastStation == 0 && store.countStation > 0) {
    store.lastStation = 1;
    save();
  }
  loadStation(store.lastStation);
}

template <class T> int Config::eepromWrite(int ee, const T& value) {
  const byte* p = (const byte*)(const void*)&value;
  int i;
  EEPROM.begin(EEPROM_SIZE);
  for (i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
  EEPROM.commit();
  delay(20);
  EEPROM.end();
  return i;
}

template <class T> int Config::eepromRead(int ee, T& value) {
  byte* p = (byte*)(void*)&value;
  int i;
  EEPROM.begin(EEPROM_SIZE);
  for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
  EEPROM.end();
  return i;
}

void Config::setDefaults() {
  store.config_set = 4256;
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
  store.tz_set = 57;
  store.tzHour = 3;
  store.tzMin = 0;
  store.timezoneOffset = 0;
}

void Config::setTimezone(int8_t tzh, int8_t tzm) {
  store.tzHour=tzh;
  store.tzMin=tzm;
  save();
}

void Config::setTimezoneOffset(uint16_t tzo) {
  store.timezoneOffset=tzo;
  save();
}

uint16_t Config::getTimezoneOffset() {
  return 0; // TODO
}

void Config::save() {
  eepromWrite(EEPROM_START, store);
}

byte Config::setVolume(byte val, bool dosave) {
  store.volume = val;
  if (dosave) save();
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

void Config::fillPlMenu(char plmenu[][40], int from, byte count) {
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
        strlcpy(plmenu[c], sName, 39);
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
  tmpe=strstr(cursor, "\t");
  if(tmpe==NULL) return false;
  strlcpy(name, cursor, tmpe-cursor+1);
  if(strlen(name)==0) return false;
  cursor=tmpe+1;
  tmpe=strstr(cursor, "\t");
  if(tmpe==NULL) return false;
  strlcpy(url, cursor, tmpe-cursor+1);
  if(strlen(url)==0) return false;
  cursor=tmpe+1;
  if (strlen(cursor) == 0) return false;
  strlcpy(buf, cursor, 4);
  ovol = atoi(buf);
  return true;
}

bool Config::parseJSON(const char* line, char* name, char* url, int &ovol) {
  char* tmps, *tmpe;
  const char* cursor = line;
  char port[8], host[254], file[254];
  tmps=strstr(cursor, "\":\"");
  if(tmps==NULL) return false;
  tmpe=strstr(tmps, "\",\"");
  if(tmpe==NULL) return false;
  strlcpy(name, tmps+3, tmpe-tmps-3+1);
  if(strlen(name)==0) return false;
  cursor=tmpe+3;
  tmps=strstr(cursor, "\":\"");
  if(tmps==NULL) return false;
  tmpe=strstr(tmps, "\",\"");
  if(tmpe==NULL) return false;
  strlcpy(host, tmps+3, tmpe-tmps-3+1);
  if(strlen(host)==0) return false;
  if(strstr(host,"http://")==NULL && strstr(host,"https://")==NULL) {
    sprintf(file, "http://%s", host);
    strlcpy(host, file, strlen(file)+1);
  }
  cursor=tmpe+3;
  tmps=strstr(cursor, "\":\"");
  if(tmps==NULL) return false;
  tmpe=strstr(tmps, "\",\"");
  if(tmpe==NULL) return false;
  strlcpy(file, tmps+3, tmpe-tmps-3+1);
  cursor=tmpe+3;
  tmps=strstr(cursor, "\":\"");
  if(tmps==NULL) return false;
  tmpe=strstr(tmps, "\",\"");
  if(tmpe==NULL) return false;
  strlcpy(port, tmps+3, tmpe-tmps-3+1);
  int p = atoi(port);
  if(p>0){
    sprintf(url, "%s:%d%s", host, p, file);
  }else{
    sprintf(url, "%s%s", host, file);
  }
  cursor=tmpe+3;
  tmps=strstr(cursor, "\":\"");
  if(tmps==NULL) return false;
  tmpe=strstr(tmps, "\"}");
  if(tmpe==NULL) return false;
  strlcpy(port, tmps+3, tmpe-tmps-3+1);
  ovol = atoi(port);
  return true;
}

bool Config::parseWsCommand(const char* line, char* cmd, char* val, byte cSize) {
  char *tmpe;
  tmpe=strstr(line, "=");
  if(tmpe==NULL) return false;
  memset(cmd, 0, cSize);
  strlcpy(cmd, line, tmpe-line+1);
  if (strlen(tmpe+1) == 0) return false;
  memset(val, 0, cSize);
  strlcpy(val, tmpe+1, tmpe+1-line+1);
  return true;
}

bool Config::parseSsid(const char* line, char* ssid, char* pass) {
  char *tmpe;
  tmpe=strstr(line, "\t");
  if(tmpe==NULL) return false;
  uint16_t pos= tmpe-line;
  if (pos > 19 || strlen(line) > 61) return false;
  memset(ssid, 0, 20);
  strlcpy(ssid, line, pos + 1);
  memset(pass, 0, 40);
  strlcpy(pass, line + pos + 1, strlen(line) - pos);
  return true;
}

bool Config::saveWifi(const char* post) {
  File file = SPIFFS.open(SSIDS_PATH, "w");
  if (!file) {
    return false;
  } else {
    file.print(post);
    file.close();
    ESP.restart();
  }
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
}
