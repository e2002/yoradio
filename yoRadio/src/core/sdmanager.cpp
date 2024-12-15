#if SDC_CS!=255

#define USE_SD
#include "sdmanager.h"
#include "display.h"

#if defined(SD_SPIPINS) || SD_HSPI
SPIClass  SDSPI(HSPI);
#define SDREALSPI SDSPI
#else
  #define SDREALSPI SPI
#endif

#ifndef SDSPISPEED
  #define SDSPISPEED 4000000
#endif

SDManager sdman(FSImplPtr(new VFSImpl()));

bool SDManager::start(){
  ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(10);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(10);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  return ready;
}

void SDManager::stop(){
  end();
  ready = false;
}
#include "diskio_impl.h"
bool SDManager::cardPresent() {

  if(!ready) return false;
  if(sectorSize()<1) {
    return false;
  }
  uint8_t buff[sectorSize()] = { 0 };
  bool bread = readRAW(buff, 1);
  if(sectorSize()>0 && !bread) return false;
  return bread;
}

bool SDManager::_checkNoMedia(const char* path){
  char nomedia[BUFLEN]= {0};
  strlcat(nomedia, path, BUFLEN);
  strlcat(nomedia, "/.nomedia", BUFLEN);
  bool nm = exists(nomedia);
  return nm;
}

bool SDManager::_endsWith (const char* base, const char* str) {
  int slen = strlen(str) - 1;
  const char *p = base + strlen(base) - 1;
  while(p > base && isspace(*p)) p--;
  p -= slen;
  if (p < base) return false;
  return (strncmp(p, str, slen) == 0);
}

void SDManager::listSD(File &plSDfile, File &plSDindex, const char * dirname, uint8_t levels){
  File root = sdman.open(dirname);
  if(!root){
    Serial.println("##[ERROR]#\tFailed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("##[ERROR]#\tNot a directory");
    return;
  }
  File file = root.openNextFile();
  uint32_t pos = 0;
  while(file){
  	vTaskDelay(2);
    bool fid = file.isDirectory();
    const char * fp = file.path();
    const char * fn = file.name();
    if(fid){
      if(levels && !_checkNoMedia(fp)){
        listSD(plSDfile, plSDindex, fp, levels -1);
      }
    } else {
      if(_endsWith(strlwr((char*)fn), ".mp3") || _endsWith(fn, ".m4a") || _endsWith(fn, ".aac") || _endsWith(fn, ".wav") || _endsWith(fn, ".flac")){
        pos = plSDfile.position();
        plSDfile.print(fn); plSDfile.print("\t"); plSDfile.print(fp); plSDfile.print("\t"); plSDfile.println(0);
        plSDindex.write((uint8_t *) &pos, 4);
        Serial.print(".");
        if(display.mode()==SDCHANGE) display.putRequest(SDFILEINDEX, _sdFCount+1);
        _sdFCount++;
        if(_sdFCount%64==0) Serial.println();
      }
    }
    if(file) file.close(); file = root.openNextFile();
  }
  if(root) root.close();
}

void SDManager::indexSDPlaylist() {
  _sdFCount = 0;
  if(exists(PLAYLIST_SD_PATH)) remove(PLAYLIST_SD_PATH);
  if(exists(INDEX_SD_PATH)) remove(INDEX_SD_PATH);
  File playlist = open(PLAYLIST_SD_PATH, "w", true);
  if (!playlist) {
    return;
  }
  File index = open(INDEX_SD_PATH, "w", true);
  listSD(playlist, index, "/", SD_MAX_LEVELS);
  index.flush();
  index.close();
  playlist.flush();
  playlist.close();
  Serial.println();
  delay(50);
}
#endif


