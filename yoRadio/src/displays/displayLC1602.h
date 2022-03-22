#ifndef displayLC1602_h
#define displayLC1602_h

#include "Arduino.h"
#include "../LiquidCrystalI2C/LiquidCrystalI2CEx.h"

#define TFT_LINEHGHT    1
#define TFT_FRAMEWDT    0

#define PLMITEMS        1
#define PLMITEMLENGHT   40
#define PLMITEMHEIGHT   9
#define TITLE_TOP1      1
#define TITLE_SIZE2     0
#define PL_TOP          1
#define PLCURRENT_SIZE  1

#define SCROLLDELTA 1
#define SCROLLTIME 250
#define BOOTSTR_TOP2    0
#define BOOTSTR_TOP1    1
#define STARTTIME_PL    2000

class DspCore: public LiquidCrystal_I2C {
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
    char* utf8Rus(const char* str, bool uppercase=true);
    void drawScrollFrame(uint16_t texttop, uint16_t textheight, uint16_t bg);
    void getScrolBbounds(const char* text, const char* separator, byte textsize, uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth);
    void clearScroll(uint16_t texttop, uint16_t textheight, uint16_t bg);
    void frameTitle(const char* str);
    void rssi(const char* str);
    void ip(const char* str);
    void drawPlaylist(uint16_t currentItem, char* currentItemText);
    void loop();
  private:
    uint16_t swidth, sheight, xOffset, yOffset;
    int16_t nextX;
    unsigned long loopdelay;
    boolean checkdelay(int m, unsigned long &tstamp);
};

extern DspCore dsp;

/*
 * TFT COLORS
 */
#define CLOCK_SPACE 6
#define VOL_SPACE   3
#define SILVER      0
#define TFT_BG      0
#define TFT_FG      CLOCK_SPACE
#define TFT_LOGO    VOL_SPACE

#endif
