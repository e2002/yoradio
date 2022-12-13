#ifndef config_h
#define config_h
#include "Arduino.h"
#include <Ticker.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <SD.h>
#include "options.h"

#define EEPROM_SIZE       768
#define EEPROM_START      500
#define EEPROM_START_IR   0
#define EEPROM_START_2    10
#define BUFLEN            140
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
  unsigned int config_set; //must be 4262
  byte volume;
  int8_t balance;
  int8_t trebble;
  int8_t middle;
  int8_t bass;
  uint16_t lastStation;
  uint16_t countStation;
  byte lastSSID;
  bool audioinfo;
  byte smartstart;
  int8_t tzHour;
  int8_t tzMin;
  uint16_t timezoneOffset;

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
  char      weatherkey[64];
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
    neworkItem ssids[5];
    byte ssidsCount;
    uint16_t sleepfor;
  public:
    Config() {};
    void save();
#if IR_PIN!=255
    void saveIR();
#endif
    void init();
    void loadTheme();
    byte setVolume(byte val);
    void saveVolume();
    void setTone(int8_t bass, int8_t middle, int8_t trebble);
    void setBalance(int8_t balance);
    byte setLastStation(uint16_t val);
    byte setCountStation(uint16_t val);
    byte setLastSSID(byte val);
    void setTitle(const char* title);
    void setStation(const char* station);
    bool parseCSV(const char* line, char* name, char* url, int &ovol);
    bool parseJSON(const char* line, char* name, char* url, int &ovol);
    bool parseWsCommand(const char* line, char* cmd, char* val, byte cSize);
    bool parseSsid(const char* line, char* ssid, char* pass);
    void loadStation(uint16_t station);
    bool initNetwork();
    bool saveWifi();
    bool saveWifiFromNextion(const char* post);
    void setSmartStart(byte ss);
    void initPlaylist();
    void indexPlaylist();
    void listSD(File &plSDfile, File &plSDindex, const char * dirname, uint8_t levels);
    void initSDPlaylist();
    void indexSDPlaylist();
    void fillPlMenu(char plmenu[][40], int from, byte count, bool removeNum = false);
    void setTimezone(int8_t tzh, int8_t tzm);
    void setTimezoneOffset(uint16_t tzo);
    uint16_t getTimezoneOffset();
    void setBrightness(bool dosave=false);
    void setDspOn(bool dspon);
    void sleepForAfter(uint16_t sleepfor, uint16_t sleepafter=0);
    void bootInfo();
  private:
    template <class T> int eepromWrite(int ee, const T& value);
    template <class T> int eepromRead(int ee, T& value);
    void setDefaults();
    Ticker   _sleepTimer;
    static void doSleep();
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
};

extern Config config;
#if DSP_HSPI || TS_HSPI || VS_HSPI
extern SPIClass  SPI2;
#endif


#endif
