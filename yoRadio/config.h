#ifndef config_h
#define config_h
#include "Arduino.h"

#define EEPROM_SIZE   1024
#define EEPROM_START  0
#define BUFLEN        140
#define PLAYLIST_PATH "/data/playlist.csv"
#define SSIDS_PATH "/data/wifi.csv"
#define TMP_PATH "/data/tmpfile.txt"
#define INDEX_PATH "/data/index.dat"

struct config_t
{
  unsigned int config_set; //must be 4256
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
};

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
    neworkItem ssids[5];
    byte ssidsCount;
  public:
    Config() {};
    void save();
    void init();
    byte setVolume(byte val, bool dosave);
    void setTone(int8_t bass, int8_t middle, int8_t trebble); 
    void setBalance(int8_t balance);
    byte setLastStation(byte val);
    byte setCountStation(byte val);
    byte setLastSSID(byte val);
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
    void fillPlMenu(char plmenu[][40], int from, byte count);
  private:
    template <class T> int eepromWrite(int ee, const T& value);
    template <class T> int eepromRead(int ee, T& value);
    void setDefaults();
};

extern Config config;

#endif
