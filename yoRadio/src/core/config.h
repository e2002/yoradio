#ifndef config_h
#define config_h
#include "Arduino.h"
#include <Ticker.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <Preferences.h>
//#include "SD.h"
#include "options.h"
#include "rtcsupport.h"
#include "../pluginsManager/pluginsManager.h"

#ifndef BUFLEN
  #define BUFLEN            170
#endif
#define PLAYLIST_PATH     "/data/playlist.csv"
#define SSIDS_PATH        "/data/wifi.csv"
#define TMP_PATH          "/data/tmpfile.txt"
#define INDEX_PATH        "/data/index.dat"

#define PLAYLIST_SD_PATH     "/data/playlistsd.csv"
#define INDEX_SD_PATH        "/data/indexsd.dat"

#ifdef DEBUG_V
#define DBGH()       { Serial.printf("[%s:%s:%d] Heap: %d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__, xPortGetFreeHeapSize()); }
#define DBGVB( ... ) { char buf[200]; sprintf( buf, __VA_ARGS__ ) ; Serial.print("[DEBUG]\t"); Serial.println(buf); }
#else
#define DBGVB( ... )
#define DBGH()
#endif
#define BOOTLOG( ... ) { char buf[120]; sprintf( buf, __VA_ARGS__ ) ; Serial.print("##[BOOT]#\t"); Serial.println(buf); }
#define EVERY_MS(x)  static uint32_t tmr; bool flag = millis() - tmr >= (x); if (flag) tmr += (x); if (flag)
#define REAL_PLAYL   getMode()==PM_WEB?PLAYLIST_PATH:PLAYLIST_SD_PATH
#define REAL_INDEX   getMode()==PM_WEB?INDEX_PATH:INDEX_SD_PATH

#define MAX_PLAY_MODE   1
#define WEATHERKEY_LENGTH 58
#define MDNS_LENGTH 24
#if SDC_CS!=255
  #define USE_SD
#endif
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  #define ESP_ARDUINO_3 1
#endif
enum playMode_e      : uint8_t  { PM_WEB=0, PM_SDCARD=1 };
enum BitrateFormat { BF_UNCNOWN, BF_MP3, BF_AAC, BF_FLAC, BF_OGG, BF_WAV };

void u8fix(char *src);

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
struct config_t // specify defaults here (defaults are NOT saved to Prefs)
{
  uint16_t  config_set = 4262;
  uint8_t   volume = 12;
  int8_t    balance = 0;
  int8_t    trebble = 0;
  int8_t    middle = 0;
  int8_t    bass = 0;
  uint16_t  lastStation = 0;
  uint16_t  countStation = 0;
  uint8_t   lastSSID = 0;
  bool      audioinfo = false;
  uint8_t   smartstart = 2;
  int8_t    tzHour = 3;
  int8_t    tzMin = 0;
  uint16_t  timezoneOffset = 0;
  bool      vumeter = false;
  uint8_t   softapdelay = 0;
  bool      flipscreen = false;
  bool      invertdisplay = false;
  bool      numplaylist = false;
  bool      fliptouch = false;
  bool      dbgtouch = false;
  bool      dspon = true;
  uint8_t   brightness = 100;
  uint8_t   contrast = 55;
  char      sntp1[35] = "pool.ntp.org";
  char      sntp2[35] = "1.ru.pool.ntp.org";
  bool      showweather = false;
  char      weatherlat[10] = "55.7512";
  char      weatherlon[10] = "37.6184";
  char      weatherkey[WEATHERKEY_LENGTH] = "";
  uint16_t  _reserved = 0;
  uint16_t  lastSdStation = 0;
  bool      sdsnuffle = false;
  uint8_t   volsteps = 1;
  uint16_t  encacc = 200;
  uint8_t   play_mode = 0;
  uint8_t   irtlp = 35;
  bool      btnpullup = true;
  uint16_t  btnlongpress = 200;
  uint16_t  btnclickticks = 300;
  uint16_t  btnpressticks = 500;
  bool      encpullup = false;
  bool      enchalf = false;
  bool      enc2pullup = false;
  bool      enc2half = false;
  bool      forcemono = false;
  bool      i2sinternal = false;
  bool      rotate90 = false;
  bool      screensaverEnabled = false;
  uint16_t  screensaverTimeout = 20;
  bool      screensaverBlank = false;
  bool      screensaverPlayingEnabled = false;
  uint16_t  screensaverPlayingTimeout = 5;
  bool      screensaverPlayingBlank = false;
  char      mdnsname[24] = "";
  bool      skipPlaylistUpDown = false;
  // if adding a variable, you can do it anywhere, just be sure to add it to configKeyMap() in config.cpp
  // if removing a variable and key, add to deleteOldKeys()
};

#define CONFIG_KEY_ENTRY(field, keyname) { offsetof(config_t, field), keyname, sizeof(((config_t*)0)->field) }

struct configKeyMap {
    size_t fieldOffset;
    const char* key;
    size_t size;
};

