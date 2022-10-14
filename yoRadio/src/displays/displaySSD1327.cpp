#include "../core/options.h"
#if DSP_MODEL==DSP_SSD1327

#include "displaySSD1327.h"
#include <Wire.h>
#include "fonts/bootlogo40.h"
#include "../core/player.h"
#include "../core/config.h"
#include "../core/network.h"

#ifndef SCREEN_ADDRESS
  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; or scan it https://create.arduino.cc/projecthub/abdularbi17/how-to-scan-i2c-address-in-arduino-eaadda
#endif

#ifndef I2CFREQ_HZ
  #define I2CFREQ_HZ   6000000UL
#endif

TwoWire tw = TwoWire(0);

DspCore::DspCore(): Adafruit_SSD1327(128, 128, &tw, I2C_RST/*, I2CFREQ_HZ*/) {}

#include "tools/utf8RusGFX.h"

#define CLR_ITEM1    0xA
#define CLR_ITEM2    0x8
#define CLR_ITEM3    0x5
void DspCore::initDisplay() {
  tw.begin(I2C_SDA, I2C_SCL);
  if (!begin(SCREEN_ADDRESS)) {
    Serial.println(F("SSD1327 allocation failed"));
    for (;;);
  }
  config.theme.background = TFT_BG;
  config.theme.meta       = TFT_BG;
  config.theme.metabg     = TFT_LOGO;
  config.theme.metafill   = TFT_LOGO;
  config.theme.title1     = TFT_LOGO;
  config.theme.title2     = SILVER;
  config.theme.clock      = TFT_LOGO;
  config.theme.rssi       = TFT_FG;
  config.theme.weather    = ORANGE;
  config.theme.ip         = SILVER;
  config.theme.vol        = SILVER;
  config.theme.bitrate    = TFT_LOGO;
  config.theme.digit      = TFT_LOGO;
  config.theme.buffer     = TFT_FG;
  config.theme.volbarout  = TFT_FG;
  config.theme.volbarin   = SILVER;
  config.theme.playlist[0] = CLR_ITEM1;
  config.theme.playlist[1] = CLR_ITEM2;
  config.theme.playlist[2] = CLR_ITEM3;
  config.theme.playlist[3] = CLR_ITEM3;
  config.theme.playlist[4] = CLR_ITEM3;
  cp437(true);
  flip();
  invert();
  setTextWrap(false);
}

void DspCore::drawLogo(uint16_t top) {
  drawRGBBitmap((DSP_WIDTH - 62) / 2, top, bootlogo40, 62, 40);
}

void DspCore::drawPlaylist(uint16_t currentItem, char* currentItemText) {
  for (byte i = 0; i < PLMITEMS; i++) {
    plMenu[i][0] = '\0';
  }
  config.fillPlMenu(plMenu, currentItem - 3, PLMITEMS);
  setTextSize(2);
  int yStart = (height() / 2 - PLMITEMHEIGHT / 2) - PLMITEMHEIGHT * (PLMITEMS - 1) / 2 + 3;
  for (byte i = 0; i < PLMITEMS; i++) {
    if (abs(i - 3) == 3) setTextColor(CLR_ITEM3, TFT_BG);
    if (abs(i - 3) == 2) setTextColor(CLR_ITEM2, TFT_BG);
    if (abs(i - 3) == 1) setTextColor(CLR_ITEM1, TFT_BG);
    if (i == 3) {
      strlcpy(currentItemText, plMenu[i], PLMITEMLENGHT - 1);
    } else {
      setCursor(TFT_FRAMEWDT, yStart + i * PLMITEMHEIGHT);
      fillRect(0, yStart + i * PLMITEMHEIGHT - 1, width(), PLMITEMHEIGHT - 4, TFT_BG);
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
  //_clockSeconds();
}

void DspCore::clearClock(){
  dsp.fillRect(_timeleft,  clockTop-clockTimeHeight, _timewidth, clockTimeHeight, config.theme.background);
}

void DspCore::startWrite(void) { }

void DspCore::endWrite(void) { }

void DspCore::loop(bool force) {
  display();
  delay(5);
}

void DspCore::charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
}

void DspCore::setTextSize(uint8_t s){
  Adafruit_GFX::setTextSize(s);
}

void DspCore::flip(){
  if(ROTATE_90){
    setRotation(config.store.flipscreen?3:1);
  }else{
    setRotation(config.store.flipscreen?2:0);
  }
}

void DspCore::invert(){
  invertDisplay(config.store.invertdisplay);
}

void DspCore::sleep(void) { oled_command(SSD1327_DISPLAYOFF); }
void DspCore::wake(void) { oled_command(SSD1327_DISPLAYON); }

void DspCore::writePixel(int16_t x, int16_t y, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x > _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height)) return;
  }
  Adafruit_SSD1327::writePixel(x, y, color);
}

void DspCore::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x >= _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height))  return;
  }
  Adafruit_SSD1327::writeFillRect(x, y, w, h, color);
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
