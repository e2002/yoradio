#include "../core/options.h"

#if DSP_MODEL==DSP_1602I2C || DSP_MODEL==DSP_1602 || DSP_MODEL==DSP_2004 || DSP_MODEL==DSP_2004I2C

#include "displayLC1602.h"
#include "../core/player.h"
#include "../core/config.h"
#include "../core/network.h"

#ifndef SCREEN_ADDRESS
  #define SCREEN_ADDRESS 0x27 ///< See datasheet for Address or scan it https://create.arduino.cc/projecthub/abdularbi17/how-to-scan-i2c-address-in-arduino-eaadda
#endif

DspCore::DspCore(): DSP_INIT {}

#include "tools/utf8RusLCD.h"

void DspCore::apScreen() {
  clear();
  setCursor(0,0);
  print(utf8Rus(const_lcdApMode, false));
  setCursor(0,1);
  print(WiFi.softAPIP().toString().c_str());
#ifdef LCD_2004
  setCursor(0, 2);
  print(utf8Rus(const_lcdApName, false));
  print(apSsid);
  setCursor(0, 3);
  print(utf8Rus(const_lcdApPass, false));
  print(apPassword);
#endif
}

byte Arrow[] PROGMEM = { B00000, B00100, B00010, B01001, B00010, B00100, B00000, B00000 };

void DspCore::initDisplay() {
#ifdef LCD_I2C
  init();
  backlight();
#else
  #ifdef LCD_2004
    begin(20, 4);
  #else
    begin(16, 2);
  #endif
#endif
  clearClipping();
  createChar(0, Arrow);
}

void DspCore::drawLogo(uint16_t top) { }


void DspCore::drawPlaylist(uint16_t currentItem, char* currentItemText) {
  clear();
  for (byte i = 0; i < PLMITEMS; i++) {
    plMenu[i][0] = '\0';
  }
  config.fillPlMenu(plMenu, currentItem-1, PLMITEMS);
  for (byte i = 0; i < PLMITEMS; i++) {
    if (i == 1) {
      strlcpy(currentItemText, plMenu[i], PLMITEMLENGHT - 1);
    } else {
      char tmp[width()] = {0};
      strlcpy(tmp, utf8Rus(plMenu[i], true), width());
      setCursor(1, i);
      print(tmp);
    }
  }
  setCursor(0,1);
  write(byte(0));
}

void DspCore::clearDsp(bool black) {
  clear();
}

void DspCore::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
  if(w<2) return;
  char buf[width()+1] = { 0 };
  snprintf(buf, sizeof(buf), "%*s%s", w-1, "", " ");
  setCursor(x, y);
  print(buf);
  setCursor(x, y);
}

uint16_t DspCore::width(){
#ifdef LCD_2004
  return 20;
#else
  return 16;
#endif
}

uint16_t DspCore::height(){
#ifdef LCD_2004
  return 4;
#else
  return 2;
#endif
}

uint8_t DspCore::_charWidth(unsigned char c){
  return 1;
}

uint16_t DspCore::textWidth(const char *txt){
  uint16_t w = 0, l=strlen(txt);
  for(uint16_t c=0;c<l;c++) w+=_charWidth(txt[c]);
  return w;
}

void DspCore::_getTimeBounds() { }

void DspCore::_clockSeconds(){
  setCursor(_timeleft+_dotsLeft, clockTop);
  print((network.timeinfo.tm_sec % 2 == 0)?":":" ");
}

void DspCore::_clockDate(){ }

void DspCore::_clockTime(){ }

void DspCore::printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw) {
  clockTop = 0;
  _timeleft = width()-5;
  _dotsLeft = 2;
  strftime(_timeBuf, sizeof(_timeBuf), "%H:%M", &network.timeinfo);
  if(strcmp(_oldTimeBuf, _timeBuf)!=0 || redraw){
    setCursor(_timeleft, clockTop);
    print(_timeBuf);
    strlcpy(_oldTimeBuf, _timeBuf, sizeof(_timeBuf));
  }
  _clockSeconds();
}

void DspCore::clearClock() {  }

void DspCore::loop(bool force) { 
//  delay(100);
}

void DspCore::charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
  width = 1;
  height = 1;
}

void DspCore::flip(){ }

void DspCore::invert(){ }

void DspCore::sleep(void) { 
  noDisplay();
#ifdef LCD_I2C
  noBacklight();
#endif
}
void DspCore::wake(void) { 
  display();
#ifdef LCD_I2C
  backlight();
#endif
}

void DspCore::writePixel(int16_t x, int16_t y, uint16_t color) { }

void DspCore::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { }

void DspCore::setClipping(clipArea ca){
  _cliparea = ca;
  _clipping = true;
}

void DspCore::clearClipping(){
  _clipping = false;
  setClipping({0, 0, width(), height()});
}

void DspCore::setNumFont(){ }

#endif
