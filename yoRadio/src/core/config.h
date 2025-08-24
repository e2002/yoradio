#ifndef config_h
#define config_h
#pragma once
#include "Arduino.h"
#include <SPI.h>
#include <SPIFFS.h>
#include <EEPROM.h>
#include "../displays/widgets/widgetsconfig.h" //BitrateFormat

#define EEPROM_SIZE       768
#define EEPROM_START      500
#define EEPROM_START_IR   0
#define EEPROM_START_2    10
#define PLAYLIST_PATH     "/data/playlist.csv"
#define SSIDS_PATH        "/data/wifi.csv"
#define TMP_PATH          "/data/tmpfile.txt"
#define INDEX_PATH        "/data/index.dat"

#define PLAYLIST_SD_PATH     "/data/playlistsd.csv"
#define INDEX_SD_PATH        "/data/indexsd.dat"

#define REAL_PLAYL   config.getMode()==PM_WEB?PLAYLIST_PATH:PLAYLIST_SD_PATH
#define REAL_INDEX   config.getMode()==PM_WEB?INDEX_PATH:INDEX_SD_PATH

#define MAX_PLAY_MODE   1
#define WEATHERKEY_LENGTH 58
#define MDNS_LENGTH 24

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  #define ESP_ARDUINO_3 1
#endif

#define CONFIG_VERSION  5

enum playMode_e      : uint8_t  { PM_WEB=0, PM_SDCARD=1 };

void u8fix(char *src);

void checkAllTasksStack();

struct theme_t {
  uint16_t background;
  uint16_t meta;
  uint16_t metabg;
  uint16_t metafill;
  uint16_t title1;
  uint16_t title2;
  uint16_t digit;
  uint16_t div;
  uint16_t weather;
  uint16_t vumax;
  uint16_t vumin;
  uint16_t clock;
  uint16_t clockbg;
  uint16_t seconds;
  uint16_t dow;
  uint16_t date;
  uint16_t heap;
  uint16_t buffer;
  uint16_t ip;
  uint16_t vol;
  uint16_t rssi;
  uint16_t bitrate;
  uint16_t volbarout;
  uint16_t volbarin;
  uint16_t plcurrent;
  uint16_t plcurrentbg;
  uint16_t plcurrentfill;
  uint16_t playlist[5];
};
struct config_t
{
  uint16_t  config_set; //must be 4262
  uint16_t  version;
  uint8_t   volume;
  int8_t    balance;
  int8_t    trebble;
  int8_t    middle;
  int8_t    bass;
  uint16_t  lastStation;
  uint16_t  countStation;
  uint8_t   lastSSID;
  bool      audioinfo;
  uint8_t   smartstart;
  int8_t    tzHour;
  int8_t    tzMin;
  uint16_t  timezoneOffset;
  bool      vumeter;
  uint8_t   softapdelay;
  bool      flipscreen;
  bool      invertdisplay;
  bool      numplaylist;
  bool      fliptouch;
  bool      dbgtouch;
  bool      dspon;
  uint8_t   brightness;
  uint8_t   contrast;
  char      sntp1[35];
  char      sntp2[35];
  bool      showweather;
  char      weatherlat[10];
  char      weatherlon[10];
  char      weatherkey[WEATHERKEY_LENGTH];
  uint16_t  _reserved;
  uint16_t  lastSdStation;
  bool      sdsnuffle;
  uint8_t   volsteps;
  uint16_t  encacc;
  uint8_t   play_mode;  //0 WEB, 1 SD
  uint8_t   irtlp;
  bool      btnpullup;
  uint16_t  btnlongpress;
  uint16_t  btnclickticks;
  uint16_t  btnpressticks;
  bool      encpullup;
  bool      enchalf;
  bool      enc2pullup;
  bool      enc2half;
  bool      forcemono;
  bool      i2sinternal;
  bool      rotate90;
  bool      screensaverEnabled;
  uint16_t  screensaverTimeout;
  bool      screensaverBlank;
  bool      screensaverPlayingEnabled;
  uint16_t  screensaverPlayingTimeout;
  bool      screensaverPlayingBlank;
  char      mdnsname[24];
  bool      skipPlaylistUpDown;
  uint16_t  abuff;
  bool      telnet;
  bool      watchdog;
  uint16_t  timeSyncInterval;
  uint16_t  timeSyncIntervalRTC;
  uint16_t  weatherSyncInterval;
};

#if IR_PIN!=255
struct ircodes_t
{
  unsigned int ir_set; //must be 4224
  uint64_t irVals[20][3];
};
#endif

struct station_t
{
  char name[BUFLEN];
  char url[BUFLEN];
  char title[BUFLEN];
  uint16_t bitrate;
  int  ovol;
};

struct neworkItem
{
  char ssid[30];
  char password[40];
};

