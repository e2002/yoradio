#ifndef displayST7789_h
#define displayST7789_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
// https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#include "fonts/DS_DIGI42pt7b.h"

#define VU_READY        1
#define WEATHER_READY   1
#define DSP_CAN_SLEEP   true

#define TFT_LINEHGHT    10
#define TFT_FRAMEWDT    8
#define META_SIZE       3
#define TITLE_SIZE1     2
#define TITLE_SIZE2     2

#if !defined(SCROLLDELTA) || !defined(SCROLLTIME)
#define SCROLLDELTA 4
#define SCROLLTIME 30
#endif

#define PLMITEMS        11
#define PLMITEMLENGHT   40
#define PLMITEMHEIGHT   22
#define TFT_FULLTIME    1

#define TITLE_TOP1 TFT_FRAMEWDT + META_SIZE * TFT_LINEHGHT + 8
#define TITLE_TOP2 TFT_FRAMEWDT + (META_SIZE+2) * TFT_LINEHGHT + 8

class DspCore: public Adafruit_ST7789 {
  public:
    DspCore();
    char plMenu[PLMITEMS][PLMITEMLENGHT];
    uint16_t clockY;
    void initD(uint16_t &screenwidth, uint16_t &screenheight);
    void apScreen();
    void drawLogo();
    void clearDsp();
    void centerText(const char* text, uint16_t y, uint16_t fg, uint16_t bg);
    void rightText(const char* text, uint16_t y, uint16_t fg, uint16_t bg, bool fliprect=false, uint16_t delta = 0);
    void set_TextSize(uint8_t s);
    void set_TextColor(uint16_t fg, uint16_t bg);
    void set_Cursor(int16_t x, int16_t y);
    void printText(const char* txt);
    void printClock(const char* timestr);
    void printClock(struct tm timeinfo, bool dots, bool redraw = false);
    void displayHeapForDebug();
    void drawVolumeBar(bool withNumber);
    void drawNextStationNum(uint16_t num);
    char* utf8Rus(const char* str, bool uppercase);
    void drawScrollFrame(uint16_t texttop, uint16_t textheight, uint16_t bg);
    void getScrolBbounds(const char* text, const char* separator, byte textsize, uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth);
    void clearScroll(uint16_t texttop, uint16_t textheight, uint16_t bg);
    void frameTitle(const char* str);
    void rssi(const char* str);
    void ip(const char* str);
    void drawPlaylist(uint16_t currentItem, char* currentItemText);
    void loop(bool force=false);
    virtual void startWrite(void);
    virtual void endWrite(void);
    void flip();
    void invert();
    void sleep();
    void wake();
  private:
    uint16_t swidth, sheight;
    char oldTimeBuf[20];
    uint16_t wot, hot;

};

extern DspCore dsp;

#endif
