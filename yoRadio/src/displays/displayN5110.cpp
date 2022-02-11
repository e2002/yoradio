#include "../../options.h"
#if DSP_MODEL==3

#include "displayN5110.h"
#include <Wire.h>
#include "../../player.h"
#include "../../config.h"
#include "../../network.h"

#define LOGO_WIDTH 21
#define LOGO_HEIGHT 28

const unsigned char logo [] PROGMEM=
{
	0x07, 0x03, 0x80, 0x0f, 0x87, 0xc0, 0x0f, 0x87, 0xc0, 0x0f, 0x87, 0xc0, 0x0f, 0x87, 0xc0, 0x07,
	0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x07, 0xff, 0x80, 0x1f, 0xff,
	0xc0, 0x1f, 0xff, 0xe0, 0x3f, 0xff, 0xf0, 0x7f, 0x07, 0xf0, 0x7e, 0x03, 0xf0, 0x7e, 0x01, 0xf8,
	0x7f, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0x7f, 0xff, 0xf8, 0x7e, 0x00, 0x00, 0x7f,
	0x00, 0x00, 0x7f, 0x80, 0x20, 0x3f, 0xff, 0xe0, 0x3f, 0xff, 0xe0, 0x1f, 0xff, 0xe0, 0x0f, 0xff,
	0xe0, 0x01, 0xff, 0xc0
};

DisplayN5110::DisplayN5110(): Adafruit_PCD8544(TFT_DC, TFT_CS, TFT_RST) {

}

