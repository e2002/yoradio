#include "displayST7735.h"
#include <SPI.h>
#include "fonts/bootlogo.h"
#include "player.h"
#include "config.h"
#include "network.h"

#define DTYPE INITR_BLACKTAB // 1.8' https://aliexpress.ru/item/1005002822797745.html
/*  If there is a noisy line on one side of the screen, then in Adafruit_ST7735.cpp:

  // Black tab, change MADCTL color filter
  if ((options == INITR_BLACKTAB) || (options == INITR_MINI160x80)) {
    uint8_t data = 0xC0;
    sendCommand(ST77XX_MADCTL, &data, 1);
  _add this_ ->    _colstart = 2;
  _add this_ ->    _rowstart = 1;
  }

*/
//#define DTYPE INITR_144GREENTAB // 1.44' https://aliexpress.ru/item/1005002822797745.html

class GFXClock {
  public:
    GFXClock() {};
    uint16_t init(Adafruit_ST7735 &tftd, const GFXfont *font, uint16_t fgcolor, uint16_t bgcolor ) {
      _dsp = &tftd;
      tftd.setFont(font);
      tftd.getTextBounds("88:88", 0, 0, &x, &y, &cwidth, &cheight);
      tftd.setFont();
      fg = fgcolor;
      bg = bgcolor;
      swidth = tftd.width();
      _canvas = new GFXcanvas1(swidth, cheight + 3);
      _canvas->setFont(font);
      _canvas->setTextWrap(false);
      _canvas->setTextColor(WHITE);
      uint16_t header = TFT_FRAMEWDT + 4 * TFT_LINEHGHT;
      uint16_t footer = TFT_FRAMEWDT * 2 + TFT_LINEHGHT + 5;
      clockY = header + (tftd.height() - header - footer) / 2 - cheight / 2;
      return cheight;
    }
    void print(const char* timestr) {
      _canvas->fillScreen(BLACK);
      _canvas->getTextBounds(timestr, 0, 0, &x, &y, &cwidth, &cheight);
      _canvas->setCursor((swidth - cwidth) / 2 - 4, cheight);
      _canvas->print(timestr);
      _dsp->drawBitmap(0, clockY , _canvas->getBuffer(), swidth, cheight + 3, fg, bg);
    }
  private:
    int16_t x, y;
    uint16_t cwidth, cheight, fg, bg, clockY, swidth;
    GFXcanvas1 *_canvas;
    Adafruit_ST7735 *_dsp;
};

GFXClock gclock;

DisplayST7735::DisplayST7735(): Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST) {

}

char* DisplayST7735::utf8Rus(const char* str, bool uppercase) {
  int index = 0;
  static char strn[BUFLEN];
  bool E = false;
  strlcpy(strn, str, BUFLEN);
  if (uppercase) {
    bool next = false;
    for (char *iter = strn; *iter != '\0'; ++iter)
    {
      if (E) {
        E = false;
        continue;
      }
      byte rus = (byte) * iter;
      if (rus == 208 && (byte) * (iter + 1) == 129) { // ёКостыли
        *iter = (char)209;
        *(iter + 1) = (char)145;
        E = true;
        continue;
      }
      if (rus == 209 && (byte) * (iter + 1) == 145) {
        *iter = (char)209;
        *(iter + 1) = (char)145;
        E = true;
        continue;
      }
      if (next) {
        if (rus >= 128 && rus <= 143) *iter = (char)(rus + 32);
        if (rus >= 176 && rus <= 191) *iter = (char)(rus - 32);
        next = false;
      }
      if (rus == 208) next = true;
      if (rus == 209) {
        *iter = (char)208;
        next = true;
      }
      *iter = toupper(*iter);
    }
  }
  while (strn[index])
  {
    if (strn[index] >= 0xBF)
    {
      switch (strn[index]) {
        case 0xD0: {
            if (strn[index + 1] == 0x81) {
              strn[index] = 0xA8;
              break;
            }
            if (strn[index + 1] >= 0x90 && strn[index + 1] <= 0xBF) strn[index] = strn[index + 1] + 0x30;
            break;
          }
        case 0xD1: {
            if (strn[index + 1] == 0x91) {
              //strn[index] = 0xB7;
              strn[index] = 0xB8;
              break;
            }
            if (strn[index + 1] >= 0x80 && strn[index + 1] <= 0x8F) strn[index] = strn[index + 1] + 0x70;
            break;
          }
      }
      int sind = index + 2;
      while (strn[sind]) {
        strn[sind - 1] = strn[sind];
        sind++;
      }
      strn[sind - 1] = 0;
    }
    index++;
  }
  return strn;
}

