#include "../../options.h"
#if DSP_MODEL==DSP_ST7789 || DSP_MODEL==DSP_ST7789_240

#include "displayST7789.h"
#include <SPI.h>
#include "fonts/bootlogo.h"
#include "../../player.h"
#include "../../config.h"
#include "../../network.h"

#ifndef DEF_SPI_FREQ
#define DEF_SPI_FREQ        40000000UL      /*  set it to 0 for system default */
#endif
const char *dow[7] = {"вс","пн","вт","ср","чт","пт","сб"};
const char *mnths[12] = {"января","февраля","марта","апреля","мая","июня","июля","августа","сентября","октября","ноября","декабря"};

#define TAKE_MUTEX() if(player.mutex_pl) xSemaphoreTake(player.mutex_pl, portMAX_DELAY)
#define GIVE_MUTEX() if(player.mutex_pl) xSemaphoreGive(player.mutex_pl)

DspCore::DspCore(): Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST) {

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
  setTextSize(TITLE_SIZE1);
  setTextColor(TFT_FG, TFT_BG);
  setCursor(TFT_FRAMEWDT, TITLE_TOP1);
  print("AP NAME: ");
  print(apSsid);
  setCursor(TFT_FRAMEWDT, TITLE_TOP2);
  print("PASSWORD: ");
  print(apPassword);
  setTextColor(SILVER, TFT_BG);
  setCursor(TFT_FRAMEWDT, sheight-TFT_FRAMEWDT-TFT_LINEHGHT*4);
  print("SETTINGS PAGE ON: ");
  setCursor(TFT_FRAMEWDT, sheight-TFT_FRAMEWDT-TFT_LINEHGHT*2);
  print("http://");
  print(WiFi.softAPIP().toString().c_str());
  print("/");
  drawFastHLine(TFT_FRAMEWDT, TITLE_TOP1-8, swidth-TFT_FRAMEWDT*2, SILVER);
}

void DspCore::initD(uint16_t &screenwidth, uint16_t &screenheight) {
  init(240,(DSP_MODEL==DSP_ST7789)?320:240);
  if(DEF_SPI_FREQ > 0) setSPISpeed(DEF_SPI_FREQ);
  invertDisplay(TFT_INVERT);
  cp437(true);
  fillScreen(TFT_BG);
  setRotation(TFT_ROTATE);
  setTextWrap(false);
  setTextSize(1);
  screenwidth = width();
  screenheight = height();
  swidth = screenwidth;
  sheight = screenheight;
}

void DspCore::drawLogo() {
  drawRGBBitmap((swidth - 99) / 2, (sheight-64)/2 - TFT_LINEHGHT*2, bootlogo2, 99, 64);
}

// http://greekgeeks.net/#maker-tools_convertColor
uint16_t iclrs[] = { 0x738E /*707070*/, 0x52AA /*575757*/, 0x39C7, 0x18E3 };
void DspCore::drawPlaylist(uint16_t currentItem, char* currentItemText) {
  for (byte i = 0; i < PLMITEMS; i++) {
    plMenu[i][0] = '\0';
  }
  config.fillPlMenu(plMenu, currentItem - 4, PLMITEMS);
  setTextSize(2);
  int yStart = (sheight / 2 - PLMITEMHEIGHT / 2) - PLMITEMHEIGHT * (PLMITEMS - 1) / 2 + 3;

  for (byte i = 0; i < PLMITEMS; i++) {
    if (i == 4) {
      //fillRect(0, (sheight / 2 - PLMITEMHEIGHT / 2) - 1, swidth, PLMITEMHEIGHT + 2, TFT_LOGO);
      strlcpy(currentItemText, plMenu[i], PLMITEMLENGHT - 1);
    } else {
      setTextColor(iclrs[abs(i - 4)-1], TFT_BG);
      setCursor(TFT_FRAMEWDT, yStart + i * PLMITEMHEIGHT);
      fillRect(0, yStart + i * PLMITEMHEIGHT - 1, swidth, PLMITEMHEIGHT - 2, TFT_BG);
      print(utf8Rus(plMenu[i], true));
    }
  }
}

