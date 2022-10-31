#include "options.h"

#include "WiFi.h"
#include "time.h"
#include "display.h"
#include "player.h"
#include "network.h"


Display display;
#ifdef USE_NEXTION
Nextion nextion;
#endif

#ifndef DUMMYDISPLAY
//============================================================================================================================
DspCore dsp;

Page *pages[] = { new Page(), new Page(), new Page() };

#ifndef CORE_STACK_SIZE
  #define CORE_STACK_SIZE  1024*3
#endif
#ifndef DSP_TASK_DELAY
  #define DSP_TASK_DELAY  2
#endif

TaskHandle_t DspTask;
QueueHandle_t displayQueue;

void returnPlayer(){
  display.putRequest(NEWMODE, PLAYER);
}

void Display::_createDspTask(){
  xTaskCreatePinnedToCore(loopDspTask, "DspTask", CORE_STACK_SIZE,  NULL,  4, &DspTask, !xPortGetCoreID());
}

void loopDspTask(void * pvParameters){
  while(true){
    if(displayQueue==NULL) break;
    display.loop();
    vTaskDelay(DSP_TASK_DELAY);
  }
  vTaskDelete( NULL );
  DspTask=NULL;
}

void Display::init() {
#ifdef USE_NEXTION
  nextion.begin();
#endif
#if LIGHT_SENSOR!=255
  analogSetAttenuation(ADC_0db);
#endif
  _bootStep = 0;
  dsp.initDisplay();
  displayQueue=NULL;
  displayQueue = xQueueCreate( 5, sizeof( requestParams_t ) );
  while(displayQueue==NULL){;}
  _createDspTask();
  while(!_bootStep==0) { delay(10); }
  //_pager.begin();
  //_bootScreen();
}

void Display::_bootScreen(){
  _boot = new Page();
  _boot->addWidget(new ProgressWidget(bootWdtConf, bootPrgConf, BOOT_PRG_COLOR, 0));
  _bootstring = (TextWidget*) &_boot->addWidget(new TextWidget(bootstrConf, 50, true, BOOT_TXT_COLOR, 0));
  _pager.addPage(_boot);
  _pager.setPage(_boot, true);
  dsp.drawLogo(bootLogoTop);
  _bootStep = 1;
}

