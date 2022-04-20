#ifndef display_h
#define display_h
#include "options.h"

#include "Arduino.h"
#include <Ticker.h>
#include "config.h"

#if DSP_MODEL==DSP_DUMMY
#include "src/displays/displayDummy.h"
#elif DSP_MODEL==DSP_ST7735
#include "src/displays/displayST7735.h"
#elif DSP_MODEL==DSP_SSD1306 || DSP_MODEL==DSP_SSD1306x32
#include "src/displays/displaySSD1306.h"
#elif DSP_MODEL==DSP_NOKIA5110
#include "src/displays/displayN5110.h"
#elif DSP_MODEL==DSP_ST7789
#include "src/displays/displayST7789.h"
#elif DSP_MODEL==DSP_SH1106
#include "src/displays/displaySH1106.h"
#elif DSP_MODEL==DSP_1602I2C
#include "src/displays/displayLC1602.h"
#elif DSP_MODEL==DSP_SSD1327
#include "src/displays/displaySSD1327.h"
#elif DSP_MODEL==DSP_ILI9341
#include "src/displays/displayILI9341.h"
#elif DSP_MODEL==DSP_SSD1305
#include "src/displays/displaySSD1305.h"
#elif DSP_MODEL==DSP_SH1107
#include "src/displays/displaySH1106.h"
#elif DSP_MODEL==DSP_1602
#include "src/displays/displayLC1602.h"
#elif DSP_MODEL==DSP_GC9106
#include "src/displays/displayGC9106.h"
#elif DSP_MODEL==DSP_CUSTOM
#include "src/displays/displayCustom.h"
#endif

enum displayMode_e { PLAYER, VOL, STATIONS, NUMBERS, LOST, UPDATING };

enum displayRequestType_e { NEWMODE, CLOCK, NEWTITLE, RETURNTITLE, NEWSTATION, NEXTSTATION, DRAWPLAYLIST, DRAWVOL };
struct requestParams_t
{
  displayRequestType_e type;
  int payload;
};

#ifndef DUMMYDISPLAY
void loopCore0( void * pvParameters );

class Scroll {
  public:
    Scroll() { };
    void init(byte ScrollId, const char *sep, byte tsize, byte top, uint16_t dlay, uint16_t fgcolor, uint16_t bgcolor);
    void setText(const char *txt);
    void loop();
    void reset();
    void lock();
    void unlock();
    bool lockRequest;
  private:
    byte textsize, texttop, id;
    char text[BUFLEN/2];
    char separator[4];
    uint16_t fg, bg;
    uint16_t delayStartScroll;
    uint16_t textwidth, textheight, sepwidth, startticks, scrollticks;
    int x;
    bool doscroll, locked;
    unsigned long scrolldelay;
    void clearscrolls();
    void getbounds(uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth);
    boolean checkdelay(int m, unsigned long &tstamp);
    void scroll();
    void sticks();
    void clear();
    void setTextParams();
    void drawFrame();
};
#endif

class Display {
  public:
    uint16_t screenwidth, screenheight;
    displayMode_e mode;
    uint16_t currentPlItem;
    uint16_t numOfNextStation;
#ifndef DUMMYDISPLAY
    Scroll plCurrent;
#endif
    bool busy;
  public:
    Display() {};
#ifndef DUMMYDISPLAY
    void init();
    void loop();
    void start(bool reboot=false);
    void stop();
    void resetQueue();
    void centerText(const char* text, byte y, uint16_t fg, uint16_t bg);
    void rightText(const char* text, byte y, uint16_t fg, uint16_t bg);
    void bootString(const char* text, byte y);
    void bootLogo();
    void putRequest(requestParams_t request);
#else
    void init(){};
    void loop(){};
    void start(bool reboot=false){};
    void stop(){};
    void resetQueue(){};
    void centerText(const char* text, byte y, uint16_t fg, uint16_t bg){};
    void rightText(const char* text, byte y, uint16_t fg, uint16_t bg){};
    void bootString(const char* text, byte y){};
    void bootLogo(){};
    void putRequest(requestParams_t request){};
#endif
#ifndef DUMMYDISPLAY
  private:
    Ticker timer;
    Scroll meta, title1, title2;
    bool clockRequest;
    bool dt;              // dots
    unsigned long volDelay;
    void clear();
    void heap();
    void rssi();
    void ip();
    void time(bool redraw = false);
    void apScreen();
    void drawPlayer();
    void drawVolume();
    void swichMode(displayMode_e newmode);
    void drawPlaylist();
    void volume();
    void title();
    void station();
    void drawNextStationNum(uint16_t num);
    void returnTile();
    void createCore0Task();
#endif
};

extern Display display;

extern __attribute__((weak)) bool dsp_before_clock(DspCore *dsp, bool dots);
extern __attribute__((weak)) void dsp_after_clock(DspCore *dsp, bool dots);
extern __attribute__((weak)) void dsp_on_init();
extern __attribute__((weak)) void dsp_on_loop(DspCore *dsp);
extern __attribute__((weak)) void dsp_on_start(DspCore *dsp);
extern __attribute__((weak)) void dsp_on_newmode(displayMode_e newmode);

#endif
