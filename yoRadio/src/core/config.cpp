#include "config.h"

//#include <SPIFFS.h>
#include "display.h"
#include "player.h"
#include "network.h"
#include "netserver.h"
#include "controls.h"
#include "timekeeper.h"
#ifdef USE_SD
#include "sdmanager.h"
#endif
#include <cstddef>
#include "../ESPFileUpdater/ESPFileUpdater.h"

// List of required web asset files
static const char* requiredFiles[] = {"dragpl.js.gz","ir.css.gz","irrecord.html.gz","ir.js.gz","logo.svg.gz","options.html.gz","script.js.gz",
                                     "timezones.json.gz","rb_srvrs.json","search.html.gz","search.js.gz","search.css.gz",
                                     "style.css.gz","updform.html.gz","theme.css","player.html.gz"}; // keep main page at end
static const size_t requiredFilesCount = sizeof(requiredFiles) / sizeof(requiredFiles[0]);

Config config;

bool wasUpdated(ESPFileUpdater::UpdateStatus status) { return status == ESPFileUpdater::UPDATED; }
#ifdef HEAP_DBG
void printHeapFragmentationInfo(const char* title){
  size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
  float fragmentation = 100.0 * (1.0 - ((float)largestBlock / (float)freeHeap));
  Serial.printf("\n****** %s ******\n", title);
  Serial.printf("* Free heap: %u bytes\n", freeHeap);
  Serial.printf("* Largest free block: %u bytes\n", largestBlock);
  Serial.printf("* Fragmentation: %.2f%%\n", fragmentation);
  Serial.printf("*************************************\n\n");
}
#endif

void u8fix(char *src){
  char last = src[strlen(src)-1]; 
  if ((uint8_t)last >= 0xC2) src[strlen(src)-1]='\0';
}

bool Config::_isFSempty() {
  // Use global requiredFiles and requiredFilesCount
  char fullpath[32];
  for (size_t i = 0; i < requiredFilesCount; i++) {
    sprintf(fullpath, "/www/%s", requiredFiles[i]);
    if(!SPIFFS.exists(fullpath)) {
      Serial.println(fullpath);
      return true;
    }
  }
  return false;
}

void Config::init() {
  sdResumePos = 0;
  screensaverTicks = 0;
  screensaverPlayingTicks = 0;
  newConfigMode = 0;
  isScreensaver = false;
  memset(tmpBuf, 0, BUFLEN);
  //bootInfo();
#if RTCSUPPORTED
  _rtcFound = false;
  BOOTLOG("RTC begin(SDA=%d,SCL=%d)", RTC_SDA, RTC_SCL);
  if(rtc.init()){
    BOOTLOG("done");
    _rtcFound = true;
  }else{
    BOOTLOG("[ERROR] - Couldn't find RTC");
  }
#endif
  emptyFS = true;
#if IR_PIN!=255
    irindex=-1;
#endif
#if defined(SD_SPIPINS) || SD_HSPI
  #if !defined(SD_SPIPINS)
    SDSPI.begin();
  #else
    SDSPI.begin(SD_SPIPINS); // SCK, MISO, MOSI
  #endif
#endif
  loadPreferences();
  bootInfo(); // https://github.com/e2002/yoradio/pull/149
  if (store.config_set != 4262) {
    setDefaults();
  }
  store.play_mode = store.play_mode & 0b11;
  if(store.play_mode>1) store.play_mode=PM_WEB;
  _initHW();
  if (!SPIFFS.begin(true)) {
    Serial.println("##[ERROR]#\tSPIFFS Mount Failed");
    return;
  }
  BOOTLOG("SPIFFS mounted");
  emptyFS = _isFSempty();
  if(emptyFS) {
    #ifndef FILESURL
      BOOTLOG("SPIFFS is empty!");
    #else
      BOOTLOG("SPIFFS is empty.  Will attempt to get files from online...");
      File markerFile = SPIFFS.open(ONLINEUPDATE_MARKERFILE, "w");
      if (markerFile) markerFile.close();
      display.putRequest(NEWMODE, UPDATING);
    #endif
  }
  ssidsCount = 0;
  #ifdef USE_SD
  _SDplaylistFS = getMode()==PM_SDCARD?&sdman:(true?&SPIFFS:_SDplaylistFS);
  #else
  _SDplaylistFS = &SPIFFS;
  #endif
  _bootDone=false;
  setTimeConf();
}

void Config::loadPreferences() {
  prefs.begin("yoradio", false);
  // Check config_set first
  uint16_t configSetValue = 0;
  size_t configSetRead = prefs.getBytes("cfgset", &configSetValue, sizeof(configSetValue));
  if (configSetRead != sizeof(configSetValue)) {
    // Preferences is empty, save config_set and version
    Serial.println("[Prefs] Empty NVS detected, initializing config_set...\n");
    saveValue(&store.config_set, store.config_set);
  } else if (configSetValue != 4262) {
    // config_set present but not valid, reset config
    Serial.printf("[Prefs] Invalid config_set (%u), resetting config...\n", configSetValue);
    prefs.end();
    reset();
    return;
  }
  // Load all fields in keyMap
  for (size_t i = 0; keyMap[i].key != nullptr; ++i) {
    uint8_t* field = (uint8_t*)&store + keyMap[i].fieldOffset;
    size_t sz = keyMap[i].size;
    size_t read = prefs.getBytes(keyMap[i].key, field, sz);
  }
  deleteOldKeys();
  prefs.end();
}

#ifdef USE_SD

void Config::changeMode(int newmode){
  bool pir = player.isRunning();
  if(SDC_CS==255) return;
  if(getMode()==PM_SDCARD) {
    sdResumePos = player.getFilePos();
  }
  if(network.status==SOFT_AP || display.mode()==LOST){
    saveValue(&store.play_mode, static_cast<uint8_t>(PM_SDCARD));
    delay(50);
    ESP.restart();
  }
  if(!sdman.ready && newmode!=PM_WEB) {
    if(!sdman.start()){
      Serial.println("##[ERROR]#\tSD Not Found");
      netserver.requestOnChange(GETPLAYERMODE, 0);
      sdman.stop();
      return;
    }
  }
  if(newmode<0||newmode>MAX_PLAY_MODE){
    store.play_mode++;
    if(getMode() > MAX_PLAY_MODE) store.play_mode=0;
  }else{
    store.play_mode=(playMode_e)newmode;
  }
  saveValue(&store.play_mode, store.play_mode, true, true);
  _SDplaylistFS = getMode()==PM_SDCARD?&sdman:(true?&SPIFFS:_SDplaylistFS);
  if(getMode()==PM_SDCARD){
    if(pir) player.sendCommand({PR_STOP, 0});
    display.putRequest(NEWMODE, SDCHANGE);
    #ifdef NETSERVER_LOOP1
    while(display.mode()!=SDCHANGE)
      delay(10);
    #endif
    delay(50);
  }
  if(getMode()==PM_WEB) {
    if(network.status==SDREADY) ESP.restart();
    sdman.stop();
  }
  if(!_bootDone) return;
  initPlaylistMode();
  if (pir) player.sendCommand({PR_PLAY, getMode()==PM_WEB?store.lastStation:store.lastSdStation});
  netserver.resetQueue();
  //netserver.requestOnChange(GETPLAYERMODE, 0);
  netserver.requestOnChange(GETINDEX, 0);
  //netserver.requestOnChange(GETMODE, 0);
 // netserver.requestOnChange(CHANGEMODE, 0);
  display.resetQueue();
  display.putRequest(NEWMODE, PLAYER);
  display.putRequest(NEWSTATION);
}

