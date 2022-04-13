#include "../../options.h"
#if DSP_MODEL==DSP_ST7735

#include "displayST7735.h"
#include <SPI.h>
#ifdef DSP_MINI
#include "fonts/bootlogo40.h"
#else
#include "fonts/bootlogo.h"
#endif
#include "../../player.h"
#include "../../config.h"
#include "../../network.h"

#ifndef DEF_SPI_FREQ
#define DEF_SPI_FREQ        40000000UL      /*  set it to 0 for system default */
#endif

DspCore::DspCore(): Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST) {

}

char* DspCore::utf8Rus(const char* str, bool uppercase) {
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

void DspCore::apScreen() {
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

void DspCore::initD(uint16_t &screenwidth, uint16_t &screenheight) {
  initR(DTYPE);
  if(DEF_SPI_FREQ > 0) setSPISpeed(DEF_SPI_FREQ);
  cp437(true);
  invertDisplay((DTYPE==INITR_MINI160x80)?TFT_INVERT:!TFT_INVERT);
  fillScreen(TFT_BG);
  setRotation(TFT_ROTATE);
  setTextWrap(false);
  screenwidth = width();
  screenheight = height();
  swidth = screenwidth;
  sheight = screenheight;
  setClockBounds();
}

void DspCore::drawLogo() {
#ifdef DSP_MINI
  drawRGBBitmap((swidth - 62) / 2, 5, bootlogo40, 62, 40);
#else
  drawRGBBitmap((swidth - 99) / 2, 18, bootlogo2, 99, 64);
#endif
}

// http://greekgeeks.net/#maker-tools_convertColor
#define CLR_ITEM1    0x52AA
#define CLR_ITEM2    0x39C7
#define CLR_ITEM3    0x18E3

void DspCore::drawPlaylist(uint16_t currentItem, char* currentItemText) {
  for (byte i = 0; i < PLMITEMS; i++) {
    plMenu[i][0] = '\0';
  }
  config.fillPlMenu(plMenu, currentItem - 3, PLMITEMS);
  setTextSize(2);
  int yStart = (sheight / 2 - PLMITEMHEIGHT / 2) - PLMITEMHEIGHT * (PLMITEMS - 1) / 2 + 3;
  //fillRect(0, (sheight / 2 - PLMITEMHEIGHT / 2) - 1, swidth, PLMITEMHEIGHT + 2, TFT_LOGO);
  for (byte i = 0; i < PLMITEMS; i++) {
    if (abs(i - 3) == 3) setTextColor(CLR_ITEM3, TFT_BG);
    if (abs(i - 3) == 2) setTextColor(CLR_ITEM2, TFT_BG);
    if (abs(i - 3) == 1) setTextColor(CLR_ITEM1, TFT_BG);
    if (i == 3) {
      strlcpy(currentItemText, plMenu[i], PLMITEMLENGHT - 1);
    } else {
      setCursor(TFT_FRAMEWDT, yStart + i * PLMITEMHEIGHT);
      fillRect(0, yStart + i * PLMITEMHEIGHT - 1, swidth, PLMITEMHEIGHT - 4, TFT_BG);
      print(utf8Rus(plMenu[i], true));
    }
  }
}

void DspCore::clearDsp() {
  fillScreen(TFT_BG);
}

void DspCore::drawScrollFrame(uint16_t texttop, uint16_t textheight, uint16_t bg) {
  fillRect(0, texttop, TFT_FRAMEWDT, textheight, bg);
  fillRect(swidth - TFT_FRAMEWDT, texttop, TFT_FRAMEWDT, textheight, bg);
}

void DspCore::getScrolBbounds(const char* text, const char* separator, byte textsize, uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth) {
  int16_t  x1, y1;
  uint16_t w, h;
  setTextSize(textsize);
  getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tWidth = w;
  tHeight = h;
  getTextBounds(separator, 0, 0, &x1, &y1, &w, &h);
  sWidth = w;
}

void DspCore::clearScroll(uint16_t texttop, uint16_t textheight, uint16_t bg) {
  fillRect(0,  texttop-2, swidth, textheight+3, bg);
}

void DspCore::centerText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  int16_t  x1, y1;
  uint16_t w, h;
  const char* txt = text;
  getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  setTextColor(fg);
  setCursor((swidth - w) / 2, y);
  fillRect(0, y, swidth, h, bg);
  print(txt);
}

void DspCore::rightText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  int16_t  x1, y1;
  uint16_t w, h;
  getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  setTextColor(fg);
  setCursor(swidth - w - TFT_FRAMEWDT, y);
  fillRect(swidth - w - TFT_FRAMEWDT, y, w, h, bg);
  print(text);
}

