#ifndef displaySSD1306_h
#define displaySSD1306_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define TFT_LINEHGHT    8
#define TFT_FRAMEWDT    0
#define PLMITEMLENGHT   40
#define DSP_CAN_SLEEP  true
#define DSP_OLED       true

#if !defined(SCROLLDELTA) || !defined(SCROLLTIME)
#define SCROLLDELTA 2
#define SCROLLTIME 35
#endif

#if DSP_MODEL==DSP_SSD1306

#define PLMITEMS        5
#define PLMITEMHEIGHT   18
#define TITLE_TOP2 TFT_FRAMEWDT + 3 * TFT_LINEHGHT
#define PLCURRENT_SIZE  2
#define BOOTSTR_TOP1    64-TFT_LINEHGHT*2-5
#define BOOTSTR_TOP2    64-TFT_LINEHGHT
#define VOL_TOP     24

#else

#define PLMITEMS        5
#define PLMITEMHEIGHT   10
#define TITLE_TOP2 TFT_FRAMEWDT + 2 * TFT_LINEHGHT
#define PLCURRENT_SIZE  1
#define META_SIZE       1
#define TITLE_SIZE2     0
#define TITLE_TOP1 TFT_FRAMEWDT + META_SIZE * TFT_LINEHGHT + 3
#define TFT_FULLTIME    1
#define BOOTSTR_TOP1    14
#define BOOTSTR_TOP2    24
#define CLOCK_SPACE 38
#define VOL_SPACE   0
#define VOL_TOP     16

#endif
class DspCore: public Adafruit_SSD1306 {
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
    unsigned long loopdelay;
    uint8_t maxcontrast;
    char insideClc[10];
    boolean checkdelay(int m, unsigned long &tstamp);
};

extern DspCore dsp;

/*
 * TFT COLORS
 */
#define SILVER      WHITE
#define TFT_BG      BLACK
#define TFT_FG      WHITE
#define TFT_LOGO    WHITE

#endif