void Display::_buildPager(){
  _meta.init("*", metaConf, config.theme.meta, config.theme.metabg);
  _title1.init("*", title1Conf, config.theme.title1, config.theme.background);
  _clock.init(clockConf, 0, 0);
  #if DSP_MODEL==DSP_NOKIA5110
    _plcurrent.init("*", playlistConf, 0, 1);
  #else
    _plcurrent.init("*", playlistConf, config.theme.plcurrent, config.theme.plcurrentbg);
  #endif
  #ifndef HIDE_TITLE2
    _title2 = new ScrollWidget("*", title2Conf, config.theme.title2, config.theme.background);
  #endif
  #if !defined(DSP_LCD) && DSP_MODEL!=DSP_NOKIA5110
    _plbackground = new FillWidget(playlBGConf, config.theme.plcurrentfill);
    _metabackground = new FillWidget(metaBGConf, config.theme.metafill);
  #endif
  #if DSP_MODEL==DSP_NOKIA5110
    _plbackground = new FillWidget(playlBGConf, 1);
    //_metabackground = new FillWidget(metaBGConf, 1);
  #endif
  #ifndef HIDE_VU
    _vuwidget = new VuWidget(vuConf, bandsConf, config.theme.vumax, config.theme.vumin, config.theme.background);
  #endif
  #ifndef HIDE_VOLBAR
    _volbar = new SliderWidget(volbarConf, config.theme.volbarin, config.theme.background, 254, config.theme.volbarout);
  #endif
  #ifndef HIDE_HEAPBAR
    _heapbar = new SliderWidget(heapbarConf, config.theme.buffer, config.theme.background, psramInit()?300000:1600 * AUDIOBUFFER_MULTIPLIER2);
  #endif
  #ifndef HIDE_VOL
    _voltxt = new TextWidget(voltxtConf, 10, false, config.theme.vol, config.theme.background);
  #endif
  #ifndef HIDE_IP
    _volip = new TextWidget(iptxtConf, 30, false, config.theme.ip, config.theme.background);
  #endif
  #ifndef HIDE_RSSI
    _rssi = new TextWidget(rssiConf, 20, false, config.theme.rssi, config.theme.background);
  #endif
  _nums.init(numConf, 10, false, config.theme.digit, config.theme.background);
  #ifndef HIDE_WEATHER
    _weather = new ScrollWidget("*", weatherConf, config.theme.weather, config.theme.background);
  #endif
  
  if(_volbar)   _footer.addWidget( _volbar);
  if(_voltxt)   _footer.addWidget( _voltxt);
  if(_volip)    _footer.addWidget( _volip);
  if(_rssi)     _footer.addWidget( _rssi);
  if(_heapbar)  _footer.addWidget( _heapbar);
  
  if(_metabackground) pages[PG_PLAYER]->addWidget( _metabackground);
  pages[PG_PLAYER]->addWidget(&_meta);
  pages[PG_PLAYER]->addWidget(&_title1);
  if(_title2) pages[PG_PLAYER]->addWidget(_title2);
  if(_weather) pages[PG_PLAYER]->addWidget(_weather);
  #ifdef BITRATE_FULL
    _fullbitrate = new BitrateWidget(fullbitrateConf, config.theme.bitrate, config.theme.background);
    pages[PG_PLAYER]->addWidget( _fullbitrate);
  #else
    _bitrate = new TextWidget(bitrateConf, 30, false, config.theme.bitrate, config.theme.background);
    pages[PG_PLAYER]->addWidget( _bitrate);
  #endif
  if(_vuwidget) pages[PG_PLAYER]->addWidget( _vuwidget);
  pages[PG_PLAYER]->addWidget(&_clock);
  pages[PG_PLAYER]->addPage(&_footer);

  if(_metabackground) pages[PG_DIALOG]->addWidget( _metabackground);
  pages[PG_DIALOG]->addWidget(&_meta);
  pages[PG_DIALOG]->addWidget(&_nums);
  
  #if !defined(DSP_LCD) && DSP_MODEL!=DSP_NOKIA5110
    pages[PG_DIALOG]->addPage(&_footer);
  #endif
  
  if(_plbackground) pages[PG_PLAYLIST]->addWidget( _plbackground);
  pages[PG_PLAYLIST]->addWidget(&_plcurrent);

  for(const auto& p: pages) _pager.addPage(p);
}

void Display::_apScreen() {
  if(_boot) _pager.removePage(_boot);
  #ifndef DSP_LCD
    _boot = new Page();
    #if DSP_MODEL!=DSP_NOKIA5110
    _boot->addWidget(new FillWidget(metaBGConf, config.theme.metafill));
    #endif
    ScrollWidget *bootTitle = (ScrollWidget*) &_boot->addWidget(new ScrollWidget("*", apTitleConf, config.theme.meta, config.theme.metabg));
    bootTitle->setText("ёRadio AP Mode");
    TextWidget *apname = (TextWidget*) &_boot->addWidget(new TextWidget(apNameConf, 30, false, config.theme.title1, config.theme.background));
    apname->setText(apNameTxt);
    TextWidget *apname2 = (TextWidget*) &_boot->addWidget(new TextWidget(apName2Conf, 30, false, config.theme.metabg, config.theme.background));
    apname2->setText(apSsid);
    TextWidget *appass = (TextWidget*) &_boot->addWidget(new TextWidget(apPassConf, 30, false, config.theme.title1, config.theme.background));
    appass->setText(apPassTxt);
    TextWidget *appass2 = (TextWidget*) &_boot->addWidget(new TextWidget(apPass2Conf, 30, false, config.theme.metabg, config.theme.background));
    appass2->setText(apPassword);
    ScrollWidget *bootSett = (ScrollWidget*) &_boot->addWidget(new ScrollWidget("*", apSettConf, config.theme.title2, config.theme.background));
    bootSett->setText(WiFi.softAPIP().toString().c_str(), apSettFmt);
    _pager.addPage(_boot);
    _pager.setPage(_boot);
  #else
    dsp.apScreen();
  #endif
}

