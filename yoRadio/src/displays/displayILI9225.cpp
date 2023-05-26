#include "../core/options.h"
#if DSP_MODEL==DSP_ILI9225

#include "displayILI9225.h"
#include <SPI.h>
#include "fonts/bootlogo.h"
#include "../core/config.h"
#include "../core/network.h"
#include "../core/spidog.h"

extern unsigned char yofont5x7[];
extern unsigned char yofont10x14[];

#define TAKE_MUTEX() sdog.takeMutex()
#define GIVE_MUTEX() sdog.giveMutex()

DspCore::DspCore(): TFT_22_ILI9225(TFT_RST, TFT_DC, TFT_CS, 0) {}

#include "tools/utf8RusGFX.h"

void DspCore::setTextSize(uint8_t s){
  if(s==2){
    setFont(yofont10x14, true);
  }else{
    setFont(yofont5x7, true);
  }
}

void DspCore::setTextColor(uint16_t fg, uint16_t bg){
  _bgcolor=bg;
  setBackgroundColor(_bgcolor);
  _fgcolor=fg;
}

void DspCore::setCursor(int16_t x, int16_t y){
  _cursorx=x;
  _cursory=y;
}

uint16_t DspCore::print(const char* s){
  
  if(_gFont){
    TAKE_MUTEX();
    drawGFXText(_cursorx, _cursory, s, _fgcolor);
    GIVE_MUTEX();
    return 0;
  }else{
    _cursorx=drawText(_cursorx, _cursory, s, _fgcolor);
    //GIVE_MUTEX();
    return _cursorx;
  }
}

void DspCore::setFont(uint8_t* font, bool monoSp) {
  _gFont = false;
  TFT_22_ILI9225::setFont(font, monoSp);
}

void DspCore::setFont(const GFXfont *f) {
  if (f) {
    _gFont = true;
    setGFXFont(f);
  } else {
    setFont(yofont5x7, false);
  }
}

void DspCore::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if(y<0){
    h=h+y;
    y=0;
  }
  TAKE_MUTEX();
  fillRectangle(x, y, x+w, y+h, color);
  GIVE_MUTEX();
}

void DspCore::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
  TAKE_MUTEX();
  drawRectangle(x, y, x+w, y+h, color); 
  GIVE_MUTEX();
}

void DspCore::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color){
  TAKE_MUTEX();
  drawLine(x, y, x, y+h, color);
  GIVE_MUTEX();
}

void DspCore::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color){
  TAKE_MUTEX();
  drawLine(x, y, x+w, y, color);
  GIVE_MUTEX();
}
      
void DspCore::initDisplay() {
//  TAKE_MUTEX();
#if DSP_HSPI
  begin(SPI2);
#else
  begin();
#endif
  invert();
  flip();
  setTextSize(1);
//  GIVE_MUTEX();
  plItemHeight = playlistConf.widget.textsize*(CHARHEIGHT-1)+playlistConf.widget.textsize*4;
  plTtemsCount = round((float)height()/plItemHeight);
  if(plTtemsCount%2==0) plTtemsCount++;
  plCurrentPos = plTtemsCount/2;
  plYStart = (height() / 2 - plItemHeight / 2) - plItemHeight * (plTtemsCount - 1) / 2 + playlistConf.widget.textsize*2;
}

void DspCore::drawLogo(uint16_t top) {
  TAKE_MUTEX();
  drawBitmap((width() - 99) / 2, top, bootlogo2, 99, 64);
  GIVE_MUTEX();
}

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

void DspCore::clearDsp(bool black) {
  TAKE_MUTEX();
  clear(black?0x0000:config.theme.background);
  GIVE_MUTEX();
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
  setTextSize(2);
  setTextColor(config.theme.seconds, config.theme.background);
  setCursor(width() - 8 - clockRightSpace - CHARWIDTH*2*2, clockTop-clockTimeHeight+1);
  sprintf(_bufforseconds, "%02d", network.timeinfo.tm_sec);
  print(_bufforseconds);                                      /* print seconds */
}

void DspCore::_clockDate(){
  if(_olddateleft>0)
    fillRect(_olddateleft,  clockTop+10, _olddatewidth, CHARHEIGHT, config.theme.background);
  setTextColor(config.theme.date, config.theme.background);
  setCursor(_dateleft, clockTop+10);
  print(_dateBuf);                                            /* print date */
  strlcpy(_oldDateBuf, _dateBuf, sizeof(_dateBuf));
  _olddatewidth = _datewidth;
  _olddateleft = _dateleft;
  setTextSize(2);
  setTextColor(config.theme.dow, config.theme.background);
  setCursor(width() - 8 - clockRightSpace - CHARWIDTH*2*2, clockTop-CHARHEIGHT*2+4);
  print(utf8Rus(dow[network.timeinfo.tm_wday], false));       /* print dow */
}

