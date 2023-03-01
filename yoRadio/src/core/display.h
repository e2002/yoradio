#ifndef display_h
#define display_h
#include "options.h"

#include "Arduino.h"
#include <Ticker.h>
#include "config.h"

#include "../displays/dspcore.h"

enum displayMode_e { PLAYER, VOL, STATIONS, NUMBERS, LOST, UPDATING, INFO, SETTINGS, TIMEZONE, WIFI, CLEAR, SLEEPING };
enum pages_e      : uint8_t  { PG_PLAYER=0, PG_DIALOG=1, PG_PLAYLIST=2 };
//enum dialogType_e : uint8_t  { DG_NONE=0,   DG_VOLUME=1, DG_LOST=2, DG_UPDATING=3, DG_NEXTION=4 };

enum displayRequestType_e { BOOTSTRING, NEWMODE, CLOCK, NEWTITLE, NEWSTATION, NEXTSTATION, DRAWPLAYLIST, DRAWVOL, DBITRATE, AUDIOINFO, SHOWVUMETER, DSPRSSI, SHOWWEATHER, NEWWEATHER, PSTOP, PSTART, DSP_START };
struct requestParams_t
{
  displayRequestType_e type;
  int payload;
};

#if NEXTION_RX!=255 && NEXTION_TX!=255
  #define USE_NEXTION
  #include "../displays/nextion.h"
#endif

#ifndef DUMMYDISPLAY
  void loopDspTask(void * pvParameters);

class Display {
  public:
    uint16_t currentPlItem;
    uint16_t numOfNextStation;
    displayMode_e _mode;
  public:
    Display() {};
    displayMode_e mode() { return _mode; }
    void mode(displayMode_e m) { _mode=m; }
    void init();
    void loop();
    void _start();
    bool ready() { return _bootStep==2; }
    void resetQueue();
    void putRequest(displayRequestType_e type, int payload=0);
    void flip();
    void invert();
    bool deepsleep();
    void wakeup();
    void setContrast();
  private:
    ScrollWidget _meta, _title1, _plcurrent;
    ScrollWidget *_weather;
    ScrollWidget *_title2;
    BitrateWidget *_fullbitrate;
    FillWidget *_metabackground, *_plbackground;
    SliderWidget *_volbar, *_heapbar;
    Pager _pager;
    Page _footer;
    VuWidget *_vuwidget;
    NumWidget _nums;
    ProgressWidget _testprogress;
    ClockWidget _clock;
    Page *_boot;
    TextWidget *_bootstring, *_volip, *_voltxt, *_rssi, *_bitrate;
    Ticker _returnTicker;
    uint8_t _bootStep;
    void _time(bool redraw = false);
    void _apScreen();
    void _swichMode(displayMode_e newmode);
    void _drawPlaylist();
    void _volume();
    void _title();
    void _station();
    void _drawNextStationNum(uint16_t num);
    void _createDspTask();
    void _showDialog(const char *title);
    void _buildPager();
    void _bootScreen();
    void _setReturnTicker(uint8_t time_s);
    void _layoutChange(bool played);
    void _setRSSI(int rssi);
};

#else

class Display {
  public:
    uint16_t currentPlItem;
    uint16_t numOfNextStation;
    displayMode_e _mode;
  public:
    Display() {};
    displayMode_e mode() { return _mode; }
    void mode(displayMode_e m) { _mode=m; }
    void init();
    void _start();
    void putRequest(displayRequestType_e type, int payload=0);
    void loop(){}
    bool ready() { return true; }
    void resetQueue(){}
    void centerText(const char* text, byte y, uint16_t fg, uint16_t bg){}
    void rightText(const char* text, byte y, uint16_t fg, uint16_t bg){}
    void flip(){}
    void invert(){}
    void setContrast(){}
    bool deepsleep(){return true;}
    void wakeup(){}
};

#endif

extern Display display;


#endif