void Config::initSDPlaylist() {
  //store.countStation = 0;
  bool doIndex = !sdman.exists(INDEX_SD_PATH);
  if(doIndex) sdman.indexSDPlaylist();
  if (SDPLFS()->exists(INDEX_SD_PATH)) {
    File index = SDPLFS()->open(INDEX_SD_PATH, "r");
    //store.countStation = index.size() / 4;
    if(doIndex){
      lastStation(_randomStation());
      sdResumePos = 0;
    }
    index.close();
    //saveValue(&store.countStation, store.countStation, true, true);
  }
}

#endif //#ifdef USE_SD

bool Config::spiffsCleanup(){
  bool ret = (SPIFFS.exists(PLAYLIST_SD_PATH)) || (SPIFFS.exists(INDEX_SD_PATH)) || (SPIFFS.exists(INDEX_PATH));
  if(SPIFFS.exists(PLAYLIST_SD_PATH)) SPIFFS.remove(PLAYLIST_SD_PATH);
  if(SPIFFS.exists(INDEX_SD_PATH)) SPIFFS.remove(INDEX_SD_PATH);
  if(SPIFFS.exists(INDEX_PATH)) SPIFFS.remove(INDEX_PATH);
  return ret;
}

void Config::waitConnection(){
  while(!player.connproc) vTaskDelay(50);
  vTaskDelay(500);
}