char* DisplayN5110::utf8Rus(const char* str, bool uppercase) {
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
      if (rus == 208 && (byte) * (iter + 1) == 129) {
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

void DisplayN5110::apScreen() {
  setTextSize(1);
  setTextColor(TFT_FG, TFT_BG);
  setFont(&TinyFont6);
  setCursor(TFT_FRAMEWDT, TFT_FRAMEWDT + 1 * TFT_LINEHGHT+6);
  print("AP NAME: ");
  print(apSsid);
  setCursor(TFT_FRAMEWDT, TFT_FRAMEWDT + 2 * TFT_LINEHGHT+6);
  print("PASSWD: ");
  print(apPassword);
  setTextColor(SILVER, TFT_BG);
  setCursor(TFT_FRAMEWDT, sheight - 10);
  print("SETTINGS PAGE ON: ");
  setCursor(TFT_FRAMEWDT, sheight-2);
  print("http://");
  print(WiFi.softAPIP().toString().c_str());
  print("/");
  setFont();
}

void DisplayN5110::initD(uint16_t &screenwidth, uint16_t &screenheight) {
  begin();
  setContrast(TFT_CONTRAST);
  cp437(true);
  fillScreen(TFT_BG);
  setRotation(TFT_ROTATE);
  setTextWrap(false);
  screenwidth = width();
  screenheight = height();
  swidth = screenwidth;
  sheight = screenheight;
}

void DisplayN5110::drawLogo() {
  clearDisplay();
  drawBitmap((width()  - LOGO_WIDTH ) / 2, 0, logo, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display();
}

void DisplayN5110::drawPlaylist(uint16_t currentItem, char* currentItemText) {
  for (byte i = 0; i < PLMITEMS; i++) {
    plMenu[i][0] = '\0';
  }
  config.fillPlMenu(plMenu, currentItem - 3, PLMITEMS);
  setTextSize(1);
  int yStart = (sheight / 2 - PLMITEMHEIGHT / 2) - PLMITEMHEIGHT * (PLMITEMS - 1) / 2 + 3;
  fillRect(0, (sheight / 2 - PLMITEMHEIGHT / 2) + 1, swidth, PLMITEMHEIGHT, TFT_LOGO);
  setTextColor(TFT_FG, TFT_BG);
  for (byte i = 0; i < PLMITEMS; i++) {
    if (i == 3) {
      strlcpy(currentItemText, plMenu[i], PLMITEMLENGHT - 1);
    } else {
      setCursor(TFT_FRAMEWDT, yStart + i * PLMITEMHEIGHT);
      print(utf8Rus(plMenu[i], true));
    }
  }
}

void DisplayN5110::clearDsp() {
  fillScreen(TFT_BG);
}

void DisplayN5110::drawScrollFrame(uint16_t texttop, uint16_t textheight, uint16_t bg) {
  if (TFT_FRAMEWDT == 0) return;
  fillRect(0, texttop, TFT_FRAMEWDT, textheight, bg);
  fillRect(swidth - TFT_FRAMEWDT, texttop, TFT_FRAMEWDT, textheight, bg);
}

void DisplayN5110::getScrolBbounds(const char* text, const char* separator, byte textsize, uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth) {
  int16_t  x1, y1;
  uint16_t w, h;
  setTextSize(textsize);
  getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tWidth = w;
  tHeight = h;
  getTextBounds(separator, 0, 0, &x1, &y1, &w, &h);
  sWidth = w;
}

void DisplayN5110::clearScroll(uint16_t texttop, uint16_t textheight, uint16_t bg) {
  fillRect(0,  texttop, swidth, textheight, bg);
}

void DisplayN5110::centerText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  int16_t  x1, y1;
  uint16_t w, h;
  const char* txt = text;

  if(y==90) {
    y=sheight-TFT_LINEHGHT*2;
  }
  if(y==110) {
    y=sheight;
    setFont(&TinyFont5);
  }
  getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  setTextColor(fg);
  setCursor((swidth - w) / 2, y);
  fillRect(0, y, swidth, h, bg);
  print(utf8Rus(txt, true));
  setFont();
}

void DisplayN5110::rightText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  int16_t  x1, y1;
  uint16_t w, h;
  getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  setTextColor(fg);
  setCursor(swidth - w - TFT_FRAMEWDT, y+h+1);
  fillRect(swidth - w - TFT_FRAMEWDT, y, w, h+1, bg);
  print(text);
}

void DisplayN5110::displayHeapForDebug() {

}

void DisplayN5110::printClock(const char* timestr) {
  int16_t  x1, y1;
  uint16_t w, h;
  setTextSize(1);
  setFont(&DS_DIGI15pt7b);
  getTextBounds(timestr, 0, 0, &x1, &y1, &w, &h);
  setTextColor(TFT_FG);
  setCursor((swidth - w) / 2, 19+17);
  fillRect(0, 18, swidth, h+2, TFT_BG);
  print(timestr);
  setFont();
}

void DisplayN5110::drawVolumeBar(bool withNumber) {
  int16_t vTop = sheight - 3;
  int16_t vWidth = swidth;
  uint8_t ww = map(config.store.volume, 0, 254, 0, vWidth - 2);
  fillRect(TFT_FRAMEWDT, vTop, vWidth, 3, TFT_BG);
  drawRect(TFT_FRAMEWDT, vTop, vWidth, 3, TFT_LOGO);
  fillRect(TFT_FRAMEWDT + 1, vTop + 1, ww, 1, TFT_LOGO);
  if (withNumber) {
    setTextSize(1);
    setTextColor(TFT_FG);
    char volstr[4];
    uint16_t wv, hv;
    int16_t  x1, y1;
    sprintf(volstr, "%d", config.store.volume);
    setFont(&DS_DIGI15pt7b);
    getTextBounds(volstr, 0, 0, &x1, &y1, &wv, &hv);
    fillRect(TFT_FRAMEWDT, 24-10, swidth - TFT_FRAMEWDT / 2, hv + 3, TFT_BG);
    setCursor((swidth - wv) / 2, 24+8);
    print(volstr);
    setFont();
  }
}

void DisplayN5110::frameTitle(const char* str) {
  setTextSize(1);
  centerText(str, TFT_FRAMEWDT, TFT_LOGO, TFT_BG);
}

void DisplayN5110::rssi(const char* str) {
  char buf[4];
  strlcpy(buf, str, strlen(str)-2);
  int16_t vTop = sheight - TFT_LINEHGHT - 2;
  setTextSize(1);
  setFont(&TinyFont5);
  rightText(buf, vTop, SILVER, TFT_BG);
  setFont();
}

void DisplayN5110::ip(const char* str) {
  int16_t vTop = sheight - TFT_LINEHGHT - 2;
  setTextSize(1);
  setTextColor(SILVER, TFT_BG);
  setCursor(0, vTop);
  setFont(&TinyFont5);
  print(str);
  setFont();
}

void DisplayN5110::set_TextSize(uint8_t s) {
  setTextSize(s);
}

void DisplayN5110::set_TextColor(uint16_t fg, uint16_t bg) {
  setTextColor(fg, bg);
}

void DisplayN5110::set_Cursor(int16_t x, int16_t y) {
  setCursor(x, y);
}

void DisplayN5110::printText(const char* txt) {
  print(txt);
}

void DisplayN5110::loop() {
  if (checkdelay(83, loopdelay)) {
    display();
  }
  yield();
}

boolean DisplayN5110::checkdelay(int m, unsigned long &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

#endif
