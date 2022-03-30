#include "../../options.h"
#if DSP_MODEL==DSP_SSD1327

#include "displaySSD1327.h"
#include <Wire.h>
#include "fonts/bootlogo.h"
#include "../../player.h"
#include "../../config.h"
#include "../../network.h"

#ifndef SCREEN_ADDRESS
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; or scan it https://create.arduino.cc/projecthub/abdularbi17/how-to-scan-i2c-address-in-arduino-eaadda
#endif


#ifndef I2CFREQ_HZ
#define I2CFREQ_HZ   4000000UL
#endif

TwoWire tw = TwoWire(0);

DspCore::DspCore(): Adafruit_SSD1327(128, 128, &tw, I2C_RST, I2CFREQ_HZ) {

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
  setCursor(TFT_FRAMEWDT, TFT_FRAMEWDT + ((DSP_MODEL==DSP_SSD1306)?2:1) * TFT_LINEHGHT);
  print("AP NAME: ");
  print(apSsid);
  setCursor(TFT_FRAMEWDT, TFT_FRAMEWDT + ((DSP_MODEL==DSP_SSD1306)?3:2) * TFT_LINEHGHT);
  print("PASSWORD: ");
  print(apPassword);
  setTextColor(SILVER, TFT_BG);
  setCursor(TFT_FRAMEWDT, sheight - TFT_LINEHGHT * 2);
  print("SETTINGS PAGE ON: ");
  setCursor(TFT_FRAMEWDT, sheight - TFT_LINEHGHT);
  print("http://");
  print(WiFi.softAPIP().toString().c_str());
  print("/");
}

void DspCore::initD(uint16_t &screenwidth, uint16_t &screenheight) {
  tw.begin(I2C_SDA, I2C_SCL);
  if (!begin(SCREEN_ADDRESS)) {
    Serial.println(F("SSD1327 allocation failed"));
    for (;;);
  }
  cp437(true);
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
  drawRGBBitmap((swidth - 99) / 2, 18, bootlogo2, 99, 64);
}

#define CLR_ITEM1    0xA
#define CLR_ITEM2    0x8
#define CLR_ITEM3    0x5
void DspCore::drawPlaylist(uint16_t currentItem, char* currentItemText) {
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
  //display();
}

void DspCore::clearDsp() {
  fillScreen(TFT_BG);
  //clearDisplay();
}

void DspCore::drawScrollFrame(uint16_t texttop, uint16_t textheight, uint16_t bg) {
  if (TFT_FRAMEWDT == 0) return;
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
  fillRect(0,  texttop, swidth, textheight, bg);
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
  setTextColor(fg, bg);
  setCursor(swidth - w - TFT_FRAMEWDT, y);
  fillRect(swidth - w - TFT_FRAMEWDT, y, w, h, bg);
  print(text);
  display();
}

void DspCore::displayHeapForDebug() {

}

void DspCore::setClockBounds(){
  setFont(&DS_DIGI28pt7b);
  setTextSize(1);
  getTextBounds("88:88", 0, 0, &x, &y, &cwidth, &cheight);
  uint16_t header = TFT_FRAMEWDT + 4 * TFT_LINEHGHT;
  uint16_t footer = TFT_FRAMEWDT * 2 + TFT_LINEHGHT + 5;
  clockY = header + (sheight - header - footer) / 2 - cheight / 2;
  setFont();
}

void DspCore::printClock(const char* timestr) {
  uint16_t ncwidth, ncheight;
  setFont(&DS_DIGI28pt7b);
  setTextSize(1);
  getTextBounds(oldTimeBuf, 0, 0, &x, &y, &wot, &hot);
  setCursor((swidth - wot) / 2 - 4, clockY+28+6);
  setTextColor(TFT_BG);
  print(oldTimeBuf);
  strlcpy(oldTimeBuf, timestr, 20);
  setTextColor(TFT_LOGO);
  //fillRect(0, clockY, swidth, cheight + 3, TFT_BG);
  getTextBounds(timestr, 0, 0, &x, &y, &ncwidth, &ncheight);
  setCursor((swidth - ncwidth) / 2 - 4, clockY+28+6);
  print(timestr);
  setFont();
  display();
}

void DspCore::drawVolumeBar(bool withNumber) {
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
    display();
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
  fillRect(TFT_FRAMEWDT, 48, swidth - TFT_FRAMEWDT / 2, hv + 3, TFT_BG);
  setCursor((swidth - wv) / 2, 48 + hv);
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

void DspCore::loop() {
  if (checkdelay(LOOP_DELAY, loopdelay)) {
    display();
  }
  yield();
}

boolean DspCore::checkdelay(int m, unsigned long &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

#endif