void DspCore::clearDsp() {
  fillScreen(TFT_BG);
}

void DspCore::drawScrollFrame(uint16_t texttop, uint16_t textheight, uint16_t bg) {
  if (TFT_FRAMEWDT==0) return;
  fillRect(swidth - TFT_FRAMEWDT, texttop, TFT_FRAMEWDT, textheight, bg);
  fillRect(0, texttop, TFT_FRAMEWDT, textheight, bg);
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
  fillRect(0,  texttop-4, swidth, textheight+7, bg);
  yield();
}

void DspCore::centerText(const char* text, uint16_t y, uint16_t fg, uint16_t bg) {
  int16_t  x1, y1;
  uint16_t w, h;
  const char* txt = text;
  if(y==90) y=(sheight-64)/2 + 64 + TFT_LINEHGHT;
  if(y==110) y=(sheight-64)/2 + 64 + TFT_LINEHGHT*3;
  getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  setTextColor(fg);
  setCursor((swidth - w) / 2, y);
  fillRect((swidth-w)/2-5, y, w+10, h, bg);
  print(txt);
  yield();
}

void DspCore::rightText(const char* text, uint16_t y, uint16_t fg, uint16_t bg, bool fliprect, uint16_t delta) {
  int16_t  x1, y1;
  uint16_t w, h;
  getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  setTextColor(fg,bg);
  setCursor(swidth - w - TFT_FRAMEWDT - delta, y);
  fillRect(swidth - w - TFT_FRAMEWDT, fliprect?y-h:y, w, h, bg);
  print(text);
  yield();
}

void DspCore::displayHeapForDebug() {
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
  yield();
}

void DspCore::printClock(const char* timestr) {

}

uint16_t cltop = 0;
uint8_t clsp = 24;
uint16_t clleft = 0;
uint16_t clwidth = 0;

void DspCore::printClock(struct tm timeinfo, bool dots, bool redraw){
  char timeBuf[50] = { 0 };
  strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
  if(strstr(oldTimeBuf, timeBuf)==NULL || redraw){
    int16_t  x1, y1;
    setTextSize(1);
    setFont(&DS_DIGI42pt7b);
    getTextBounds(oldTimeBuf, 0, 0, &x1, &y1, &wot, &hot);
    if(cltop==0){
      cltop=sheight-(TFT_FRAMEWDT * 2 + TFT_LINEHGHT + 38) - hot;
    }
    clwidth = wot+clsp+(swidth>240?46:34);
    fillRect(swidth-TFT_FRAMEWDT-clwidth, cltop-hot, clwidth, hot+3, TFT_BG);
    strlcpy(oldTimeBuf, timeBuf, 20);
    setTextSize(1);
    getTextBounds(timeBuf, 0, 0, &x1, &y1, &wot, &hot);
    clwidth = wot+clsp+(swidth>240?46:34);
    clleft=swidth-TFT_FRAMEWDT-clwidth;
    setTextColor(TFT_LOGO, TFT_BG);
    setCursor(clleft, cltop);
    setTextSize(1);
    print(timeBuf);

    setFont();
    setTextSize(3);
    setTextColor(TFT_FG, TFT_BG);
    setCursor(clleft+wot+clsp, cltop-hot+32);
    print(utf8Rus(dow[timeinfo.tm_wday], false));

    sprintf(timeBuf, "%2d %s %d", timeinfo.tm_mday,mnths[timeinfo.tm_mon], timeinfo.tm_year+1900);
    setTextSize(1);
    uint16_t wdate, hdate;
    getTextBounds(timeBuf, 0, 0, &x1, &y1, &wdate, &hdate);
    fillRect(swidth - wdate - TFT_FRAMEWDT-20, cltop+10, wdate+20, hdate, TFT_BG);
    rightText(utf8Rus(timeBuf,true), cltop+10, TFT_FG, TFT_BG, false, swidth>240?12:0);
    drawFastVLine(clleft+wot+clsp/2+3, cltop-hot, hot+3, SILVER);
    drawFastHLine(clleft+wot+clsp/2+3, cltop-hot+29, 42, SILVER);

    drawFastHLine(TFT_FRAMEWDT, TITLE_TOP1-8, swidth-TFT_FRAMEWDT*2, SILVER);
  }
  setTextSize(3);
  setTextColor(TFT_LOGO, TFT_BG);
  setCursor(clleft+wot+clsp, cltop-hot+1);
  sprintf(timeBuf, "%02d", timeinfo.tm_sec);
  print(timeBuf);
  yield();
}

