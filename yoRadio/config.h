#ifndef config_h
#define config_h
#include "Arduino.h"
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

void DBGVB(const char *format, ...);

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
  uint8_t   irto;
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
  char ssid[20];
  char password[40];
};

class Config {
  public:
    config_t store;
    station_t station;
#if IR_PIN!=255
    int irindex;
    uint8_t irchck;
    ircodes_t ircodes;
#endif
    neworkItem ssids[5];
    byte ssidsCount;
  public:
    Config() {};
    void save();
#if IR_PIN!=255
    void saveIR();
#endif
    void init();
    byte setVolume(byte val);
    void saveVolume();
    void setTone(int8_t bass, int8_t middle, int8_t trebble);
    void setBalance(int8_t balance);
    byte setLastStation(byte val);
    byte setCountStation(byte val);
    byte setLastSSID(byte val);
    void setTitle(const char* title);
    void setStation(const char* station);
    bool parseCSV(const char* line, char* name, char* url, int &ovol);
    bool parseJSON(const char* line, char* name, char* url, int &ovol);
    bool parseWsCommand(const char* line, char* cmd, char* val, byte cSize);
    bool parseSsid(const char* line, char* ssid, char* pass);
    void loadStation(uint16_t station);
    bool initNetwork();
    bool saveWifi(const char* post);
    void setSmartStart(byte ss);
    void initPlaylist();
    void indexPlaylist();
    void fillPlMenu(char plmenu[][40], int from, byte count, bool removeNum = false);
    void setTimezone(int8_t tzh, int8_t tzm);
    void setTimezoneOffset(uint16_t tzo);
    uint16_t getTimezoneOffset();
    void setBrightness(bool dosave=false);
    
  private:
    template <class T> int eepromWrite(int ee, const T& value);
    template <class T> int eepromRead(int ee, T& value);
    void setDefaults();
};

extern Config config;

#endif
