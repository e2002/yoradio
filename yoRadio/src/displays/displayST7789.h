#ifndef displayST7789_h
#define displayST7789_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
// https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#include "fonts/DS_DIGI42pt7b.h"

#define TFT_LINEHGHT    10
#define TFT_FRAMEWDT    8
#define META_SIZE       3
#define TITLE_SIZE1     2
#define TITLE_SIZE2     2

#if !defined(SCROLLDELTA) || !defined(SCROLLTIME)
#define SCROLLDELTA 5
#define SCROLLTIME 50
#endif

#define PLMITEMS        9
#define PLMITEMLENGHT   40
#define PLMITEMHEIGHT   22
#define TFT_FULLTIME    1

#define TITLE_TOP1 TFT_FRAMEWDT + META_SIZE * TFT_LINEHGHT + 8
#define TITLE_TOP2 TFT_FRAMEWDT + (META_SIZE+2) * TFT_LINEHGHT + 8
#define TITLE_FG2 SILVER

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
  private:
    uint16_t swidth, sheight;
    char oldTimeBuf[20];
    uint16_t wot, hot;

};

extern DspCore dsp;

/*
 * TFT COLORS
 */
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define GREEN       0x07E0
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define GRAY        0x7BEF
#define DARK_GRAY   0x2945
#define LIGHT_GRAY  0xC618
#define LIME        0x87E0
#define AQUA        0x5D1C
#define CYAN        0x07FF
#define DARK_CYAN   0x03EF
#define ORANGE      0xFCA0
#define PINK        0xF97F
#define BROWN       0x8200
#define VIOLET      0x9199
#define SILVER      0xA510
#define GOLD        0xA508
#define NAVY        0x000F
#define MAROON      0x7800
#define PURPLE      0x780F
#define OLIVE       0x7BE0

#define TFT_BG      BLACK
#define TFT_FG      WHITE
#define TFT_LOGO    0xE68B // 224, 209, 92

#endif