char * Config::ipToStr(IPAddress ip){
  snprintf(ipBuf, 16, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return ipBuf;
}
bool Config::prepareForPlaying(uint16_t stationId){
  setDspOn(1);
  vuThreshold = 0;
  screensaverTicks=SCREENSAVERSTARTUPDELAY;
  screensaverPlayingTicks=SCREENSAVERSTARTUPDELAY;
  if(getMode()!=PM_SDCARD) {
    display.putRequest(PSTOP);
  }
  
  if(!loadStation(stationId)) return false;
  setTitle(getMode()==PM_WEB?const_PlConnect:"[next track]");
  station.bitrate=0;
  setBitrateFormat(BF_UNCNOWN);
  display.putRequest(DBITRATE);
  display.putRequest(NEWSTATION);
  display.putRequest(NEWMODE, PLAYER);
  netserver.requestOnChange(STATION, 0);
  netserver.requestOnChange(MODE, 0);
  netserver.loop();
  netserver.loop();
  if(store.smartstart!=2)
    setSmartStart(0);
  return true;
}
void Config::configPostPlaying(uint16_t stationId){
  if(getMode()==PM_SDCARD) {
    sdResumePos = 0;
    saveValue(&store.lastSdStation, stationId);
  }
  if(store.smartstart!=2) setSmartStart(1);
  netserver.requestOnChange(MODE, 0);
  //display.putRequest(NEWMODE, PLAYER);
  display.putRequest(PSTART);
}
void Config::initPlaylistMode(){
  uint16_t _lastStation = 0;
  uint16_t cs = playlistLength();
  #ifdef USE_SD
    if(getMode()==PM_SDCARD){
      if(!sdman.start()){
        store.play_mode=PM_WEB;
        Serial.println("SD Mount Failed");
        changeMode(PM_WEB);
        _lastStation = store.lastStation;
      }else{
        if(_bootDone) Serial.println("SD Mounted"); else BOOTLOG("SD Mounted");
          if(_bootDone) Serial.println("Waiting for SD card indexing..."); else BOOTLOG("Waiting for SD card indexing...");
          initSDPlaylist();
          if(_bootDone) Serial.println("done"); else BOOTLOG("done");
          _lastStation = store.lastSdStation;
          
          if(_lastStation>cs && cs>0){
            _lastStation=1;
          }
          if(_lastStation==0) {
            _lastStation = _randomStation();
          }
      }
    }else{
      Serial.println("done");
      _lastStation = store.lastStation;
    }
  #else //ifdef USE_SD
    store.play_mode=PM_WEB;
    _lastStation = store.lastStation;
  #endif
  if(getMode()==PM_WEB && !emptyFS) initPlaylist();
  log_i("%d" ,_lastStation);
  if (_lastStation == 0 && cs > 0) {
    _lastStation = getMode()==PM_WEB?1:_randomStation();
  }
  lastStation(_lastStation);
  saveValue(&store.play_mode, store.play_mode, true, true);
  _bootDone = true;
  loadStation(_lastStation);
}

void Config::_initHW(){
  loadTheme();
  #if IR_PIN!=255
  prefs.begin("yoradio", false);
  memset(&ircodes, 0, sizeof(ircodes));
  size_t read = prefs.getBytes("ircodes", &ircodes, sizeof(ircodes));
  if (read != sizeof(ircodes) || ircodes.ir_set != 4224) {
      Serial.println("[_initHW] ircodes not initialized or corrupt, resetting...");
      prefs.remove("ircodes");
      memset(ircodes.irVals, 0, sizeof(ircodes.irVals));
  }
  prefs.end();
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
  theme.clockbg       = color565(COLOR_CLOCK_BG);
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
  #include "../displays/tools/tftinverttitle.h"
}

void Config::reset(){
  Serial.print("[Prefs] Reset requested, resetting config...\n");
  prefs.begin("yoradio", false);
  prefs.clear();
  prefs.end();
  setDefaults();
  delay(500);
  ESP.restart();
}
void Config::enableScreensaver(bool val){
  saveValue(&store.screensaverEnabled, val);
#ifndef DSP_LCD
  display.putRequest(NEWMODE, PLAYER);
#endif
}
void Config::setScreensaverTimeout(uint16_t val){
  val=constrain(val,5,65520);
  saveValue(&store.screensaverTimeout, val);
#ifndef DSP_LCD
  display.putRequest(NEWMODE, PLAYER);
#endif
}
void Config::setScreensaverBlank(bool val){
  saveValue(&store.screensaverBlank, val);
#ifndef DSP_LCD
  display.putRequest(NEWMODE, PLAYER);
#endif
}
void Config::setScreensaverPlayingEnabled(bool val){
  saveValue(&store.screensaverPlayingEnabled, val);
#ifndef DSP_LCD
  display.putRequest(NEWMODE, PLAYER);
#endif
}
void Config::setScreensaverPlayingTimeout(uint16_t val){
  val=constrain(val,1,1080);
  config.saveValue(&config.store.screensaverPlayingTimeout, val);
#ifndef DSP_LCD
  display.putRequest(NEWMODE, PLAYER);
#endif
}
void Config::setScreensaverPlayingBlank(bool val){
  saveValue(&store.screensaverPlayingBlank, val);
#ifndef DSP_LCD
  display.putRequest(NEWMODE, PLAYER);
#endif
}

void Config::setShowweather(bool val){
  config.saveValue(&config.store.showweather, val);
  timekeeper.forceWeather = true;
  display.putRequest(SHOWWEATHER);
}
void Config::setWeatherKey(const char *val){
  saveValue(store.weatherkey, val, WEATHERKEY_LENGTH);
  display.putRequest(NEWMODE, CLEAR);
  display.putRequest(NEWMODE, PLAYER);
}
void Config::setSDpos(uint32_t val){
  if (getMode()==PM_SDCARD){
    sdResumePos = 0;
    if(!player.isRunning()){
      player.setResumeFilePos(val-player.sd_min);
      player.sendCommand({PR_PLAY, config.store.lastSdStation});
    }else{
      player.setFilePos(val-player.sd_min);
    }
  }
}
#if IR_PIN!=255
void Config::setIrBtn(int val){
  irindex = val;
  netserver.irRecordEnable = (irindex >= 0);
  irchck = 0;
  netserver.irValsToWs();
  if (irindex < 0) saveIR();
}
#endif
void Config::resetSystem(const char *val, uint8_t clientId){
  if (strcmp(val, "system") == 0) {
    saveValue(&store.smartstart, (uint8_t)2, false);
    saveValue(&store.audioinfo, false, false);
    saveValue(&store.vumeter, false, false);
    saveValue(&store.softapdelay, (uint8_t)0, false);
    saveValue(&store.abuff, (uint16_t)(VS1053_CS==255?7:10), false);
    saveValue(&store.telnet, true);
    saveValue(&store.watchdog, true);
    snprintf(store.mdnsname, MDNS_LENGTH, "yoradio-%x", (unsigned int)getChipId());
    saveValue(store.mdnsname, store.mdnsname, MDNS_LENGTH, true, true);
    display.putRequest(NEWMODE, CLEAR); display.putRequest(NEWMODE, PLAYER);
    netserver.requestOnChange(GETSYSTEM, clientId);
    return;
  }
  if (strcmp(val, "screen") == 0) {
    saveValue(&store.flipscreen, false, false);
    saveValue(&store.volumepage, true);
    saveValue(&store.clock12, false);
    display.flip();
    saveValue(&store.invertdisplay, false, false);
    display.invert();
    saveValue(&store.dspon, true, false);
    store.brightness = 100;
    setBrightness(false);
    saveValue(&store.contrast, (uint8_t)55, false);
    display.setContrast();
    saveValue(&store.numplaylist, false);
    saveValue(&store.screensaverEnabled, false);
    saveValue(&store.screensaverTimeout, (uint16_t)20);
    saveValue(&store.screensaverBlank, false);
    saveValue(&store.screensaverPlayingEnabled, false);
    saveValue(&store.screensaverPlayingTimeout, (uint16_t)5);
    saveValue(&store.screensaverPlayingBlank, false);
    display.putRequest(NEWMODE, CLEAR); display.putRequest(NEWMODE, PLAYER);
    netserver.requestOnChange(GETSCREEN, clientId);
    return;
  }
  if (strcmp(val, "timezone") == 0) {
    saveValue(store.tz_name, TIMEZONE_NAME, sizeof(store.tz_name), false);
    saveValue(store.tzposix, TIMEZONE_POSIX, sizeof(store.tzposix), false);
    saveValue(store.sntp1, SNTP1, sizeof(store.sntp1), false);
    saveValue(store.sntp2, SNTP2, sizeof(store.sntp2));
    saveValue(&store.timeSyncInterval, (uint16_t)60);
    saveValue(&store.timeSyncIntervalRTC, (uint16_t)24);
    timekeeper.forceTimeSync = true;
    netserver.requestOnChange(GETTIMEZONE, clientId);
    return;
  }
  if (strcmp(val, "weather") == 0) {
    saveValue(&store.showweather, false, false);
    saveValue(store.weatherlat, WEATHERLAT, sizeof(store.weatherlat), false);
    saveValue(store.weatherlon, WEATHERLON, sizeof(store.weatherlon), false);
    saveValue(store.weatherkey, "", WEATHERKEY_LENGTH);
    saveValue(&store.weatherSyncInterval, (uint16_t)30);
    //network.trueWeather=false;
    display.putRequest(NEWMODE, CLEAR); display.putRequest(NEWMODE, PLAYER);
    netserver.requestOnChange(GETWEATHER, clientId);
    return;
  }
  if (strcmp(val, "controls") == 0) {
    saveValue(&store.volsteps, (uint8_t)1, false);
    saveValue(&store.fliptouch, false, false);
    saveValue(&store.dbgtouch, false, false);
    saveValue(&store.skipPlaylistUpDown, false);
    setEncAcceleration(200);
    setIRTolerance(40);
    netserver.requestOnChange(GETCONTROLS, clientId);
    return;
  }
  if (strcmp(val, "1") == 0) {
    config.reset();
    return;
  }
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
  store.volumepage = true;
  store.clock12 = false;
  store.invertdisplay=false;
  store.numplaylist=false;
  store.fliptouch=false;
  store.dbgtouch=false;
  store.dspon=true;
  store.brightness=100;
  store.contrast=55;
  strlcpy(store.tz_name,TIMEZONE_NAME, sizeof(store.tz_name));
  strlcpy(store.tzposix,TIMEZONE_POSIX, sizeof(store.tzposix));
  strlcpy(store.sntp1,SNTP1, sizeof(store.sntp1));
  strlcpy(store.sntp2,SNTP2, sizeof(store.sntp2));
  store.showweather=false;
  strlcpy(store.weatherlat,WEATHERLAT, sizeof(store.weatherlat));
  strlcpy(store.weatherlon,WEATHERLON, sizeof(store.weatherlon));
  strlcpy(store.weatherkey,"", WEATHERKEY_LENGTH);
  store._reserved = 0;
  store.lastSdStation = 0;
  store.sdsnuffle = false;
  store.volsteps = 1;
  store.encacc = 200;
  store.play_mode = 0;
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
  store.screensaverEnabled = false;
  store.screensaverTimeout = 20;
  store.screensaverBlank = false;
  snprintf(store.mdnsname, MDNS_LENGTH, "yoradio-%x", (unsigned int)getChipId());
  store.skipPlaylistUpDown = false;
  store.screensaverPlayingEnabled = false;
  store.screensaverPlayingTimeout = 5;
  store.screensaverPlayingBlank = false;
  store.abuff = VS1053_CS==255?7:10;
  store.telnet = true;
  store.watchdog = true;
  store.timeSyncInterval = 60;    //min
  store.timeSyncIntervalRTC = 24; //hour
  store.weatherSyncInterval = 30; //min
}

void Config::setSnuffle(bool sn){
  saveValue(&store.sdsnuffle, sn);
  if(store.sdsnuffle) player.next();
}

#if IR_PIN!=255
void Config::saveIR(){
  ircodes.ir_set = 4224;
  prefs.begin("yoradio", false);
  size_t written = prefs.putBytes("ircodes", &ircodes, sizeof(ircodes));
  prefs.end();
}
#endif

void Config::saveVolume(){
  saveValue(&store.volume, store.volume, true, true);
}

uint8_t Config::setVolume(uint8_t val) {
  store.volume = val;
  display.putRequest(DRAWVOL);
  netserver.requestOnChange(VOLUME, 0);
  return store.volume;
}

void Config::setTone(int8_t bass, int8_t middle, int8_t trebble) {
  saveValue(&store.bass, bass, false);
  saveValue(&store.middle, middle, false);
  saveValue(&store.trebble, trebble);
  player.setTone(store.bass, store.middle, store.trebble);
  netserver.requestOnChange(EQUALIZER, 0);
}

void Config::setSmartStart(uint8_t ss) {
  saveValue(&store.smartstart, ss);
}

void Config::setBalance(int8_t balance) {
  saveValue(&store.balance, balance);
  player.setBalance(store.balance);
  netserver.requestOnChange(BALANCE, 0);
}

uint8_t Config::setLastStation(uint16_t val) {
  lastStation(val);
  return store.lastStation;
}

uint8_t Config::setCountStation(uint16_t val) {
  saveValue(&store.countStation, val);
  return store.countStation;
}

uint8_t Config::setLastSSID(uint8_t val) {
  saveValue(&store.lastSSID, val);
  return store.lastSSID;
}

void Config::setTitle(const char* title) {
  vuThreshold = 0;
  memset(config.station.title, 0, BUFLEN);
  strlcpy(config.station.title, title, BUFLEN);
  u8fix(config.station.title);
  netserver.requestOnChange(TITLE, 0);
  netserver.loop();
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
  int sOvol;
  File index = SPIFFS.open(INDEX_PATH, "w");
  while (playlist.available()) {
    uint32_t pos = playlist.position();
    if (parseCSV(playlist.readStringUntil('\n').c_str(), tmpBuf, tmpBuf2, sOvol)) {
      index.write((uint8_t *) &pos, 4);
    }
  }
  index.close();
  playlist.close();
}

void Config::initPlaylist() {
  //store.countStation = 0;
  if (!SPIFFS.exists(INDEX_PATH)) indexPlaylist();

  /*if (SPIFFS.exists(INDEX_PATH)) {
    File index = SPIFFS.open(INDEX_PATH, "r");
    store.countStation = index.size() / 4;
    index.close();
    saveValue(&store.countStation, store.countStation, true, true);
  }*/
}
uint16_t Config::playlistLength(){
  uint16_t out = 0;
  if (SDPLFS()->exists(REAL_INDEX)) {
    File index = SDPLFS()->open(REAL_INDEX, "r");
    out = index.size() / 4;
    index.close();
  }
  return out;
}
bool Config::loadStation(uint16_t ls) {
  int sOvol;
  uint16_t cs = playlistLength();
  if (cs == 0) {
    memset(station.url, 0, BUFLEN);
    memset(station.name, 0, BUFLEN);
    #ifdef YO_FIX
      strncpy(station.name, "yoRadio", BUFLEN);
    #else
      strncpy(station.name, "ёRadio", BUFLEN);
    #endif
    station.ovol = 0;
    return false;
  }
  if (ls > playlistLength()) {
    ls = 1;
  }
  File playlist = SDPLFS()->open(REAL_PLAYL, "r");
  File index = SDPLFS()->open(REAL_INDEX, "r");
  index.seek((ls - 1) * 4, SeekSet);
  uint32_t pos;
  index.readBytes((char *) &pos, 4);
  index.close();
  playlist.seek(pos, SeekSet);
  if (parseCSV(playlist.readStringUntil('\n').c_str(), tmpBuf, tmpBuf2, sOvol)) {
    memset(station.url, 0, BUFLEN);
    memset(station.name, 0, BUFLEN);
    strncpy(station.name, tmpBuf, BUFLEN);
    strncpy(station.url, tmpBuf2, BUFLEN);
    station.ovol = sOvol;
    setLastStation(ls);
  }
  playlist.close();
  return true;
}

char * Config::stationByNum(uint16_t num){
  File playlist = SDPLFS()->open(REAL_PLAYL, "r");
  File index = SDPLFS()->open(REAL_INDEX, "r");
  index.seek((num - 1) * 4, SeekSet);
  uint32_t pos;
  memset(_stationBuf, 0, sizeof(_stationBuf));
  index.readBytes((char *) &pos, 4);
  index.close();
  playlist.seek(pos, SeekSet);
  strncpy(_stationBuf, playlist.readStringUntil('\t').c_str(), sizeof(_stationBuf));
  playlist.close();
  return _stationBuf;
}

uint8_t Config::fillPlMenu(int from, uint8_t count, bool fromNextion) {
  int     ls      = from;
  uint8_t c       = 0;
  bool    finded  = false;
  if (playlistLength() == 0) {
    return 0;
  }
  File playlist = SDPLFS()->open(REAL_PLAYL, "r");
  File index = SDPLFS()->open(REAL_INDEX, "r");
  while (true) {
    if (ls < 1) {
      ls++;
      if(!fromNextion) display.printPLitem(c, "");
  #ifdef USE_NEXTION
    if(fromNextion) nextion.printPLitem(c, "");
  #endif
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
    bool pla = true;
    while (pla) {
      pla = playlist.available();
      String stationName = playlist.readStringUntil('\n');
      stationName = stationName.substring(0, stationName.indexOf('\t'));
      if(config.store.numplaylist && stationName.length()>0) stationName = String(from+c)+" "+stationName;
      if(!fromNextion) display.printPLitem(c, stationName.c_str());
      #ifdef USE_NEXTION
        if(fromNextion) nextion.printPLitem(c, stationName.c_str());
      #endif
      c++;
      if (c >= count) break;
    }
    break;
  }
  playlist.close();
  return c;
}

void Config::escapeQuotes(const char* input, char* output, size_t maxLen) {
  size_t j = 0;
  for (size_t i = 0; input[i] != '\0' && j < maxLen - 1; ++i) {
    if (input[i] == '"' && j < maxLen - 2) {
      output[j++] = '\\';
      output[j++] = '"';
    } else {
      output[j++] = input[i];
    }
  }
  output[j] = '\0';
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

bool Config::parseCSVimport(const char* line, char* name, char* url, int &ovol) {
  // Reset outputs
  if (name) name[0] = 0;
  if (url) url[0] = 0;
  ovol = 0;

  // Copy line to a buffer for tokenization
  char buf[BUFLEN * 2];
  strncpy(buf, line, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;

  // Detect delimiter: prefer tab, then space
  char delim = 0;
  if (strchr(buf, '\t')) delim = '\t';
  else delim = ' ';

  // Tokenize by detected delimiter only
  char* tokens[32];
  int t = 0;
  char* p = strtok(buf, (delim == '\t') ? "\t" : " ");
  while (p && t < 32) {
    tokens[t++] = p;
    p = strtok(nullptr, (delim == '\t') ? "\t" : " ");
  }

  // --- TAB-DELIMITED LOGIC ---
  if (delim == '\t') {
    if (t == 1) {
      // 1 field: URL only
      if (strstr(tokens[0], ".") && (strstr(tokens[0], "/") || strstr(tokens[0], "://"))) {
        if (url) {
          if (strncmp(tokens[0], "http://", 7) != 0 && strncmp(tokens[0], "https://", 8) != 0) {
            snprintf(url, BUFLEN, "http://%s", tokens[0]);
          } else {
            strlcpy(url, tokens[0], BUFLEN);
          }
        }
        if (name) {
          const char* u = url;
          if (strncmp(u, "http://", 7) == 0) u += 7;
          else if (strncmp(u, "https://", 8) == 0) u += 8;
          strlcpy(name, u, BUFLEN);
          // Sanitize '/' to ' '
          for (char* p = name; *p; ++p) if (*p == '/') *p = ' ';
        }
        ovol = 0;
        return true;
      } else {
        return false;
      }
    } else if (t == 2) {
      // 2 fields: one is URL, one is name (order does not matter)
      int urlIdx = -1, nameIdx = -1;
      for (int i = 0; i < 2; ++i) {
        if (strstr(tokens[i], ".") && (strstr(tokens[i], "/") || strstr(tokens[i], "://"))) urlIdx = i;
        else nameIdx = i;
      }
      if (urlIdx == -1 || nameIdx == -1) return false;
      if (url) {
        if (strncmp(tokens[urlIdx], "http://", 7) != 0 && strncmp(tokens[urlIdx], "https://", 8) != 0) {
          snprintf(url, BUFLEN, "http://%s", tokens[urlIdx]);
        } else {
          strlcpy(url, tokens[urlIdx], BUFLEN);
        }
      }
      if (name) {
        strlcpy(name, tokens[nameIdx], BUFLEN);
        // Sanitize '/' to ' '
        for (char* p = name; *p; ++p) if (*p == '/') *p = ' ';
      }
      ovol = 0;
      return true;
    } else if (t == 3) {
      // 3 fields: one is URL, one is name, one is ovol (ovol must be integer 0-255)
      int urlIdx = -1, nameIdx = -1, ovolIdx = -1;
      for (int i = 0; i < 3; ++i) {
        if (strstr(tokens[i], ".") && (strstr(tokens[i], "/") || strstr(tokens[i], "://"))) urlIdx = i;
        else {
          char* endptr = nullptr;
          long val = strtol(tokens[i], &endptr, 10);
          if (endptr && *endptr == '\0' && val >= 0 && val <= 255) {
            ovolIdx = i;
            ovol = (int)val;
          } else {
            nameIdx = i;
          }
        }
      }
      if (urlIdx == -1 || nameIdx == -1) return false;
      if (url) {
        if (strncmp(tokens[urlIdx], "http://", 7) != 0 && strncmp(tokens[urlIdx], "https://", 8) != 0) {
          snprintf(url, BUFLEN, "http://%s", tokens[urlIdx]);
        } else {
          strlcpy(url, tokens[urlIdx], BUFLEN);
        }
      }
      if (name) {
        strlcpy(name, tokens[nameIdx], BUFLEN);
        // Sanitize '/' to ' '
        for (char* p = name; *p; ++p) if (*p == '/') *p = ' ';
      }
      if (ovolIdx == -1) ovol = 0;
      return true;
    } else {
      // More than 3 fields: invalid for tab-delimited
      return false;
    }
  }

  // --- SPACE-DELIMITED LOGIC ---
  // Find URL token (must contain dot and slash or ://)
  int urlIdx = -1;
  for (int i = 0; i < t; ++i) {
    if (strstr(tokens[i], ".") && (strstr(tokens[i], "/") || strstr(tokens[i], "://"))) {
      urlIdx = i;
      break;
    }
  }
  if (urlIdx == -1) return false; // URL is required

  // Check for ovol at the end (name url ovol)
  int ovolIdx = -1;
  if (t == urlIdx + 2) {
    char* endptr = nullptr;
    long val = strtol(tokens[t-1], &endptr, 10);
    if (endptr && *endptr == '\0' && val >= 0 && val <= 255) {
      ovolIdx = t-1;
      ovol = (int)val;
    }
  }

  // If URL is at the end
  if (urlIdx == t-1 || (ovolIdx != -1 && urlIdx == t-2)) {
    // name is everything before URL (or before URL and ovol)
    if (name) name[0] = 0;
    int nameEnd = (ovolIdx != -1) ? urlIdx : t-1;
    for (int i = 0; i < nameEnd; ++i) {
      if (name && tokens[i][0]) {
        if (strlen(name) > 0) strlcat(name, " ", BUFLEN);
        strlcat(name, tokens[i], BUFLEN);
      }
    }
    // URL
    if (url) {
      if (strncmp(tokens[urlIdx], "http://", 7) != 0 && strncmp(tokens[urlIdx], "https://", 8) != 0) {
        snprintf(url, BUFLEN, "http://%s", tokens[urlIdx]);
      } else {
        strlcpy(url, tokens[urlIdx], BUFLEN);
      }
    }
    if (ovolIdx == -1) ovol = 0;
    // If name is missing or empty, use url (minus protocol) as name
    if ((name == nullptr || strlen(name) == 0) && url && strlen(url) > 0) {
      const char* u = url;
      if (strncmp(u, "http://", 7) == 0) u += 7;
      else if (strncmp(u, "https://", 8) == 0) u += 8;
      if (name) {
        strlcpy(name, u, BUFLEN);
        // Sanitize '/' to ' '
        for (char* p = name; *p; ++p) if (*p == '/') *p = ' ';
      }
    }
    if (!url || strlen(url) == 0) return false;
    return true;
  }
  // If URL is at the beginning
  if (urlIdx == 0) {
    // name is everything after URL (and before ovol if present)
    if (name) name[0] = 0;
    int nameStart = 1;
    int nameEnd = (ovolIdx != -1) ? ovolIdx : t;
    for (int i = nameStart; i < nameEnd; ++i) {
      if (name && tokens[i][0]) {
        if (strlen(name) > 0) strlcat(name, " ", BUFLEN);
        strlcat(name, tokens[i], BUFLEN);
      }
    }
    // URL
    if (url) {
      if (strncmp(tokens[0], "http://", 7) != 0 && strncmp(tokens[0], "https://", 8) != 0) {
        snprintf(url, BUFLEN, "http://%s", tokens[0]);
      } else {
        strlcpy(url, tokens[0], BUFLEN);
      }
    }
    if (ovolIdx == -1) ovol = 0;
    // If name is missing or empty, use url (minus protocol) as name
    if ((name == nullptr || strlen(name) == 0) && url && strlen(url) > 0) {
      const char* u = url;
      if (strncmp(u, "http://", 7) == 0) u += 7;
      else if (strncmp(u, "https://", 8) == 0) u += 8;
      if (name) {
        strlcpy(name, u, BUFLEN);
        // Sanitize '/' to ' '
        for (char* p = name; *p; ++p) if (*p == '/') *p = ' ';
      }
    }
    if (!url || strlen(url) == 0) return false;
    return true;
  }
  // Otherwise, invalid for space-delimited
  return false;
}

bool Config::parseJSON(const char* line, char* name, char* url, int &ovol) {
  // Reset outputs
  if (name) name[0] = 0;
  if (url) url[0] = 0;
  ovol = 0;

  // Helper lambda to extract a value by key (key must be quoted, e.g. "name")
  // Handles both string and numeric values
  auto extract = [](const char* src, const char* key, char* out, size_t outlen) -> bool {
    const char* k = strstr(src, key);
    if (!k) return false;
    k += strlen(key);
    // Skip whitespace and colon
    while (*k && (*k == ' ' || *k == '\t')) k++;
    if (*k != ':') return false;
    k++;
    while (*k && (*k == ' ' || *k == '\t')) k++;
    if (*k == '"') {
      // String value
      k++;
      const char* end = strchr(k, '"');
      if (!end) return false;
      size_t len = end - k;
      if (len >= outlen) len = outlen - 1;
      strncpy(out, k, len);
      out[len] = 0;
      return true;
    } else {
      // Numeric value (int, float, etc.)
      const char* end = k;
      while (*end && ((*end >= '0' && *end <= '9') || *end == '-' || *end == '+')) end++;
      size_t len = end - k;
      if (len == 0 || len >= outlen) return false;
      strncpy(out, k, len);
      out[len] = 0;
      return true;
    }
  };

  // If the line starts with '[', treat as JSON array and extract the first object
  const char* obj = line;
  if (line[0] == '[') {
    // Find the first '{' and the matching '}'
    const char* start = strchr(line, '{');
    if (!start) return false;
    int brace = 1;
    const char* end = start + 1;
    while (*end && brace > 0) {
      if (*end == '{') brace++;
      else if (*end == '}') brace--;
      end++;
    }
    if (brace != 0) return false;
    static char objbuf[512];
    size_t len = end - start;
    if (len >= sizeof(objbuf)) len = sizeof(objbuf) - 1;
    strncpy(objbuf, start, len);
    objbuf[len] = 0;
    obj = objbuf;
  }

  char buf[256];
  // 1. Extract name
  if (!extract(obj, "\"name\"", name, BUFLEN)) {
    return false;
  }

  // 2. Try url_resolved, then url, then host+file+port
  bool gotUrl = false;
  if (extract(obj, "\"url_resolved\"", buf, sizeof(buf))) {
    strncpy(url, buf, BUFLEN);
    gotUrl = true;
  } else if (extract(obj, "\"url\"", buf, sizeof(buf))) {
    strncpy(url, buf, BUFLEN);
    gotUrl = true;
  } else {
    char host[246] = {0}, file[254] = {0}, port[16] = {0};
    bool gotHost = extract(obj, "\"host\"", host, sizeof(host));
    bool gotFile = extract(obj, "\"file\"", file, sizeof(file));
    bool gotPort = extract(obj, "\"port\"", port, sizeof(port));
    if (gotHost && gotFile) {
      if (strstr(host, "http://") == NULL && strstr(host, "https://") == NULL) {
        snprintf(buf, sizeof(buf), "http://%s", host);
        strlcpy(host, buf, sizeof(host));
      }
      if (gotPort && strlen(port) > 0) {
        snprintf(url, BUFLEN, "%s:%s%s", host, port, file);
      } else {
        snprintf(url, BUFLEN, "%s%s", host, file);
      }
      gotUrl = true;
    }
  }
  if (!gotUrl || strlen(url) == 0) {
    return false;
  }

  // 3. Try ovol, default to 0 if not found
  char ovolbuf[16] = {0};
  if (extract(obj, "\"ovol\"", ovolbuf, sizeof(ovolbuf))) {
    ovol = atoi(ovolbuf);
  } else {
    ovol = 0;
  }
  return true;
}

bool Config::parseWsCommand(const char* line, char* cmd, char* val, uint8_t cSize) {
  char *tmpe;
  tmpe = strstr(line, "=");
  if (tmpe == NULL) return false;
  memset(cmd, 0, cSize);
  strlcpy(cmd, line, tmpe - line + 1);
  //if (strlen(tmpe + 1) == 0) return false;
  memset(val, 0, cSize);
  strlcpy(val, tmpe + 1, strlen(line) - strlen(cmd) + 1);
  return true;
}

bool Config::parseSsid(const char* line, char* ssid, char* pass) {
  char *tmpe;
  tmpe = strstr(line, "\t");
  if (tmpe == NULL) return false;
  uint16_t pos = tmpe - line;
  if (pos > 29 || strlen(line) > 71) return false;
  memset(ssid, 0, 30);
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

void Config::setTimeConf(){
  if(strlen(store.sntp1)>0 && strlen(store.sntp2)>0){
    configTzTime(store.tzposix, store.sntp1, store.sntp2);
  }else if(strlen(store.sntp1)>0){
    configTzTime(store.tzposix, store.sntp1);
  }
}

bool Config::initNetwork() {
  File file = SPIFFS.open(SSIDS_PATH, "r");
  if (!file || file.isDirectory()) {
    return false;
  }
  char ssidval[30], passval[40];
  uint8_t c = 0;
  while (file.available()) {
    if (parseSsid(file.readStringUntil('\n').c_str(), ssidval, passval)) {
      strlcpy(ssids[c].ssid, ssidval, 30);
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
  analogWrite(BRIGHTNESS_PIN, map(store.brightness, 0, 100, 0, 255));
  if(!store.dspon) store.dspon = true;
  if(dosave){
    saveValue(&store.brightness, store.brightness, false, true);
    saveValue(&store.dspon, store.dspon, true, true);
  }
#endif
#ifdef USE_NEXTION
  nextion.wake();
  char cmd[15];
  snprintf(cmd, 15, "dims=%d", store.brightness);
  nextion.putcmd(cmd);
  if(!store.dspon) store.dspon = true;
  if(dosave){
    saveValue(&store.brightness, store.brightness, false, true);
    saveValue(&store.dspon, store.dspon, true, true);
  }
#endif
}

void Config::setDspOn(bool dspon, bool saveval){
  if(saveval){
    store.dspon = dspon;
    saveValue(&store.dspon, store.dspon, true, true);
  }
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
#if !defined(ARDUINO_ESP32C3_DEV)
  if(WAKE_PIN!=255) esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, LOW);
  esp_sleep_enable_timer_wakeup(config.sleepfor * 60 * 1000000ULL);
  esp_deep_sleep_start();
#endif
}

void Config::doSleepW(){
  if(BRIGHTNESS_PIN!=255) analogWrite(BRIGHTNESS_PIN, 0);
  display.deepsleep();
#ifdef USE_NEXTION
  nextion.sleep();
#endif
#if !defined(ARDUINO_ESP32C3_DEV)
  if(WAKE_PIN!=255) esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, LOW);
  esp_deep_sleep_start();
#endif
}

void Config::sleepForAfter(uint16_t sf, uint16_t sa){
  sleepfor = sf;
  if(sa > 0) timekeeper.waitAndDo(sa * 60, doSleep);
  else doSleep();
}

void cleanStaleSearchResults() {
  const char* metaPath = "/data/searchresults.json.meta";
  if (SPIFFS.exists(metaPath)) {
    File metaFile = SPIFFS.open(metaPath, "r");
    metaFile.readStringUntil('\n'); // 1st line query
    String timeStr = metaFile.readStringUntil('\n'); //2nd line is time
    metaFile.close();
    if (timeStr.length() > 0) {
      time_t fileTime = atol(timeStr.c_str());
      time_t now = time(nullptr);
      if (now < 100000000 || (now - fileTime) > 86400) {
        Serial.print("Cleaning stale search results.\n");
        SPIFFS.remove(metaPath);
        SPIFFS.remove("/data/searchresults.json");
        SPIFFS.remove("/data/search.txt");
      }
    }
  }
}

void fixPlaylistFileEnding() {
  const char* playlistPath = PLAYLIST_PATH;
  if (!SPIFFS.exists(playlistPath)) return;
  File playlistfile = SPIFFS.open(playlistPath, "r+");
  if (!playlistfile) return;
  size_t sz = playlistfile.size();
  if (sz < 2) { playlistfile.close(); return; }
  playlistfile.seek(sz - 2, SeekSet);
  char last2[3] = {0};
  playlistfile.read((uint8_t*)last2, 2);
  if (!(last2[0] == '\r' && last2[1] == '\n')) {
    playlistfile.seek(sz, SeekSet);
    playlistfile.write((const uint8_t*)"\r\n", 2);
  }
  playlistfile.close();
}

void updateFile(void* param, const char* localFile, const char* onlineFile, const char* updatePeriod, const char* simpleName) {
  char startMsg[128];
  snprintf(startMsg, sizeof(startMsg), "[ESPFileUpdater: %s] Started update.", simpleName);
  Serial.println(startMsg);
  ESPFileUpdater* updater = (ESPFileUpdater*)param;
  ESPFileUpdater::UpdateStatus result = updater->checkAndUpdate(
      localFile,
      onlineFile,
      updatePeriod,
      ESPFILEUPDATER_VERBOSE
  );
  if (result == ESPFileUpdater::UPDATED) {
    Serial.printf("[ESPFileUpdater: %s] Update completed.\n", simpleName);
  } else if (result == ESPFileUpdater::NOT_MODIFIED||result == ESPFileUpdater::MAX_AGE_NOT_REACHED) {
    Serial.printf("[ESPFileUpdater: %s] No update needed.\n", simpleName);
  } else {
    Serial.printf("[ESPFileUpdater: %s] Update failed.\n", simpleName);
  }
}

#ifdef UPDATEURL
  void getRequiredFiles(void* param) {
    for (size_t i = 0; i < requiredFilesCount; i++) {
      player.sendCommand({PR_STOP, 0});
      display.putRequest(NEWMODE, UPDATING);
      const char* fname = requiredFiles[i];
      char localPath[64];
      char remoteUrl[128];
      snprintf(localPath, sizeof(localPath), "/www/%s", fname);
      snprintf(remoteUrl, sizeof(remoteUrl), "%s%s", UPDATEURL, fname);
      updateFile(param, localPath, remoteUrl, "", fname);
    }
    // Delete any files in /www that are not in the requiredFiles list
    File root = SPIFFS.open("/www");
    if (root && root.isDirectory()) {
      File file = root.openNextFile();
      while (file) {
        const char* path = file.name();
        // Extract filename from full path
        const char* name = path;
        const char* slash = strrchr(path, '/');
        if (slash) name = slash + 1;
        bool found = false;
        for (size_t j = 0; j < requiredFilesCount; j++) {
          if (strcmp(name, requiredFiles[j]) == 0) {
            found = true;
            break;
          }
        }
        if (!found) {
          Serial.printf("[File: /www/%s] Deleting - not in required file list.\n", path);
          SPIFFS.remove(path);
        }
        file = root.openNextFile();
      }
    }
  }
#endif //#ifdef UPDATEURL

void startAsyncServices(void* param){
  fixPlaylistFileEnding();
  // if the OTA marker file exists, fetch all web assets immediately, clean up, restart
 #ifdef UPDATEURL
    if (SPIFFS.exists(ONLINEUPDATE_MARKERFILE)) {
      getRequiredFiles(param);
      SPIFFS.remove(ONLINEUPDATE_MARKERFILE);
      delay(200);
      ESP.restart();
    }
  #endif
  updateFile(param, "/www/timezones.json.gz", TIMEZONES_JSON_URL, "1 week", "Timezones database file");
  updateFile(param, "/www/rb_srvrs.json", RADIO_BROWSER_SERVERS_URL, "4 weeks", "Radio Browser Servers list");
  cleanStaleSearchResults();
  vTaskDelete(NULL);
}

void Config::startAsyncServicesButWait() {
  if (WiFi.status() != WL_CONNECTED) return;
  ESPFileUpdater* updater = nullptr;
  updater = new ESPFileUpdater(SPIFFS);
  updater->setMaxSize(1024);
  updater->setUserAgent(ESPFILEUPDATER_USERAGENT);
  xTaskCreate(startAsyncServices, "startAsyncServices", 8192, updater, 2, NULL);
}

void Config::bootInfo() {
  BOOTLOG("************************************************");
  BOOTLOG("*               ёRadio v%s v%s                *", YOVERSION);
  BOOTLOG("************************************************");
  BOOTLOG("------------------------------------------------");
  BOOTLOG("arduino:\t%d", ARDUINO);
  BOOTLOG("compiler:\t%s", __VERSION__);
  BOOTLOG("esp32core:\t%d.%d.%d", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
  uint32_t chipId = 0;
  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  BOOTLOG("chip:\t\tmodel: %s | rev: %d | id: %d | cores: %d | psram: %d", ESP.getChipModel(), ESP.getChipRevision(), chipId, ESP.getChipCores(), ESP.getPsramSize());
  BOOTLOG("display:\t%d", DSP_MODEL);
  if(VS1053_CS==255) {
    BOOTLOG("audio:\t\t%s (%d, %d, %d)", "I2S", I2S_DOUT, I2S_BCLK, I2S_LRC);
  }else{
    BOOTLOG("audio:\t\t%s (%d, %d, %d, %d, %s)", "VS1053", VS1053_CS, VS1053_DCS, VS1053_DREQ, VS1053_RST, VS_HSPI?"true":"false");
  }
  BOOTLOG("audioinfo:\t%s", store.audioinfo?"true":"false");
  BOOTLOG("smartstart:\t%d", store.smartstart);
  BOOTLOG("vumeter:\t%s", store.vumeter?"true":"false");
  BOOTLOG("softapdelay:\t%d", store.softapdelay);
  BOOTLOG("flipscreen:\t%s", store.flipscreen?"true":"false");
  BOOTLOG("volumepage:\t%s", store.volumepage?"true":"false");
  BOOTLOG("clock12:\t%s", store.clock12?"true":"false");
  BOOTLOG("invertdisplay:\t%s", store.invertdisplay?"true":"false");
  BOOTLOG("showweather:\t%s", store.showweather?"true":"false");
  BOOTLOG("buttons:\tleft=%d, center=%d, right=%d, up=%d, down=%d, mode=%d, pullup=%s", 
          BTN_LEFT, BTN_CENTER, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_MODE, BTN_INTERNALPULLUP?"true":"false");
  BOOTLOG("encoders:\tl1=%d, b1=%d, r1=%d, pullup=%s, l2=%d, b2=%d, r2=%d, pullup=%s", 
          ENC_BTNL, ENC_BTNB, ENC_BTNR, ENC_INTERNALPULLUP?"true":"false", ENC2_BTNL, ENC2_BTNB, ENC2_BTNR, ENC2_INTERNALPULLUP?"true":"false");
  BOOTLOG("ir:\t\t%d", IR_PIN);
  if(SDC_CS!=255) BOOTLOG("SD:\t%d", SDC_CS);
  #ifdef FIRMWARE
    BOOTLOG("firmware:\t%s", FIRMWARE);
  #endif
  BOOTLOG("------------------------------------------------");
}

// Preferences Look-up Table (store_variable, "key_max_15_char")
// Macro expands to 3 fields (offset_of_config_t_store_variable, "key_max_15_char", size_of_store_variable)
const configKeyMap Config::keyMap[] = {
  CONFIG_KEY_ENTRY(config_set, "cfgset"),
  CONFIG_KEY_ENTRY(volume, "vol"),
  CONFIG_KEY_ENTRY(balance, "bal"),
  CONFIG_KEY_ENTRY(trebble, "treb"),
  CONFIG_KEY_ENTRY(middle, "mid"),
  CONFIG_KEY_ENTRY(bass, "bass"),
  CONFIG_KEY_ENTRY(lastStation, "laststa"),
  CONFIG_KEY_ENTRY(countStation, "countsta"),
  CONFIG_KEY_ENTRY(lastSSID, "lastssid"),
  CONFIG_KEY_ENTRY(audioinfo, "audioinfo"),
  CONFIG_KEY_ENTRY(smartstart, "smartstart"),
  CONFIG_KEY_ENTRY(timezoneOffset, "tzoff"),
  CONFIG_KEY_ENTRY(vumeter, "vumeter"),
  CONFIG_KEY_ENTRY(softapdelay, "softapdelay"),
  CONFIG_KEY_ENTRY(flipscreen, "flipscr"),
  CONFIG_KEY_ENTRY(volumepage, "volpage"),
  CONFIG_KEY_ENTRY(clock12, "clock12"),
  CONFIG_KEY_ENTRY(invertdisplay, "invdisp"),
  CONFIG_KEY_ENTRY(numplaylist, "numplaylist"),
  CONFIG_KEY_ENTRY(fliptouch, "fliptouch"),
  CONFIG_KEY_ENTRY(dbgtouch, "dbgtouch"),
  CONFIG_KEY_ENTRY(dspon, "dspon"),
  CONFIG_KEY_ENTRY(brightness, "bright"),
  CONFIG_KEY_ENTRY(contrast, "contrast"),
  CONFIG_KEY_ENTRY(tz_name, "tzname"),
  CONFIG_KEY_ENTRY(tzposix, "tzposix"),
  CONFIG_KEY_ENTRY(sntp1, "sntp1"),
  CONFIG_KEY_ENTRY(sntp2, "sntp2"),
  CONFIG_KEY_ENTRY(showweather, "showwthr"),
  CONFIG_KEY_ENTRY(weatherlat, "weatherlat"),
  CONFIG_KEY_ENTRY(weatherlon, "weatherlon"),
  CONFIG_KEY_ENTRY(weatherkey, "weatherkey"),
  CONFIG_KEY_ENTRY(_reserved, "resv"),
  CONFIG_KEY_ENTRY(lastSdStation, "lastsdsta"),
  CONFIG_KEY_ENTRY(sdsnuffle, "sdsnuffle"),
  CONFIG_KEY_ENTRY(volsteps, "vsteps"),
  CONFIG_KEY_ENTRY(encacc, "encacc"),
  CONFIG_KEY_ENTRY(play_mode, "playmode"),
  CONFIG_KEY_ENTRY(irtlp, "irtlp"),
  CONFIG_KEY_ENTRY(btnpullup, "btnpullup"),
  CONFIG_KEY_ENTRY(btnlongpress, "btnlngpress"),
  CONFIG_KEY_ENTRY(btnclickticks, "btnclkticks"),
  CONFIG_KEY_ENTRY(btnpressticks, "btnprsticks"),
  CONFIG_KEY_ENTRY(encpullup, "encpullup"),
  CONFIG_KEY_ENTRY(enchalf, "enchalf"),
  CONFIG_KEY_ENTRY(enc2pullup, "enc2pullup"),
  CONFIG_KEY_ENTRY(enc2half, "enc2half"),
  CONFIG_KEY_ENTRY(forcemono, "forcemono"),
  CONFIG_KEY_ENTRY(i2sinternal, "i2sint"),
  CONFIG_KEY_ENTRY(rotate90, "rotate"),
  CONFIG_KEY_ENTRY(screensaverEnabled, "scrnsvren"),
  CONFIG_KEY_ENTRY(screensaverTimeout, "scrnsvrto"),
  CONFIG_KEY_ENTRY(screensaverBlank, "scrnsvrbl"),
  CONFIG_KEY_ENTRY(screensaverPlayingEnabled, "scrnsvrplen"),
  CONFIG_KEY_ENTRY(screensaverPlayingTimeout, "scrnsvrplto"),
  CONFIG_KEY_ENTRY(screensaverPlayingBlank, "scrnsvrplbl"),
  CONFIG_KEY_ENTRY(mdnsname, "mdnsname"),
  CONFIG_KEY_ENTRY(skipPlaylistUpDown, "skipplupdn"),
  CONFIG_KEY_ENTRY(abuff, "abuff"),
  CONFIG_KEY_ENTRY(telnet, "telnet"),
  CONFIG_KEY_ENTRY(watchdog, "watchdog"),
  CONFIG_KEY_ENTRY(timeSyncInterval, "tsyncint"),
  CONFIG_KEY_ENTRY(timeSyncIntervalRTC, "tsyncintrtc"),
  CONFIG_KEY_ENTRY(weatherSyncInterval, "wsyncint"),
  {0, nullptr, 0} // Yup, 3 fields - don't delete the last line!
};

void Config::deleteOldKeys() {
  // List any old/legacy keys to remove here
  // prefs.remove("removedkey");
  // prefs.remove("removedkey");
}