class Config {
  public:
    config_t store;
    station_t station;
    theme_t   theme;
#if IR_PIN!=255
    int irindex;
    uint8_t irchck;
    ircodes_t ircodes;
#endif
    BitrateFormat configFmt = BF_UNKNOWN;
    neworkItem ssids[5];
    uint8_t ssidsCount;
    uint16_t sleepfor;
    uint32_t sdResumePos;
    bool     emptyFS;
    uint16_t vuThreshold;
    uint16_t screensaverTicks;
    uint16_t screensaverPlayingTicks;
    bool     isScreensaver;
    int      newConfigMode;
    char      tmpBuf[BUFLEN];
    char     tmpBuf2[BUFLEN];
    char       ipBuf[16];
    char _stationBuf[BUFLEN/2];
  public:
    Config() {};
    //void save();
#if IR_PIN!=255
    void saveIR();
#endif
    void init();
    void loadTheme();
    uint8_t setVolume(uint8_t val);
    void saveVolume();
    void setTone(int8_t bass, int8_t middle, int8_t trebble);
    void setBalance(int8_t balance);
    uint8_t setLastStation(uint16_t val);
    uint8_t setCountStation(uint16_t val);
    uint8_t setLastSSID(uint8_t val);
    void setTitle(const char* title);
    void setStation(const char* station);
    void escapeQuotes(const char* input, char* output, size_t maxLen);
    bool parseCSV(const char* line, char* name, char* url, int &ovol);
    bool parseJSON(const char* line, char* name, char* url, int &ovol);
    bool parseWsCommand(const char* line, char* cmd, char* val, uint8_t cSize);
    bool parseSsid(const char* line, char* ssid, char* pass);
    bool loadStation(uint16_t station);
    bool initNetwork();
    bool saveWifi();
    void setTimeConf();
    bool saveWifiFromNextion(const char* post);
    void setSmartStart(uint8_t ss);
    void setBitrateFormat(BitrateFormat fmt) { configFmt = fmt; }
    void initPlaylist();
    void indexPlaylist();
    void initSDPlaylist();
    void changeMode(int newmode=-1);
    uint16_t playlistLength();
    uint16_t lastStation(){
      return getMode()==PM_WEB?store.lastStation:store.lastSdStation;
    }
    void lastStation(uint16_t newstation){
      if(getMode()==PM_WEB) saveValue(&store.lastStation, newstation);
      else saveValue(&store.lastSdStation, newstation);
    }
    char * stationByNum(uint16_t num);
    void setTimezone(int8_t tzh, int8_t tzm);
    void setTimezoneOffset(uint16_t tzo);
    uint16_t getTimezoneOffset();
    void setBrightness(bool dosave=false);
    void setDspOn(bool dspon, bool saveval = true);
    void sleepForAfter(uint16_t sleepfor, uint16_t sleepafter=0);
    void bootInfo();
    void doSleepW();
    void setSnuffle(bool sn);
    uint8_t getMode() { return store.play_mode/* & 0b11*/; }
    void initPlaylistMode();
    void reset();
    void enableScreensaver(bool val);
    void setScreensaverTimeout(uint16_t val);
    void setScreensaverBlank(bool val);
    void setScreensaverPlayingEnabled(bool val);
    void setScreensaverPlayingTimeout(uint16_t val);
    void setScreensaverPlayingBlank(bool val);
    void setSntpOne(const char *val);
    void setShowweather(bool val);
    void setWeatherKey(const char *val);
    void setSDpos(uint32_t val);
#if IR_PIN!=255
    void setIrBtn(int val);
#endif
    void resetSystem(const char *val, uint8_t clientId);
    bool spiffsCleanup();
    void waitConnection();
    char * ipToStr(IPAddress ip);
    bool prepareForPlaying(uint16_t stationId);
    void configPostPlaying(uint16_t stationId);
    FS* SDPLFS(){ return _SDplaylistFS; }
    bool isRTCFound(){ return _rtcFound; };
    template <typename T>
    size_t getAddr(const T *field) const {
      return (size_t)((const uint8_t *)field - (const uint8_t *)&store) + EEPROM_START;
    }
    template <typename T>
    void saveValue(T *field, const T &value, bool commit=true, bool force=false){
      if(*field == value && !force) return;
      *field = value;
      size_t address = getAddr(field);
      EEPROM.put(address, value);
      if(commit)
        EEPROM.commit();
    }
    void saveValue(char *field, const char *value, size_t N, bool commit=true, bool force=false) {
      if (strcmp(field, value) == 0 && !force) return;
      strlcpy(field, value, N);
      size_t address = getAddr(field);
      size_t fieldlen = strlen(field);
      for (size_t i = 0; i <=fieldlen ; i++) EEPROM.write(address + i, field[i]);
      if(commit)
        EEPROM.commit();
    }
    uint32_t getChipId(){
      uint32_t chipId = 0;
      for(int i=0; i<17; i=i+8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
      }
      return chipId;
    }
  private:
    template <class T> int eepromWrite(int ee, const T& value);
    template <class T> int eepromRead(int ee, T& value);
    bool _bootDone;
    bool _rtcFound;
    FS* _SDplaylistFS;
    void setDefaults();
    static void doSleep();
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    void _setupVersion();
    void _initHW();
    bool _isFSempty();
    uint16_t _randomStation(){
      randomSeed(esp_random() ^ millis());
      uint16_t station = random(1, store.countStation);
      return station;
    }
};

extern Config config;
#if DSP_HSPI || TS_HSPI || VS_HSPI
extern SPIClass  SPI2;
#endif

#endif