void DspCore::displayHeapForDebug() {
#ifndef DSP_MINI
  int16_t vTop = sheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT * 2 - 2;
  setTextSize(1);
  setTextColor(DARK_GRAY, TFT_BG);
  setCursor(TFT_FRAMEWDT, vTop);
  fillRect(TFT_FRAMEWDT, vTop, swidth - TFT_FRAMEWDT / 2, 7, TFT_BG);
  print(ESP.getFreeHeap());
  print(" / ");
  print(ESP.getMaxAllocHeap());
#if VS1053_CS==255
  // audio buffer;
  fillRect(0, sheight - 2, swidth, 2, TFT_BG);
  int astored = player.inBufferFilled();
  int afree = player.inBufferFree();
  int aprcnt = 100 * astored / (astored + afree);
  byte sbw = map(aprcnt, 0, 100 , 0, swidth);
  fillRect(0, sheight - 2, sbw, 2, SILVER);
#endif
#endif
}

void DspCore::setClockBounds(){
  setFont(&DS_DIGI28pt7b);
  setTextSize(1);
  getTextBounds("88:88", 0, 0, &x, &y, &cwidth, &cheight);
  uint16_t header = TFT_FRAMEWDT + 4 * TFT_LINEHGHT;
  uint16_t footer = TFT_FRAMEWDT * 2 + TFT_LINEHGHT + 5;
  clockY = header + (sheight - header - footer) / 2 - cheight / 2;
  if(DTYPE==INITR_MINI160x80) clockY = clockY-6;
  setFont();
}

void DspCore::printClock(const char* timestr) {

}

byte DspCore::getPw(uint16_t ncwidth){
  byte pw = 6;
  if(ncwidth<35) pw = 7;
  if(ncwidth<20) pw = 8;
  return pw;
}

