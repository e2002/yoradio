#ifndef displayILI9225_h
#define displayILI9225_h

#include "Arduino.h"
#include "../ILI9225Fix/TFT_22_ILI9225Fix.h"
#include "fonts/DS_DIGI28pt7b.h"

#define VU_READY        1
#define WEATHER_READY   1
#define DSP_CAN_SLEEP   true

#define TFT_LINEHGHT    10
#define TFT_FRAMEWDT    4
#define META_SIZE       2
#ifndef TITLE_SIZE1
#define TITLE_SIZE1     1
#endif
#ifndef TITLE_SIZE2
#define TITLE_SIZE2     1
#endif
#if !defined(SCROLLDELTA) || !defined(SCROLLTIME)
#define SCROLLDELTA 3
#define SCROLLTIME 30
#endif

#define PLMITEMS        9
#define PLMITEMLENGHT   40
#define PLMITEMHEIGHT   22
#define TFT_FULLTIME    1
#ifndef TITLE_TOP1
#define TITLE_TOP1 TFT_FRAMEWDT + META_SIZE * TFT_LINEHGHT + 8
#endif
#ifndef TITLE_TOP2
#define TITLE_TOP2 TFT_FRAMEWDT + (META_SIZE+2) * TFT_LINEHGHT
#endif

class DspCore: public TFT_22_ILI9225 {
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
    void setFont(uint8_t* font, bool monoSp=false );
    void setFont(const GFXfont *f = NULL);
    void setTextSize(uint8_t s);
    void setTextColor(uint16_t fg, uint16_t bg);
    void setCursor(int16_t x, int16_t y);
    uint16_t print(const char* s);
    void getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1,
                     int16_t *y1, uint16_t *w, uint16_t *h);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                        uint16_t color);
    int16_t width(void) { return (int16_t)maxX(); }
    int16_t height(void) { return (int16_t)maxY(); }
    void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);
    void flip();
    void invert();
    void sleep();
    void wake();
  private:
    uint16_t swidth, sheight;
    uint16_t bgcolor, fgcolor;
    int16_t  cursorx, cursory;
    bool gFont, started;
    char oldTimeBuf[20];
    uint8_t oldVolume;
    uint16_t wot, hot;

};

extern DspCore dsp;

#endif