void DisplayST7735::apScreen() {
  setTextSize(1);
  setTextColor(TFT_FG, TFT_BG);
  setCursor(TFT_FRAMEWDT, TFT_FRAMEWDT + 2 * TFT_LINEHGHT);
  print("AP NAME: ");
  print(apSsid);
  setCursor(TFT_FRAMEWDT, TFT_FRAMEWDT + 3 * TFT_LINEHGHT);
  print("PASSWORD: ");
  print(apPassword);
  setTextColor(SILVER, TFT_BG);
  setCursor(TFT_FRAMEWDT, 107);
  print("SETTINGS PAGE ON: ");
  setCursor(TFT_FRAMEWDT, 117);
  print("http://");
  print(WiFi.softAPIP().toString().c_str());
  print("/");
}

void DisplayST7735::initD(uint16_t &screenwidth, uint16_t &screenheight) {
  initR(DTYPE);
  cp437(true);
  fillScreen(TFT_BG);
  setRotation(TFT_ROTATE);
  setTextWrap(false);
  screenwidth = width();
  screenheight = height();
  swidth = screenwidth;
  sheight = screenheight;
  gclock.init(dsp, &DS_DIGI28pt7b, TFT_LOGO, BLACK);
}

void DisplayST7735::drawLogo() {
  drawRGBBitmap((swidth - 99) / 2, 18, bootlogo2, 99, 64);
}

// http://greekgeeks.net/#maker-tools_convertColor
#define CLR_ITEM1    0x52AA
#define CLR_ITEM2    0x39C7
#define CLR_ITEM3    0x18E3

void DisplayST7735::drawPlaylist(uint16_t currentItem, char* currentItemText) {
  for (byte i = 0; i < PLMITEMS; i++) {
    plMenu[i][0] = '\0';
  }
  config.fillPlMenu(plMenu, currentItem - 3, PLMITEMS);
  setTextSize(2);
  int yStart = (sheight / 2 - PLMITEMHEIGHT / 2) - PLMITEMHEIGHT * (PLMITEMS - 1) / 2 + 3;
  fillRect(0, (sheight / 2 - PLMITEMHEIGHT / 2) - 1, swidth, PLMITEMHEIGHT + 2, TFT_LOGO);
  for (byte i = 0; i < PLMITEMS; i++) {
    if (abs(i - 3) == 3) setTextColor(CLR_ITEM3, TFT_BG);
    if (abs(i - 3) == 2) setTextColor(CLR_ITEM2, TFT_BG);
    if (abs(i - 3) == 1) setTextColor(CLR_ITEM1, TFT_BG);
    if (i == 3) {
      strlcpy(currentItemText, plMenu[i], PLMITEMLENGHT - 1);
    } else {
      setCursor(TFT_FRAMEWDT, yStart + i * PLMITEMHEIGHT);
      print(utf8Rus(plMenu[i], true));
    }
  }
}

void DisplayST7735::clearDsp() {
  fillScreen(TFT_BG);
}

void DisplayST7735::drawScrollFrame(uint16_t texttop, uint16_t textheight, uint16_t bg) {
  fillRect(0, texttop, TFT_FRAMEWDT, textheight, bg);
  fillRect(swidth - TFT_FRAMEWDT, texttop, TFT_FRAMEWDT, textheight, bg);
}

void DisplayST7735::getScrolBbounds(const char* text, const char* separator, byte textsize, uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth) {
  int16_t  x1, y1;
  uint16_t w, h;
  setTextSize(textsize);
  getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tWidth = w;
  tHeight = h;
  getTextBounds(separator, 0, 0, &x1, &y1, &w, &h);
  sWidth = w;
}

