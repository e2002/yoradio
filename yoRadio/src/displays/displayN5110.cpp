#include "../core/options.h"
#if DSP_MODEL==3

#include "displayN5110.h"
#include <Wire.h>
#include "../core/player.h"
#include "../core/config.h"
#include "../core/network.h"

#define LOGO_WIDTH 21
#define LOGO_HEIGHT 28

#define TAKE_MUTEX() if(player.mutex_pl) xSemaphoreTake(player.mutex_pl, portMAX_DELAY)
#define GIVE_MUTEX() if(player.mutex_pl) xSemaphoreGive(player.mutex_pl)

const unsigned char logo [] PROGMEM=
{
	0x07, 0x03, 0x80, 0x0f, 0x87, 0xc0, 0x0f, 0x87, 0xc0, 0x0f, 0x87, 0xc0, 0x0f, 0x87, 0xc0, 0x07,
	0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x07, 0xff, 0x80, 0x1f, 0xff,
	0xc0, 0x1f, 0xff, 0xe0, 0x3f, 0xff, 0xf0, 0x7f, 0x07, 0xf0, 0x7e, 0x03, 0xf0, 0x7e, 0x01, 0xf8,
	0x7f, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0x7f, 0xff, 0xf8, 0x7e, 0x00, 0x00, 0x7f,
	0x00, 0x00, 0x7f, 0x80, 0x20, 0x3f, 0xff, 0xe0, 0x3f, 0xff, 0xe0, 0x1f, 0xff, 0xe0, 0x0f, 0xff,
	0xe0, 0x01, 0xff, 0xc0
};

DspCore::DspCore(): Adafruit_PCD8544(TFT_DC, TFT_CS, TFT_RST) {

}

#include "tools/utf8RusGFX.h"

void DspCore::command(uint8_t c) {
  TAKE_MUTEX();
  Adafruit_PCD8544::command(c);
  GIVE_MUTEX();
}

void DspCore::data(uint8_t c) {
  TAKE_MUTEX();
  Adafruit_PCD8544::data(c);
  GIVE_MUTEX();
}

void DspCore::initDisplay() {
  begin();
  setReinitInterval(255);
  config.theme.background = TFT_BG;
  config.theme.meta       = TFT_FG;
  config.theme.clock      = TFT_FG;
  config.theme.weather    = TFT_FG;
  config.theme.metabg     = TFT_BG;
  config.theme.metafill   = TFT_BG;
  config.theme.title1     = TFT_FG;
  config.theme.title2     = TFT_FG;
  config.theme.rssi       = TFT_FG;
  config.theme.ip         = TFT_FG;
  config.theme.bitrate    = TFT_FG;
  config.theme.digit      = TFT_FG;
  config.theme.buffer     = TFT_FG;
  config.theme.volbarout  = TFT_FG;
  config.theme.volbarin   = TFT_FG;
  for(byte i=0;i<5;i++) config.theme.playlist[i] = TFT_FG;
  
  setContrast(config.store.contrast);
  cp437(true);
  invert();
  flip();
  setTextWrap(false);
}

void DspCore::drawLogo(uint16_t top) {
  drawBitmap((width()  - LOGO_WIDTH ) / 2, top, logo, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display();
}

void DspCore::drawPlaylist(uint16_t currentItem, char* currentItemText) {
  for (byte i = 0; i < PLMITEMS; i++) {
    plMenu[i][0] = '\0';
  }
  config.fillPlMenu(plMenu, currentItem - 3, PLMITEMS);
  setTextSize(1);
  int yStart = (height() / 2 - PLMITEMHEIGHT / 2) - PLMITEMHEIGHT * (PLMITEMS - 1) / 2 + 3;
  setTextColor(TFT_FG, TFT_BG);
  for (byte i = 0; i < PLMITEMS; i++) {
    if (i == 3) {
      strlcpy(currentItemText, plMenu[i], PLMITEMLENGHT - 1);
    } else {
      setCursor(TFT_FRAMEWDT, yStart + i * PLMITEMHEIGHT);
      fillRect(0, yStart + i * PLMITEMHEIGHT, width(), PLMITEMHEIGHT - 1, TFT_BG);
      print(utf8Rus(plMenu[i], true));
    }
  }
}

void DspCore::clearDsp(bool black) {
  fillScreen(TFT_BG);
}

GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
  return gfxFont->glyph + c;
}

uint8_t DspCore::_charWidth(unsigned char c){
  GFXglyph *glyph = pgm_read_glyph_ptr(&DS_DIGI15pt7b, c - 0x20);
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
  setFont(&DS_DIGI15pt7b);
  setTextColor((network.timeinfo.tm_sec % 2 == 0) ? config.theme.clock : config.theme.background, config.theme.background);
  setCursor(_timeleft+_dotsLeft, clockTop);
  print(":");  
  setFont();                                   /* print dots */
  setTextSize(1);
  setCursor(_timeleft+_timewidth+2, clockTop-CHARHEIGHT*2-1);
  setTextColor(config.theme.clock, config.theme.background);
  sprintf(_bufforseconds, "%02d", network.timeinfo.tm_sec);
  print(_bufforseconds); 
}

void DspCore::_clockDate(){  }

void DspCore::_clockTime(){
  if(_oldtimeleft>0) dsp.fillRect(_oldtimeleft,  clockTop-clockTimeHeight+1, _oldtimewidth+CHARWIDTH*2+2, clockTimeHeight, config.theme.background);
  //if(_oldtimeleft>0) dsp.fillRect(_oldtimeleft, clockTop-clockTimeHeight+1, _oldtimewidth, clockTimeHeight, config.theme.background);
  _timeleft = (width()/2 - _timewidth/2)-clockRightSpace;
  setTextSize(1);
  setFont(&DS_DIGI15pt7b);
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
  dsp.fillRect(_timeleft,  clockTop-clockTimeHeight, _timewidth+CHARWIDTH*2+2, clockTimeHeight, config.theme.background);
  //dsp.fillRect(_timeleft,  clockTop-clockTimeHeight, _timewidth+CHARWIDTH*3*2+24, clockTimeHeight+10+CHARHEIGHT, config.theme.background);
}


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
  setRotation(config.store.flipscreen?2:0);
}

void DspCore::invert(){
  invertDisplay(config.store.invertdisplay);
}

void DspCore::sleep(void) { command( PCD8544_FUNCTIONSET | PCD8544_POWERDOWN); }
void DspCore::wake(void) { initDisplay(); }

void DspCore::writePixel(int16_t x, int16_t y, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x > _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height)) return;
  }
  Adafruit_PCD8544::writePixel(x, y, color);
}

void DspCore::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x >= _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height))  return;
  }
  Adafruit_PCD8544::writeFillRect(x, y, w, h, color);
}

void DspCore::setClipping(clipArea ca){
  _cliparea = ca;
  _clipping = true;
}

void DspCore::clearClipping(){
  _clipping = false;
}

void DspCore::setNumFont(){
  setFont(&DS_DIGI15pt7b);
  setTextSize(1);
}

#endif
