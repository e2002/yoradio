#include "../core/options.h"
#if DSP_MODEL==DSP_SSD1305 || DSP_MODEL==DSP_SSD1305I2C

#include "displaySSD1305.h"
#include "../core/player.h"
#include "../core/config.h"
#include "../core/network.h"

#ifndef SCREEN_ADDRESS
  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32 or scan it https://create.arduino.cc/projecthub/abdularbi17/how-to-scan-i2c-address-in-arduino-eaadda
#endif

#define LOGO_WIDTH 21
#define LOGO_HEIGHT 32

#ifndef DEF_SPI_FREQ
  #define DEF_SPI_FREQ        8000000UL      /*  set it to 0 for system default */
#endif

const unsigned char logo [] PROGMEM=
{
    0x06, 0x03, 0x00, 0x0f, 0x07, 0x80, 0x1f, 0x8f, 0xc0, 0x1f, 0x8f, 0xc0,
    0x0f, 0x07, 0x80, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x03, 0xff, 0x00, 0x0f, 0xff, 0x80,
    0x1f, 0xff, 0xc0, 0x1f, 0xff, 0xc0, 0x3f, 0x8f, 0xe0, 0x7e, 0x03, 0xf0,
    0x7c, 0x01, 0xf0, 0x7c, 0x01, 0xf0, 0x7f, 0xff, 0xf0, 0xff, 0xff, 0xf8,
    0xff, 0xff, 0xf8, 0xff, 0xff, 0xf8, 0x7c, 0x00, 0x00, 0x7c, 0x00, 0x00,
    0x7e, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x3f, 0xc0, 0xe0, 0x3f, 0xff, 0xe0,
    0x1f, 0xff, 0xe0, 0x0f, 0xff, 0xe0, 0x03, 0xff, 0xc0, 0x00, 0xfe, 0x00
};

#if DSP_MODEL==DSP_SSD1305
DspCore::DspCore(): Adafruit_SSD1305(128, 64, &SPI, TFT_DC, TFT_RST, TFT_CS, DEF_SPI_FREQ) {

}
#else
#include <Wire.h>
TwoWire I2CSSD1305 = TwoWire(0);
DspCore::DspCore(): Adafruit_SSD1305(128, 64, &I2CSSD1305, -1){

}
#endif

#include "tools/utf8RusGFX.h"

void DspCore::initDisplay() {
#if DSP_MODEL==DSP_SSD1305I2C
  I2CSSD1305.begin(I2C_SDA, I2C_SCL);
#endif
  if (!begin(SCREEN_ADDRESS)) {
    Serial.println(F("SSD1305 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  config.theme.background = TFT_BG;
  config.theme.meta       = TFT_BG;
  config.theme.clock      = TFT_FG;
  config.theme.weather    = TFT_FG;
  config.theme.metabg     = TFT_FG;
  config.theme.metafill   = TFT_FG;
  config.theme.title1     = TFT_FG;
  config.theme.title2     = TFT_FG;
  config.theme.rssi       = TFT_FG;
  config.theme.ip         = TFT_FG;
  config.theme.vol        = TFT_FG;
  config.theme.bitrate    = TFT_FG;
  config.theme.digit      = TFT_FG;
  config.theme.buffer     = TFT_FG;
  config.theme.volbarout  = TFT_FG;
  config.theme.volbarin   = TFT_FG;
  
  for(byte i=0;i<5;i++) config.theme.playlist[i] = TFT_FG;
  
  cp437(true);
  flip();
  invert();
  setTextWrap(false);
}

void DspCore::drawLogo(uint16_t top) {
  drawBitmap( (width()  - LOGO_WIDTH ) / 2, 8, logo, LOGO_WIDTH, LOGO_HEIGHT, 1);
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

uint8_t DspCore::_charWidth(unsigned char c){
  return CHARWIDTH*clockTimeHeight;
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
  setTextSize(clockTimeHeight);
  setTextColor((network.timeinfo.tm_sec % 2 == 0) ? config.theme.clock : config.theme.background, config.theme.background);
  setCursor(_timeleft+_dotsLeft, clockTop);
  print(":");                                     /* print dots */
  setTextSize(1);
  setCursor(_timeleft+_timewidth+1, clockTop);
  setTextColor(config.theme.clock, config.theme.background);
  sprintf(_bufforseconds, "%02d", network.timeinfo.tm_sec);
  print(_bufforseconds); 
  //setFont();
}

void DspCore::_clockDate(){ }

void DspCore::_clockTime(){
  if(_oldtimeleft>0) dsp.fillRect(_oldtimeleft,  clockTop, _oldtimewidth, clockTimeHeight*CHARHEIGHT, config.theme.background);
  _timeleft = (width()/2 - _timewidth/2)+clockRightSpace;
  setTextColor(config.theme.clock, config.theme.background);
  setTextSize(clockTimeHeight);
  
  setCursor(_timeleft, clockTop);
  print(_timeBuf);
  //setFont();
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
  dsp.fillRect(_timeleft,  clockTop, _timewidth, clockTimeHeight*CHARHEIGHT, config.theme.background);
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
  setRotation(config.store.flipscreen?2:0);
}

void DspCore::invert(){
  invertDisplay(config.store.invertdisplay);
}

void DspCore::sleep(void) { oled_command(SSD1305_DISPLAYOFF); }
void DspCore::wake(void) { oled_command(SSD1305_DISPLAYON); }

void DspCore::writePixel(int16_t x, int16_t y, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x > _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height)) return;
  }
  Adafruit_SSD1305::writePixel(x, y, color);
}

void DspCore::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x >= _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height))  return;
  }
  Adafruit_SSD1305::writeFillRect(x, y, w, h, color);
}

void DspCore::setClipping(clipArea ca){
  _cliparea = ca;
  _clipping = true;
}

void DspCore::clearClipping(){
  _clipping = false;
}

void DspCore::setNumFont(){
  setTextSize(2);
}

#endif