void DisplayST7735::clearScroll(uint16_t texttop, uint16_t textheight, uint16_t bg) {
  fillRect(0,  texttop, swidth, textheight, bg);
}

void DisplayST7735::centerText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  int16_t  x1, y1;
  uint16_t w, h;
  const char* txt = text;
  getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  setTextColor(fg);
  setCursor((swidth - w) / 2, y);
  fillRect(0, y, swidth, h, bg);
  print(txt);
}

void DisplayST7735::rightText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  int16_t  x1, y1;
  uint16_t w, h;
  getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  setTextColor(fg);
  setCursor(swidth - w - TFT_FRAMEWDT, y);
  fillRect(swidth - w - TFT_FRAMEWDT, y, w, h, bg);
  print(text);
}

void DisplayST7735::displayHeapForDebug() {
  int16_t vTop = sheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT * 2 - 2;
  setTextSize(1);
  setTextColor(DARK_GRAY, TFT_BG);
  setCursor(TFT_FRAMEWDT, vTop);
  fillRect(TFT_FRAMEWDT, vTop, swidth - TFT_FRAMEWDT / 2, 7, TFT_BG);
  print(ESP.getFreeHeap());
  print(" / ");
  print(ESP.getMaxAllocHeap());

  // audio buffer;
  fillRect(0, sheight - 2, swidth, 2, TFT_BG);
  int astored = player.inBufferFilled();
  int afree = player.inBufferFree();
  int aprcnt = 100 * astored / (astored + afree);
  byte sbw = map(aprcnt, 0, 100 , 0, swidth);
  fillRect(0, sheight - 2, sbw, 2, SILVER);
}

void DisplayST7735::printClock(const char* timestr) {
  gclock.print(timestr);
}

void DisplayST7735::drawVolumeBar(bool withNumber) {
  int16_t vTop = sheight - TFT_FRAMEWDT * 2;
  int16_t vWidth = swidth - TFT_FRAMEWDT - 4;
  uint8_t ww = map(config.store.volume, 0, 254, 0, vWidth - 2);
  fillRect(TFT_FRAMEWDT, vTop - 2, vWidth, 6, TFT_BG);
  drawRect(TFT_FRAMEWDT, vTop - 2, vWidth, 6, TFT_LOGO);
  fillRect(TFT_FRAMEWDT + 1, vTop - 1, ww, 5, TFT_LOGO);
  if (withNumber) {
    setTextSize(1);
    setTextColor(TFT_FG);
    setFont(&DS_DIGI28pt7b);
    char volstr[4];
    uint16_t wv, hv;
    int16_t  x1, y1;
    sprintf(volstr, "%d", config.store.volume);
    getTextBounds(volstr, 0, 0, &x1, &y1, &wv, &hv);
    fillRect(TFT_FRAMEWDT, 48, swidth - TFT_FRAMEWDT / 2, hv + 3, TFT_BG);
    setCursor((swidth - wv) / 2, 48 + hv);
    print(volstr);
    setFont();
  }
}

void DisplayST7735::frameTitle(const char* str) {
  setTextSize(2);
  centerText(str, TFT_FRAMEWDT, TFT_LOGO, TFT_BG);
}

void DisplayST7735::rssi(const char* str) {
  int16_t vTop = sheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT - 2;
  setTextSize(1);
  rightText(str, vTop, SILVER, TFT_BG);
}

void DisplayST7735::ip(const char* str) {
  int16_t vTop = sheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT - 2;
  setTextSize(1);
  setTextColor(SILVER, TFT_BG);
  setCursor(4, vTop);
  print(str);
}

void DisplayST7735::set_TextSize(uint8_t s) {
  setTextSize(s);
}

void DisplayST7735::set_TextColor(uint16_t fg, uint16_t bg) {
  setTextColor(fg, bg);
}

void DisplayST7735::set_Cursor(int16_t x, int16_t y) {
  setCursor(x, y);
}

void DisplayST7735::printText(const char* txt) {
  print(txt);
}
