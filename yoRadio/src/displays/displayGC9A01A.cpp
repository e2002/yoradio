#include "../core/options.h"
#if DSP_MODEL==DSP_GC9A01A

#include "displayGC9A01A.h"
//#include <SPI.h>
#include "fonts/bootlogo.h"
#include "../core/player.h"
#include "../core/config.h"
#include "../core/network.h"

#ifndef DEF_SPI_FREQ
  #define DEF_SPI_FREQ        40000000UL      /*  set it to 0 for system default */
#endif

#define TAKE_MUTEX() if(player.mutex_pl) xSemaphoreTake(player.mutex_pl, portMAX_DELAY)
#define GIVE_MUTEX() if(player.mutex_pl) xSemaphoreGive(player.mutex_pl)

#if DSP_HSPI
DspCore::DspCore(): Adafruit_GC9A01A(&SPI2, TFT_CS, TFT_DC, TFT_RST) {}
#else
DspCore::DspCore(): Adafruit_GC9A01A(TFT_CS, TFT_DC, TFT_RST) {}
#endif

#include "tools/utf8RusGFX.h"

void DspCore::initDisplay() {
  begin();
  if(DEF_SPI_FREQ > 0) setSPISpeed(DEF_SPI_FREQ);
  invert();
  cp437(true);
  flip();
  setTextWrap(false);
  setTextSize(1);
  fillScreen(0x0000);
  
  plItemHeight = playlistConf.widget.textsize*(CHARHEIGHT-1)+playlistConf.widget.textsize*4;
  plTtemsCount = round((float)height()/plItemHeight);
  if(plTtemsCount%2==0) plTtemsCount++;
  plCurrentPos = plTtemsCount/2;
  plYStart = (height() / 2 - plItemHeight / 2) - plItemHeight * (plTtemsCount - 1) / 2 + playlistConf.widget.textsize*2;
}

void DspCore::drawLogo(uint16_t top) { drawRGBBitmap((width() - 99) / 2, top, bootlogo2, 99, 64); }

void DspCore::printPLitem(uint8_t pos, const char* item, ScrollWidget& current){
  setTextSize(playlistConf.widget.textsize);
  if (pos == plCurrentPos) {
    current.setText(item);
  } else {
    uint8_t plColor = (abs(pos - plCurrentPos)-1)>4?4:abs(pos - plCurrentPos)-1;
    setTextColor(config.theme.playlist[plColor], config.theme.background);
    setCursor(TFT_FRAMEWDT, plYStart + pos * plItemHeight);
    fillRect(0, plYStart + pos * plItemHeight - 1, width(), plItemHeight - 2, config.theme.background);
    print(utf8Rus(item, true));
  }
}

void DspCore::drawPlaylist(uint16_t currentItem) {
  uint8_t lastPos = config.fillPlMenu(currentItem - plCurrentPos, plTtemsCount);
  if(lastPos<plTtemsCount){
    fillRect(0, lastPos*plItemHeight+plYStart, width(), height()/2, config.theme.background);
  }
}

void DspCore::clearDsp(bool black) { fillScreen(black?0:config.theme.background); }

GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
  return gfxFont->glyph + c;
}

uint8_t DspCore::_charWidth(unsigned char c){
  GFXglyph *glyph = pgm_read_glyph_ptr(&DS_DIGI42pt7b, c - 0x20);
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
  setTextSize(3);
  setTextColor(config.theme.seconds, config.theme.background);
  setCursor(width() - 8 - clockRightSpace - CHARWIDTH*3*2, clockTop-clockTimeHeight+1);
  sprintf(_bufforseconds, "%02d", network.timeinfo.tm_sec);
  print(_bufforseconds);                                      /* print seconds */
  setTextSize(1);
  setFont(&DS_DIGI42pt7b);
  setTextColor((network.timeinfo.tm_sec % 2 == 0) ? config.theme.clock : config.theme.background, config.theme.background);
  setCursor(_timeleft+_dotsLeft, clockTop);
  print(":");                                     /* print dots */
  setFont();
}

