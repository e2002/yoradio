#ifndef sdmanager_h
#define sdmanager_h
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "vfs_api.h"
#include "sd_diskio.h"
#include "options.h"

enum cardStatus_e    : uint8_t  { CS_NONE=0, CS_PRESENT=1, CS_MOUNTED=2, CS_EJECTED=3 };

class SDManager : public SDFS {
  public:
    bool ready;
  public:
    SDManager(FSImplPtr impl) : SDFS(impl) {}
    
    bool init();
    void mount();
    bool cardPresent();
    void listSD(File &plSDfile, File &plSDindex, const char * dirname, uint8_t levels);
    void indexSDPlaylist();
    void checkSD();
    cardStatus_e status(){ return _cardStatus; };
    void status(cardStatus_e newstatus){ _cardStatus=newstatus; };
    void clearCardStatus() { if(_cardStatus!=CS_NONE) _cardStatus=CS_NONE; }
  private:
    uint32_t _sdFCount;
    cardStatus_e _cardStatus;
  private:
    bool _checkNoMedia(const char* path);
    bool _endsWith (const char* base, const char* str);
};

extern SDManager sdman;
#if defined(SD_SPIPINS) || SD_HSPI
extern SPIClass  SDSPI;
#endif
#endif
