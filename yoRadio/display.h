#ifndef display_h
#define display_h

#include "Arduino.h"
#include <Ticker.h>
#include "config.h"

enum displayMode_e { PLAYER, VOL, STATIONS, NUMBERS };

class Scroll {
  public:
    Scroll() { };
    void init(const char *sep, byte tsize, byte top, uint16_t dlay, uint16_t fgcolor, uint16_t bgcolor);
    void setText(const char *txt);
    void loop();
    void reset();
    void lock();
    void unlock();
  private:
    byte textsize, texttop;
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

class Display {
  public:
    bool clockRequest;
    uint16_t screenwidth, screenheight;
    displayMode_e mode;
    uint16_t currentPlItem;
    uint16_t numOfNextStation;
  public:
    Display() {};
    void init();
    void clear();
    void loop();
    void start();
    void centerText(const char* text, byte y, uint16_t fg, uint16_t bg);
    void rightText(const char* text, byte y, uint16_t fg, uint16_t bg);
    void bootString(const char* text, byte y);
    void bootLogo();
    void station();
    void title(const char *str);
    void returnTile();
    void time(bool redraw = false);
    void volume();
    void ip();
    void swichMode(displayMode_e newmode);
    void drawPlaylist();
    void drawNextStationNum(uint16_t num);
  private:
    Ticker timer;
    Scroll meta, title1, title2, plCurrent;
    bool dt;              // dots
    unsigned long volDelay;
    void heap();
    void rssi();
    void apScreen();
    void drawPlayer();
    void drawVolume();
    
};

extern Display display;

#endif
