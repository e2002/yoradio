#include "../core/options.h"
#if DSP_MODEL==DSP_GC9106

#include "displayGC9106.h"
#include "fonts/bootlogo40.h"
#include "../core/player.h"
#include "../core/config.h"
#include "../core/network.h"

#ifndef DEF_SPI_FREQ
  #define DEF_SPI_FREQ        24000000      /*  set it to 0 for system default */
#endif

#define TAKE_MUTEX() if(player.mutex_pl) xSemaphoreTake(player.mutex_pl, portMAX_DELAY)
#define GIVE_MUTEX() if(player.mutex_pl) xSemaphoreGive(player.mutex_pl)

#if DSP_HSPI
DspCore::DspCore(): Adafruit_GC9106Ex(&SPI2, TFT_DC, TFT_CS, TFT_RST) {}
#else
DspCore::DspCore(): Adafruit_GC9106Ex(TFT_CS, TFT_DC, TFT_RST) {}
#endif

#include "tools/utf8RusGFX.h"

void DspCore::initDisplay() {
  begin(DEF_SPI_FREQ);
  cp437(true);
  invert();
  flip();
  setTextWrap(false);
}

void DspCore::drawLogo(uint16_t top) {
  drawRGBBitmap((width() - 62) / 2, 5, bootlogo40, 62, 40);
}

void DspCore::drawPlaylist(uint16_t currentItem, char* currentItemText) {
  for (byte i = 0; i < PLMITEMS; i++) {
    plMenu[i][0] = '\0';
  }
  config.fillPlMenu(plMenu, currentItem - 3, PLMITEMS);
  setTextSize(2);
  int yStart = (height() / 2 - PLMITEMHEIGHT / 2) - PLMITEMHEIGHT * (PLMITEMS - 1) / 2 + 3;
  for (byte i = 0; i < PLMITEMS; i++) {
    if (abs(i - 3) == 3) setTextColor(config.theme.playlist[2], config.theme.background);
    if (abs(i - 3) == 2) setTextColor(config.theme.playlist[1], config.theme.background);
    if (abs(i - 3) == 1) setTextColor(config.theme.playlist[0], config.theme.background);
    if (i == 3) {
      strlcpy(currentItemText, plMenu[i], PLMITEMLENGHT - 1);
    } else {
      setCursor(TFT_FRAMEWDT, yStart + i * PLMITEMHEIGHT);
      fillRect(0, yStart + i * PLMITEMHEIGHT - 1, width(), PLMITEMHEIGHT - 4, config.theme.background);
      print(utf8Rus(plMenu[i], true));
    }
  }
}

void DspCore::clearDsp(bool black) {
  fillScreen(black?0:config.theme.background);
}

GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
  return gfxFont->glyph + c;
}

uint8_t DspCore::_charWidth(unsigned char c){
  GFXglyph *glyph = pgm_read_glyph_ptr(&DS_DIGI28pt7b, c - 0x20);
  return pgm_read_byte(&glyph->xAdvance);
}

uint16_t DspCore::textWidth(const char *txt){
  uint16_t w = 0, l=strlen(txt);
  for(uint16_t c=0;c<l;c++) w+=_charWidth(txt[c]);
  return w;
}

void DspCore::_getTimeBounds() {
  _timewidth = textWidth(_timeBuf);
  char buf[4];
  strftime(buf, 4, "%H", &network.timeinfo);
  _dotsLeft=textWidth(buf);
}

void DspCore::_clockSeconds(){
  setTextSize(1);
  setFont(&DS_DIGI28pt7b);
  setTextColor((network.timeinfo.tm_sec % 2 == 0) ? config.theme.clock : config.theme.background, config.theme.background);
  setCursor(_timeleft+_dotsLeft, clockTop);
  print(":");                                     /* print dots */
  setFont();
}

void DspCore::_clockDate(){ }

void DspCore::_clockTime(){
  if(_oldtimeleft>0) dsp.fillRect(_oldtimeleft,  clockTop-clockTimeHeight+1, _oldtimewidth, clockTimeHeight, config.theme.background);
  _timeleft = (width()/2 - _timewidth/2)+clockRightSpace;
  setTextSize(1);
  setFont(&DS_DIGI28pt7b);
  setTextColor(config.theme.clock, config.theme.background);
  setCursor(_timeleft, clockTop);
  print(_timeBuf);
  setFont();
  strlcpy(_oldTimeBuf, _timeBuf, sizeof(_timeBuf));
  _oldtimewidth = _timewidth;
  _oldtimeleft = _timeleft;
}

void DspCore::printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw){
  clockTop = top;
  clockRightSpace = rightspace;
  clockTimeHeight = timeheight;
  strftime(_timeBuf, sizeof(_timeBuf), "%H:%M", &network.timeinfo);
  if(strcmp(_oldTimeBuf, _timeBuf)!=0 || redraw){
    _getTimeBounds();
    _clockTime();
  }
  _clockSeconds();
}

void DspCore::clearClock(){
  dsp.fillRect(_timeleft,  clockTop-clockTimeHeight, _timewidth+CHARWIDTH*3*2+24, clockTimeHeight+10+CHARHEIGHT, config.theme.background);
}

void DspCore::startWrite(void) {
  TAKE_MUTEX();
  Adafruit_GC9106Ex::startWrite();
}

void DspCore::endWrite(void) {
  Adafruit_GC9106Ex::endWrite();
  GIVE_MUTEX();
}

void DspCore::loop(bool force) { }

void DspCore::charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
}

void DspCore::setTextSize(uint8_t s){
  Adafruit_GFX::setTextSize(s);
}

void DspCore::flip(){
  setRotation(config.store.flipscreen?1:3);
}

void DspCore::invert(){
  invertDisplay(config.store.invertdisplay);
}

void DspCore::sleep(void) { sendCommand(GC9106_SLPIN); delay(150); sendCommand(GC9106_DISPOFF); delay(150); }
void DspCore::wake(void) { sendCommand(GC9106_DISPON); delay(150); sendCommand(GC9106_SLPOUT); delay(150); }


void DspCore::writePixel(int16_t x, int16_t y, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x > _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height)) return;
  }
  Adafruit_GC9106Ex::writePixel(x, y, color);
}

void DspCore::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x >= _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height))  return;
  }
  Adafruit_GC9106Ex::writeFillRect(x, y, w, h, color);
}

void DspCore::setClipping(clipArea ca){
  _cliparea = ca;
  _clipping = true;
}

void DspCore::clearClipping(){
  _clipping = false;
}

void DspCore::setNumFont(){
  setFont(&DS_DIGI28pt7b);
  setTextSize(1);
}

#endif