void DspCore::_clockTime(){
  if(_oldtimeleft>0 && !CLOCKFONT_MONO) fillRect(_oldtimeleft, clockTop-clockTimeHeight+1, _oldtimewidth, clockTimeHeight, config.theme.background);
  _timeleft = width()-clockRightSpace-CHARWIDTH*2*2-24-_timewidth;
  setTextSize(1);
  setFont(&DS_DIGI28pt7b);
  
  if(CLOCKFONT_MONO) {
    setCursor(_timeleft, clockTop);
    setTextColor(config.theme.clockbg, config.theme.background);
    print("88:88");
  }
  setTextColor(config.theme.clock, config.theme.background);
  setCursor(_timeleft, clockTop);
  print(_timeBuf);
  setFont();
  strlcpy(_oldTimeBuf, _timeBuf, sizeof(_timeBuf));
  _oldtimewidth = _timewidth;
  _oldtimeleft = _timeleft;
  drawFastVLine(width()-clockRightSpace-CHARWIDTH*2*2-18, clockTop-clockTimeHeight, clockTimeHeight+3, config.theme.div);  /*divider vert*/
  drawFastHLine(width()-clockRightSpace-CHARWIDTH*2*2-18, clockTop-clockTimeHeight+21, 32, config.theme.div);              /*divider hor*/
  sprintf(_buffordate, "%2d %s %d", network.timeinfo.tm_mday,mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year+1900);
  strlcpy(_dateBuf, utf8Rus(_buffordate, true), sizeof(_dateBuf));
  _datewidth = strlen(_dateBuf) * CHARWIDTH;
  _dateleft = width() - 8 - clockRightSpace - _datewidth;
}

void DspCore::printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw){
  clockTop = top;
  clockRightSpace = rightspace;
  clockTimeHeight = timeheight;
  strftime(_timeBuf, sizeof(_timeBuf), "%H:%M", &network.timeinfo);
  if(strcmp(_oldTimeBuf, _timeBuf)!=0 || redraw){
    _getTimeBounds();
    _clockTime();
    if(strcmp(_oldDateBuf, _dateBuf)!=0 || redraw) _clockDate();
  }
  _clockSeconds();
}

void DspCore::clearClock(){
  fillRect(_timeleft,  clockTop-clockTimeHeight, _timewidth+CHARWIDTH*3*2+24, clockTimeHeight+11+CHARHEIGHT, config.theme.background);
}

void DspCore::startWrite(void) {
  //TAKE_MUTEX();
  TFT_22_ILI9225::startWrite();
}

void DspCore::endWrite(void) {
  TFT_22_ILI9225::endWrite();
  //GIVE_MUTEX();
}

void DspCore::loop(bool force) { 
  //delay(5);
}

void DspCore::charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
}

void DspCore::drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h) {
  TAKE_MUTEX();
  drawBitmap(x, y, bitmap, w, h);
  GIVE_MUTEX();
}

void DspCore::flip(){
  TAKE_MUTEX();
  setOrientation(config.store.flipscreen?3:1);
  GIVE_MUTEX();
}
void DspCore::invert(){
  TAKE_MUTEX();
  invertDisplay(config.store.invertdisplay);
  GIVE_MUTEX();
}

void DspCore::sleep(void) { TAKE_MUTEX(); setDisplay(false); GIVE_MUTEX(); }
void DspCore::wake(void) { TAKE_MUTEX(); setDisplay(true); GIVE_MUTEX(); }

void DspCore::writePixel(int16_t x, int16_t y, uint16_t color) { }

void DspCore::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { }

uint16_t DspCore::drawChar(uint16_t x, uint16_t y, uint16_t ch, uint16_t color) {
  if(_clipping){
    if ((x < _cliparea.left) || (x >= _cliparea.left+_cliparea.width) || (y < _cliparea.top) || (y > _cliparea.top + _cliparea.height))  {
      return cfont.width;
    }
  }
  TAKE_MUTEX();
  uint16_t ret=TFT_22_ILI9225::drawChar(x, y, ch, color);
  GIVE_MUTEX();
  return ret;
}

void DspCore::setClipping(clipArea ca){
  _cliparea = ca;
  _clipping = true;
}

void DspCore::clearClipping(){
  _clipping = false;
}

void DspCore::setNumFont(){
  setTextSize(1);
  setFont(&DS_DIGI28pt7b);
}

#endif
