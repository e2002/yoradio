#ifndef displayN5110_h
#define displayN5110_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "fonts/TinyFont5.h"
#include "fonts/TinyFont6.h"
#include "fonts/DS_DIGI15pt7b.h"

#define TFT_LINEHGHT    8
#define TFT_FRAMEWDT    0

#define PLMITEMS        7
#define PLMITEMLENGHT   40
#define PLMITEMHEIGHT   10

class DisplayN5110: public Adafruit_PCD8544 {
  public:
    DisplayN5110();
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
    void displayHeapForDebug();
    void drawVolumeBar(bool withNumber);
    char* utf8Rus(const char* str, bool uppercase);
    void drawScrollFrame(uint16_t texttop, uint16_t textheight, uint16_t bg);
    void getScrolBbounds(const char* text, const char* separator, byte textsize, uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth);
    void clearScroll(uint16_t texttop, uint16_t textheight, uint16_t bg);
    void frameTitle(const char* str);
    void rssi(const char* str);
    void ip(const char* str);
    void drawPlaylist(uint16_t currentItem, char* currentItemText);
    void loop();
  private:
    uint16_t swidth, sheight;
    unsigned long loopdelay;
    boolean checkdelay(int m, unsigned long &tstamp);
};

extern DisplayN5110 dsp;

/*
 * TFT COLORS
 */
#define SILVER      BLACK
#define TFT_BG      WHITE
#define TFT_FG      BLACK
#define TFT_LOGO    BLACK

#endif