void Display::_start() {
  if(_boot) _pager.removePage(_boot);
  #ifdef USE_NEXTION
    nextion.wake();
  #endif
  if (network.status != CONNECTED) {
    _apScreen();
    #ifdef USE_NEXTION
      nextion.apScreen();
    #endif
    _bootStep = 2;
    return;
  }
  #ifdef USE_NEXTION
    nextion.putcmd("page player");
    nextion.start();
  #endif
  _buildPager();
  _mode = PLAYER;
  config.setTitle(const_PlReady);
  
  if(_heapbar)  _heapbar->lock(!config.store.audioinfo);
  
  if(_weather)  _weather->lock(!config.store.showweather);
  if(_weather && config.store.showweather)  _weather->setText(const_getWeather);

  if(_vuwidget) _vuwidget->lock();
  if(_rssi)     _rssi->setText(WiFi.RSSI(), rssiFmt);
  #ifndef HIDE_IP
    if(_volip) _volip->setText(WiFi.localIP().toString().c_str(), iptxtFmt);
  #endif
  _pager.setPage( pages[PG_PLAYER]);
  _volume();
  _station();
  _time(false);
  _bootStep = 2;
}

void Display::_showDialog(const char *title){
  dsp.setScrollId(NULL);
  _pager.setPage( pages[PG_DIALOG]);
  #ifdef META_MOVE
    _meta.moveTo(metaMove);
  #endif
  _meta.setAlign(WA_CENTER);
  _meta.setText(title);
}

void Display::_setReturnTicker(uint8_t time_s){
  _returnTicker.detach();
  _returnTicker.once(time_s, returnPlayer);
}

void Display::_swichMode(displayMode_e newmode) {
  #ifdef USE_NEXTION
    nextion.swichMode(newmode);
  #endif
  if (newmode == _mode || network.status != CONNECTED) return;
  _mode = newmode;
  dsp.setScrollId(NULL);
  if (newmode == PLAYER) {
    numOfNextStation = 0;
    _returnTicker.detach();
    #ifdef META_MOVE
      _meta.moveBack();
    #endif
    _meta.setAlign(WA_LEFT);
    _meta.setText(config.station.name);
    _nums.setText("");
    _pager.setPage( pages[PG_PLAYER]);
  }
  if (newmode == VOL) {
    #ifndef HIDE_IP
      _showDialog(const_DlgVolume);
    #else
      _showDialog(WiFi.localIP().toString().c_str());
    #endif
  }
  if (newmode == LOST)      _showDialog(const_DlgLost);
  if (newmode == UPDATING)  _showDialog(const_DlgUpdate);
  if (newmode == INFO || newmode == SETTINGS || newmode == TIMEZONE || newmode == WIFI) _showDialog(const_DlgNextion);
  if (newmode == NUMBERS) _showDialog("");
  if (newmode == STATIONS) {
    _pager.setPage( pages[PG_PLAYLIST]);
    _plcurrent.setText("");
    currentPlItem = config.store.lastStation;
    _drawPlaylist();
  }
}

void Display::resetQueue(){
  xQueueReset(displayQueue);
}

void Display::_drawPlaylist() {
  char buf[PLMITEMLENGHT];
  dsp.drawPlaylist(currentPlItem, buf);
  _plcurrent.setText(buf);
/*#ifdef USE_NEXTION
  nextion.drawPlaylist(currentPlItem);
#endif*/
  _setReturnTicker(30);
}