#if IR_PIN!=255
struct ircodes_t
{
  unsigned int ir_set = 0; // will be 4224 if written/restored correctly
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
    BitrateFormat configFmt = BF_UNCNOWN;
    neworkItem ssids[5];
    uint8_t ssidsCount;
    uint16_t sleepfor;
    uint32_t sdResumePos;
    bool     emptyFS;
    uint16_t vuThreshold;
    uint16_t screensaverTicks;
    uint16_t screensaverPlayingTicks;
    bool     isScreensaver;
  public:
    Config() {};
    //void save();
#if IR_PIN!=255
    void saveIR();
#endif
    void init();
    void loadPreferences();
    void deleteOldKeys();
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
    bool parseCSV(const char* line, char* name, char* url, int &ovol);
    bool parseJSON(const char* line, char* name, char* url, int &ovol);
    bool parseWsCommand(const char* line, char* cmd, char* val, uint8_t cSize);
    bool parseSsid(const char* line, char* ssid, char* pass);
    void loadStation(uint16_t station);
    bool initNetwork();
    bool saveWifi();
    bool saveWifiFromNextion(const char* post);
    void setSmartStart(uint8_t ss);
    void setBitrateFormat(BitrateFormat fmt) { configFmt = fmt; }
    void initPlaylist();
    void indexPlaylist();
    #ifdef USE_SD
      void initSDPlaylist();
      void changeMode(int newmode=-1);
    #endif
    uint16_t lastStation(){
      return getMode()==PM_WEB?store.lastStation:store.lastSdStation;
    }
    void lastStation(uint16_t newstation){
      if(getMode()==PM_WEB) saveValue(&store.lastStation, newstation);
      else saveValue(&store.lastSdStation, newstation);
    }
    uint8_t fillPlMenu(int from, uint8_t count, bool fromNextion=false);
    char * stationByNum(uint16_t num);
    void setBrightness(bool dosave=false);
    void setDspOn(bool dspon, bool saveval = true);
    void sleepForAfter(uint16_t sleepfor, uint16_t sleepafter=0);
    void bootInfo();
    void doSleepW();
    void setSnuffle(bool sn);
    uint8_t getMode() { return store.play_mode/* & 0b11*/; }
    void initPlaylistMode();
    void reset();
    bool spiffsCleanup();
    FS* SDPLFS(){ return _SDplaylistFS; }
    #if RTCSUPPORTED
      bool isRTCFound(){ return _rtcFound; };
    #endif
    Preferences prefs; // For Preferences, we use a look-up table to maintain compatibility...
    static const configKeyMap keyMap[];

    // Helper to get key map entry for a field pointer
    const configKeyMap* getKeyMapEntryForField(const void* field) const {
        size_t offset = (const uint8_t*)field - (const uint8_t*)&store;
        for (size_t i = 0; keyMap[i].key != nullptr; ++i) {
            if (keyMap[i].fieldOffset == offset) return &keyMap[i];
        }
        return nullptr;
    }
    template <typename T>
    void loadValue(T *field) {
      const configKeyMap* entry = getKeyMapEntryForField(field);
      if (entry) prefs.getBytes(entry->key, field, entry->size);
    }
    template <typename T>
    void saveValue(T *field, const T &value, bool commit=true, bool force=false) {
      // commit ignored (kept for compatibility)
      const configKeyMap* entry = getKeyMapEntryForField(field);
      if (entry) {
        prefs.begin("yoradio", false);
        T oldValue;
        size_t existingLen = prefs.getBytesLength(entry->key);
        size_t bytesRead = prefs.getBytes(entry->key, &oldValue, entry->size);
        bool exists = bytesRead == entry->size;
        bool needSave = (existingLen != entry->size) || !exists || memcmp(&oldValue, &value, entry->size) != 0;
        if (needSave) {
          prefs.putBytes(entry->key, &value, entry->size);
          *field = value;
        }
        prefs.end();
      }
    }
    void saveValue(char *field, const char *value, size_t N = 0, bool commit=true, bool force=false) {
      // commit ignored (kept for compatibility)
      const configKeyMap* entry = getKeyMapEntryForField(field);
      if (entry) {
        size_t sz = entry->size;
        prefs.begin("yoradio", false);
        char oldValue[sz];
        memset(oldValue, 0, sz);
        size_t existingLen = prefs.getBytesLength(entry->key);
        bool exists = prefs.getBytes(entry->key, oldValue, sz) == sz;
        bool needSave = (existingLen != sz) || !exists || strncmp(oldValue, value, sz) != 0 || force;
        if (needSave) {
          prefs.putBytes(entry->key, value, sz);
          strlcpy(field, value, sz);
        }
        prefs.end();
      }
    }
    uint32_t getChipId(){
      uint32_t chipId = 0;
      for(int i=0; i<17; i=i+8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
      }
      return chipId;
    }
  private:
    bool _bootDone;
    #if RTCSUPPORTED
      bool _rtcFound;
    #endif
    FS* _SDplaylistFS;
    void setDefaults();
    Ticker   _sleepTimer;
    static void doSleep();
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    void _initHW();
    bool _isFSempty();
    uint16_t _randomStation(){
      randomSeed(esp_random() ^ millis());
      uint16_t station = random(1, store.countStation);
      return station;
    }
    char _stationBuf[BUFLEN/2];

};

extern Config config;
#if DSP_HSPI || TS_HSPI || VS_HSPI
extern SPIClass  SPI2;
#endif

#endif
