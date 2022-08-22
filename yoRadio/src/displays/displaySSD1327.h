#ifndef displaySSD1327_h
#define displaySSD1327_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1327.h>
#include "fonts/DS_DIGI28pt7b.h"

#define WEATHER_READY   1
#define DSP_CAN_SLEEP   true

#define TFT_LINEHGHT    10
#define TFT_FRAMEWDT    4

#define PLMITEMS        7
#define PLMITEMLENGHT   40
#define PLMITEMHEIGHT   22
#define TITLE_TOP2 TFT_FRAMEWDT + 3 * TFT_LINEHGHT

#if !defined(SCROLLDELTA) || !defined(SCROLLTIME)

#define SCROLLDELTA     2
#define SCROLLTIME      30
#define LOOP_DELAY      33
#endif

#define TFT_FULLTIME    1

class DspCore: public Adafruit_SSD1327 {
  public:
    bool fillSpaces;
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
    void flip();
    void invert();
    void sleep();
    void wake();
  private:
    uint16_t swidth, sheight;
    int16_t x, y;
    uint16_t cwidth, cheight;
    unsigned long loopdelay;
    char oldTimeBuf[20];
    uint16_t wot, hot, dot;
    boolean checkdelay(int m, unsigned long &tstamp);
    void setClockBounds();
    byte getPw(uint16_t ncwidth);
};

extern DspCore dsp;

      /*
      SSD1327_GRAYTABLE,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
      0x07, 0x08, 0x10, 0x18, 0x20, 0x2f, 0x38, 0x3f,
      */

/*
 * TFT COLORS
 */
#define DARK_GRAY   0x01
#define SILVER      0x07
#define TFT_BG      0x00
#define TFT_FG      0x08
#define TFT_LOGO    0x3f
#define ORANGE      0x02
#define PINK        0x02

#endif