void DspCore::_clockDate(){
  if(_olddateleft>0)
    dsp.fillRect(_olddateleft,  clockTop+10, _olddatewidth, CHARHEIGHT, config.theme.background);
  setTextColor(config.theme.date, config.theme.background);
  setCursor(_dateleft, clockTop+10);
  print(_dateBuf);                                            /* print date */
  strlcpy(_oldDateBuf, _dateBuf, sizeof(_dateBuf));
  _olddatewidth = _datewidth;
  _olddateleft = _dateleft;
  setTextSize(3);
  setTextColor(config.theme.dow, config.theme.background);
  setCursor(width() - 8 - clockRightSpace - CHARWIDTH*3*2, clockTop-CHARHEIGHT*3+4);
  print(utf8Rus(dow[network.timeinfo.tm_wday], false));       /* print dow */
}

void DspCore::_clockTime(){
  if(_oldtimeleft>0) dsp.fillRect(_oldtimeleft, clockTop-clockTimeHeight+1, _oldtimewidth, clockTimeHeight, config.theme.background);
  _timeleft = width()-clockRightSpace-CHARWIDTH*3*2-24-_timewidth;
  clearClock();
  setTextSize(1);
  setFont(&DS_DIGI42pt7b);
  setTextColor(config.theme.clock, config.theme.background);
  setCursor(_timeleft, clockTop);
  print(_timeBuf);
  setFont();
  strlcpy(_oldTimeBuf, _timeBuf, sizeof(_timeBuf));
  _oldtimewidth = _timewidth;
  _oldtimeleft = _timeleft;
  drawFastVLine(width()-clockRightSpace-CHARWIDTH*3*2-18, clockTop-clockTimeHeight, clockTimeHeight+3, config.theme.div);  /*divider vert*/
  drawFastHLine(width()-clockRightSpace-CHARWIDTH*3*2-18, clockTop-clockTimeHeight+29, 44, config.theme.div);              /*divider hor*/
  sprintf(_buffordate, "%2d %s %d", network.timeinfo.tm_mday,mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year+1900);
  strlcpy(_dateBuf, utf8Rus(_buffordate, true), sizeof(_dateBuf));
  _datewidth = strlen(_dateBuf) * CHARWIDTH;
  //_dateleft = width() - 8 - clockRightSpace - _datewidth;
  _dateleft = (width() - _datewidth)/2;
}

void DspCore::printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw){
  clockTop = top;
  clockRightSpace = (width()-_timewidth-CHARWIDTH*3*2-36)/2;//rightspace;
  clockTimeHeight = timeheight;
  strftime(_timeBuf, sizeof(_timeBuf), "%H:%M", &network.timeinfo);
  if(strcmp(_oldTimeBuf, _timeBuf)!=0 || redraw){
    _getTimeBounds();
    clockRightSpace = (width()-_timewidth-CHARWIDTH*3*2-36)/2;
    _clockTime();
    /*if(strcmp(_oldDateBuf, _dateBuf)!=0 || redraw) */_clockDate();
  }
  _clockSeconds();
}

void DspCore::clearClock(){
  if(_oldtimeleft>0){
    dsp.fillRect(_oldtimeleft,  clockTop-clockTimeHeight, _oldtimewidth+CHARWIDTH*3*2+24, clockTimeHeight+10+CHARHEIGHT, config.theme.background);
  }else{
    dsp.fillRect(_timeleft,  clockTop-clockTimeHeight, _timewidth+CHARWIDTH*3*2+24, clockTimeHeight+10+CHARHEIGHT, config.theme.background);
  }
}

void DspCore::startWrite(void) {
  TAKE_MUTEX();
  Adafruit_GC9A01A::startWrite();
}

void DspCore::endWrite(void) {
  Adafruit_GC9A01A::endWrite();
  GIVE_MUTEX();
}

void DspCore::loop(bool force) {

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
  invertDisplay(!config.store.invertdisplay);
}

void DspCore::sleep(void) { 
  enableSleep(true); delay(150); enableDisplay(false); delay(150);
}
void DspCore::wake(void) { 
  enableDisplay(true); delay(150); enableSleep(false); delay(150);
}


void DspCore::writePixel(int16_t x, int16_t y, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x > _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height)) return;
  }
  Adafruit_GC9A01A::writePixel(x, y, color);
}

void DspCore::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x >= _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height))  return;
  }
  Adafruit_GC9A01A::writeFillRect(x, y, w, h, color);
}

void DspCore::setClipping(clipArea ca){
  _cliparea = ca;
  _clipping = true;
}

void DspCore::clearClipping(){
  _clipping = false;
}

void DspCore::setNumFont(){
  setFont(&DS_DIGI42pt7b);
  setTextSize(1);
}
#endif