void DspCore::printClock(struct tm timeinfo, bool dots, bool redraw){
  char timeBuf[50] = { 0 };
  char tmpBuf[4] = { 0 };
  uint16_t ncwidth, ncheight;
  strftime(timeBuf, sizeof(timeBuf), "%H %M", &timeinfo);
  setTextSize(1);
    setFont(&DS_DIGI28pt7b);
  if(strstr(oldTimeBuf, timeBuf)==NULL || redraw){
    getTextBounds(oldTimeBuf, 0, 0, &x, &y, &wot, &hot);
    setCursor((swidth - wot) / 2 - 4, clockY+28+6);
    setTextColor(TFT_BG);
    print(oldTimeBuf);
    dot = (swidth - wot) / 2 - 4;
    /*  dots  */
    strlcpy(tmpBuf, oldTimeBuf, 3);
    getTextBounds(tmpBuf, 0, 0, &x, &y, &ncwidth, &ncheight);
    dot = dot + ncwidth + getPw(ncwidth);
    setCursor(dot, clockY+28+6);
    print(":");
    /*  dots  */

    strlcpy(oldTimeBuf, timeBuf, 20);
    setTextSize(1);
    getTextBounds(timeBuf, 0, 0, &x, &y, &ncwidth, &ncheight);
    setTextColor(TFT_LOGO);
    setCursor((swidth - ncwidth) / 2 - 4, clockY+28+6);
    dot = (swidth - ncwidth) / 2 - 4;
    setTextSize(1);
    print(timeBuf);
    /*  dots  */
    strftime(timeBuf, sizeof(timeBuf), "%H", &timeinfo);
    getTextBounds(timeBuf, 0, 0, &x, &y, &ncwidth, &ncheight);
    dot = dot + ncwidth + getPw(ncwidth);
    /*  dots  */
  }
  setCursor(dot, clockY+28+6);
  setTextColor(dots?TFT_BG:TFT_LOGO);
  print(":");
  setFont();
  yield();
}
#ifdef DSP_MINI
#define VTOP  TITLE_TOP1+6
#else
#define VTOP  48
#endif
void DspCore::drawVolumeBar(bool withNumber) {
#ifdef DSP_MINI
  int16_t vTop = sheight - TFT_FRAMEWDT - 2;
  int16_t vWidth = swidth - TFT_FRAMEWDT * 2;
  uint8_t ww = map(config.store.volume, 0, 254, 0, vWidth);
  fillRect(TFT_FRAMEWDT, vTop, vWidth, 2, TFT_BG);
  fillRect(TFT_FRAMEWDT, vTop, ww, 2, TFT_LOGO);
#else
  int16_t vTop = sheight - TFT_FRAMEWDT - 6;
  int16_t vWidth = swidth - TFT_FRAMEWDT * 2;
  uint8_t ww = map(config.store.volume, 0, 254, 0, vWidth - 2);
  fillRect(TFT_FRAMEWDT, vTop - 2, vWidth, 6, TFT_BG);
  drawRect(TFT_FRAMEWDT, vTop - 2, vWidth, 6, TFT_LOGO);
  fillRect(TFT_FRAMEWDT + 1, vTop - 1, ww, 5, TFT_LOGO);
#endif
  if (withNumber) {
    setTextSize(1);
    setTextColor(TFT_FG);
    setFont(&DS_DIGI28pt7b);
    char volstr[4];
    uint16_t wv, hv;
    int16_t  x1, y1;
    sprintf(volstr, "%d", config.store.volume);
    getTextBounds(volstr, 0, 0, &x1, &y1, &wv, &hv);
    fillRect(TFT_FRAMEWDT, VTOP, swidth - TFT_FRAMEWDT / 2, hv + 3, TFT_BG);
    setCursor((swidth - wv) / 2, VTOP + hv);
    print(volstr);
    setFont();
  }
}

void DspCore::drawNextStationNum(uint16_t num) {
  setTextSize(1);
  setTextColor(TFT_FG);
  setFont(&DS_DIGI28pt7b);
  char numstr[7];
  uint16_t wv, hv;
  int16_t  x1, y1;
  sprintf(numstr, "%d", num);
  getTextBounds(numstr, 0, 0, &x1, &y1, &wv, &hv);
  fillRect(TFT_FRAMEWDT, VTOP, swidth - TFT_FRAMEWDT / 2, hv + 3, TFT_BG);
  setCursor((swidth - wv) / 2, VTOP + hv);
  print(numstr);
  setFont();
}

void DspCore::frameTitle(const char* str) {
  setTextSize(2);
  centerText(str, TFT_FRAMEWDT, TFT_LOGO, TFT_BG);
}

void DspCore::rssi(const char* str) {
  int16_t vTop = sheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT - 2;
  setTextSize(1);
  rightText(str, vTop, SILVER, TFT_BG);
}

void DspCore::ip(const char* str) {
  int16_t vTop = sheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT - 2;
  setTextSize(1);
  setTextColor(SILVER, TFT_BG);
  setCursor(4, vTop);
  print(str);
}

void DspCore::set_TextSize(uint8_t s) {
  setTextSize(s);
}

void DspCore::set_TextColor(uint16_t fg, uint16_t bg) {
  setTextColor(fg, bg);
}

void DspCore::set_Cursor(int16_t x, int16_t y) {
  setCursor(x, y);
}

void DspCore::printText(const char* txt) {
  print(txt);
}

void DspCore::loop(bool force) {

}

#endif