void DspCore::drawVolumeBar(bool withNumber) {
  int16_t vTop = sheight - TFT_FRAMEWDT * 2;
  int16_t volTop = sheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT - 2;
  int16_t vWidth = swidth - TFT_FRAMEWDT *2;
  uint16_t ww = map(config.store.volume, 0, 254, 0, vWidth - 2);
  fillRect(TFT_FRAMEWDT, vTop - 2, vWidth, 6, TFT_BG);
  drawRect(TFT_FRAMEWDT, vTop - 2, vWidth, 6, TFT_LOGO);
  fillRect(TFT_FRAMEWDT + 1, vTop - 1, ww, 5, TFT_LOGO);
  if(swidth>240){
    char buf[20];
    sprintf(buf, "VOL %d", config.store.volume);
    setTextSize(1);
    centerText(buf, volTop, SILVER, TFT_BG);
  }
  if (withNumber) {
    setTextSize(1);
    setTextColor(TFT_FG);
    setFont(&DS_DIGI42pt7b);
    char volstr[4];
    uint16_t wv, hv;
    int16_t  x1, y1;
    sprintf(volstr, "%d", config.store.volume);
    getTextBounds(volstr, 0, 0, &x1, &y1, &wv, &hv);
    fillRect(TFT_FRAMEWDT, (sheight-hv)/2, swidth - TFT_FRAMEWDT / 2, hv + 3, TFT_BG);
    setCursor((swidth - wv) / 2, (sheight-hv)/2 + hv);
    print(volstr);
    setFont();
  }
  yield();
}

void DspCore::drawNextStationNum(uint16_t num) {
  setTextSize(1);
  setTextColor(TFT_FG);
  setFont(&DS_DIGI42pt7b);
  char numstr[7];
  uint16_t wv, hv;
  int16_t  x1, y1;
  sprintf(numstr, "%d", num);
  getTextBounds(numstr, 0, 0, &x1, &y1, &wv, &hv);
  fillRect(TFT_FRAMEWDT, (sheight-hv)/2, swidth - TFT_FRAMEWDT / 2, hv + 3, TFT_BG);
  setCursor((swidth - wv) / 2, (sheight-hv)/2 + hv);
  print(numstr);
  setFont();
}

void DspCore::frameTitle(const char* str) {
  setTextSize(META_SIZE);
  centerText(str, TFT_FRAMEWDT, TFT_LOGO, TFT_BG);
  drawFastHLine(TFT_FRAMEWDT, TITLE_TOP1-8, swidth-TFT_FRAMEWDT*2, SILVER);
}

void DspCore::rssi(const char* str) {
  int16_t vTop = sheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT - 2;
  char buf[20];
  sprintf(buf, "RSSI:%s", str);
  setTextSize(1);
  rightText(buf, vTop, SILVER, TFT_BG);
}

void DspCore::ip(const char* str) {
  int16_t vTop = sheight - TFT_FRAMEWDT * 2 - TFT_LINEHGHT - 2;
  char buf[30];
  sprintf(buf, "IP: %s", str);
  setTextSize(1);
  setTextColor(SILVER, TFT_BG);
  setCursor(TFT_FRAMEWDT, vTop);
  print(buf);
}

void DspCore::startWrite(void) {
  TAKE_MUTEX();
  Adafruit_ST7789::startWrite();
}

void DspCore::endWrite(void) {
  Adafruit_ST7789::endWrite();
  GIVE_MUTEX();
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
