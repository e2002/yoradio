#ifndef displayST7735_h
#define displayST7735_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "fonts/DS_DIGI28pt7b.h"

#define VU_READY        1
#define WEATHER_READY   1
#define DSP_CAN_SLEEP   true

#define TFT_LINEHGHT    10
#if DTYPE==INITR_MINI160x80
#define TFT_FRAMEWDT    0
#define DSP_MINI
#else
#define TFT_FRAMEWDT    4
#endif

#define PLMITEMS        7
#define PLMITEMLENGHT   40
#if DTYPE==INITR_MINI160x80
#define PLMITEMHEIGHT   19
#else
#define PLMITEMHEIGHT   21
#endif
#define TITLE_TOP2 TFT_FRAMEWDT + 3 * TFT_LINEHGHT

#if !defined(SCROLLDELTA) || !defined(SCROLLTIME)
#define SCROLLDELTA 2
#define SCROLLTIME 30
#endif

#define TFT_FULLTIME    1

#if DTYPE==INITR_MINI160x80
#define TITLE_SIZE2     0
#define TITLE_TOP1 TFT_FRAMEWDT + 2 * TFT_LINEHGHT-3
#define BOOTSTR_TOP1 50
#define BOOTSTR_TOP2 65
#endif

class DspCore: public Adafruit_ST7735 {
  public:
    DspCore();
    char plMenu[PLMITEMS][PLMITEMLENGHT];
    uint16_t clockY;
    void initD(uint16_t &screenwidth, uint16_t &screenheight);
    void apScreen();
    void drawLogo();
    void clearDsp();
    void centerText(const char* text, byte y, uint16_t fg, uint16_t bg);
    void rightText(const char* text, byte y, uint16_t fg, uint16_t bg);
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
    uint16_t wot, hot, dot;
    int16_t x, y;
    uint16_t cwidth, cheight;
    void setClockBounds();
    byte getPw(uint16_t ncwidth);
};

extern DspCore dsp;

#endif