void Display::_drawNextStationNum(uint16_t num) {
  char plMenu[1][40];
  char currentItemText[40] = {0};
  config.fillPlMenu(plMenu, num, 1, true);
  strlcpy(currentItemText, plMenu[0], 39);
  _setReturnTicker(30);
  _meta.setText(currentItemText);
  _nums.setText(num, "%d");
/*#ifdef USE_NEXTION
  nextion.drawNextStationNum(num);
#endif*/
}

void Display::putRequest(displayRequestType_e type, int payload){
  if(displayQueue==NULL) return;
  requestParams_t request;
  request.type = type;
  request.payload = payload;
  xQueueSend(displayQueue, &request, portMAX_DELAY);
  #ifdef USE_NEXTION
    nextion.putRequest(request);
  #endif
}

void Display::_layoutChange(bool played){
  if(config.store.vumeter){
    if(played){
      if(_vuwidget) _vuwidget->unlock();
      _clock.moveTo(clockMove);
      if(_weather) _weather->moveTo(weatherMoveVU);
    }else{
      if(_vuwidget) if(!_vuwidget->locked()) _vuwidget->lock();
      _clock.moveBack();
      if(_weather) _weather->moveBack();
    }
  }else{
    if(played){
      if(_weather) _weather->moveTo(weatherMove);
      _clock.moveBack();
    }else{
      if(_weather) _weather->moveBack();
      _clock.moveBack();
    }
  }
}
    
void Display::loop() {
  if(_bootStep==0) {
    _pager.begin();
    _bootScreen();
    return;
  }
  if(displayQueue==NULL) return;
  _pager.loop();
#ifdef USE_NEXTION
  nextion.loop();
#endif
  requestParams_t request;
  if(xQueueReceive(displayQueue, &request, 20)){
    switch (request.type){
      case NEWMODE: _swichMode((displayMode_e)request.payload); break;
      case CLOCK: 
        if(_mode==PLAYER) _time(); 
        /*#ifdef USE_NEXTION
          if(_mode==TIMEZONE) nextion.localTime(network.timeinfo);
          if(_mode==INFO)     nextion.rssi();
        #endif*/
        break;
      case NEWTITLE: _title(); break;
      case NEWSTATION: _station(); break;
      case NEXTSTATION: _drawNextStationNum(request.payload); break;
      case DRAWPLAYLIST: _drawPlaylist(); break;
      case DRAWVOL: _volume(); break;
      case DBITRATE: {
          char buf[20]; 
          snprintf(buf, 20, bitrateFmt, config.station.bitrate); 
          if(_bitrate) { _bitrate->setText(config.station.bitrate==0?"":buf); } 
          if(_fullbitrate) { 
            _fullbitrate->setBitrate(config.station.bitrate); 
            _fullbitrate->setFormat(BF_MP3); 
          } 
        }
        break;
      case AUDIOINFO: if(_heapbar)  { _heapbar->lock(!config.store.audioinfo); _heapbar->setValue(player.inBufferFilled()); } break;
      case SHOWVUMETER: {
        if(_vuwidget){
          _vuwidget->lock(!config.store.vumeter); 
          _layoutChange(player.isRunning());
        }
        break;
      }
      case SHOWWEATHER: {
        if(_weather) _weather->lock(!config.store.showweather);
        if(!config.store.showweather){
          #ifndef HIDE_IP
          if(_volip) _volip->setText(WiFi.localIP().toString().c_str(), iptxtFmt);
          #endif
        }else{
          if(_weather) _weather->setText(const_getWeather);
        }
        break;
      }
      case NEWWEATHER: {
        if(_weather && network.weatherBuf) _weather->setText(network.weatherBuf);
        break;
      }
      case BOOTSTRING: {
        if(_bootstring) _bootstring->setText(config.ssids[request.payload].ssid, bootstrFmt);
        /*#ifdef USE_NEXTION
          char buf[50];
          snprintf(buf, 50, bootstrFmt, config.ssids[request.payload].ssid);
          nextion.bootString(buf);
        #endif*/
        break;
      }
      case DSPRSSI: if(_rssi){ _rssi->setText(request.payload, rssiFmt); } if (_heapbar && config.store.audioinfo) _heapbar->setValue(player.inBufferFilled()); break;
      case PSTART: _layoutChange(true);   break;
      case PSTOP:  _layoutChange(false);  break;
      case DSP_START: _start();  break;
      default: break;
    }
  }
  dsp.loop();
}

