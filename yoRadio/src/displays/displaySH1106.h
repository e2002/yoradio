#ifndef displaySH1106_h
#define displaySH1106_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define TFT_LINEHGHT    8
#define TFT_FRAMEWDT    0

#define PLMITEMS        7
#define PLMITEMLENGHT   40
#define PLMITEMHEIGHT   9
#define TITLE_TOP2 TFT_FRAMEWDT + 3 * TFT_LINEHGHT
#define PLCURRENT_SIZE  1
#define TFT_FULLTIME    1
#define SCROLLDELTA 5
#define SCROLLTIME 110

class DspCore: public Adafruit_SH1106G {
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
    void loop();
  private:
    uint16_t swidth, sheight;
    unsigned long loopdelay;
    boolean checkdelay(int m, unsigned long &tstamp);
};

extern DspCore dsp;

/*
 * TFT COLORS
 */
#define SILVER      SH110X_WHITE
#define TFT_BG      SH110X_BLACK
#define TFT_FG      SH110X_WHITE
#define TFT_LOGO    SH110X_WHITE

#endif