void Display::_station() {
  _meta.setAlign(WA_LEFT);
  _meta.setText(config.station.name);
/*#ifdef USE_NEXTION
  nextion.newNameset(config.station.name);
  nextion.bitrate(config.station.bitrate);
  nextion.bitratePic(ICON_NA);
#endif*/
}

char *split(char *str, const char *delim) {
  char *dmp = strstr(str, delim);
  if (dmp == NULL) return NULL;
  *dmp = '\0'; 
  return dmp + strlen(delim);
}

void Display::_title() {
  if (strlen(config.station.title) > 0) {
    char tmpbuf[strlen(config.station.title)+1];
    strlcpy(tmpbuf, config.station.title, strlen(config.station.title)+1);
    char *stitle = split(tmpbuf, " - ");
    if(stitle && _title2){
      _title1.setText(tmpbuf);
      _title2->setText(stitle);
    }else{
      _title1.setText(config.station.title);
      if(_title2) _title2->setText("");
    }
    /*#ifdef USE_NEXTION
      nextion.newTitle(config.station.title);
    #endif*/
    if (player_on_track_change) player_on_track_change();
  }
}

void Display::_time(bool redraw) {
#if LIGHT_SENSOR!=255
  config.store.brightness = AUTOBACKLIGHT(analogRead(LIGHT_SENSOR));
  config.setBrightness();
#endif
  _clock.draw();
  /*#ifdef USE_NEXTION
    nextion.printClock(network.timeinfo);
  #endif*/
}

void Display::_volume() {
  if(_volbar) _volbar->setValue(config.store.volume);
  #ifndef HIDE_VOL
    if(_voltxt) _voltxt->setText(config.store.volume, voltxtFmt);
  #endif
  if(_mode==VOL) {
    _setReturnTicker(3);
    _nums.setText(config.store.volume, numtxtFmt);
  }
  /*#ifdef USE_NEXTION
    nextion.setVol(config.store.volume, _mode == VOL);
  #endif*/
}

void Display::flip(){ dsp.flip(); }

void Display::invert(){ dsp.invert(); }

void  Display::setContrast(){
  #if DSP_MODEL==DSP_NOKIA5110
    dsp.setContrast(config.store.contrast);
  #endif
}

bool Display::deepsleep(){
#if defined(LCD_I2C) || defined(DSP_OLED) || BRIGHTNESS_PIN!=255
  dsp.sleep();
  return true;
#endif
  return false;
}

void Display::wakeup(){
#if defined(LCD_I2C) || defined(DSP_OLED) || BRIGHTNESS_PIN!=255
  dsp.wake();
#endif
}
//============================================================================================================================
#else // !DUMMYDISPLAY
//============================================================================================================================
void Display::init(){
  #ifdef USE_NEXTION
  nextion.begin(true);
  #endif
}
void Display::_start(){
  #ifdef USE_NEXTION
  nextion.start();
  #endif
}
void Display::putRequest(displayRequestType_e type, int payload){
  if(type==DSP_START) _start();
  #ifdef USE_NEXTION
  requestParams_t request;
  request.type = type;
  request.payload = payload;
  nextion.putRequest(request);
  #endif
}
//============================================================================================================================
#endif // DUMMYDISPLAY
