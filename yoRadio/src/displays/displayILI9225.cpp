#include "../core/options.h"
#if DSP_MODEL==DSP_ILI9225
#include "dspcore.h"
#include <SPI.h>
#include "../core/config.h"

extern unsigned char yofont5x7[];
extern unsigned char yofont10x14[];

DspCore::DspCore(): TFT_22_ILI9225(TFT_RST, TFT_DC, TFT_CS, 0) {}

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
    drawGFXText(_cursorx, _cursory, s, _fgcolor);
    return 0;
  }else{
    _cursorx=drawText(_cursorx, _cursory, s, _fgcolor);
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
  fillRectangle(x, y, x+w, y+h, color);
}

void DspCore::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
  drawRectangle(x, y, x+w, y+h, color); 
}

void DspCore::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color){
  drawLine(x, y, x, y+h, color);
}

void DspCore::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color){
  drawLine(x, y, x+w, y, color);
}
      
void DspCore::initDisplay() {
#if DSP_HSPI
  begin(SPI2);
#else
  begin();
#endif
  invert();
  flip();
  setTextSize(1);
}

void DspCore::clearDsp(bool black){ clear(black?0x0000:config.theme.background); }
void DspCore::flip(){ setOrientation(config.store.flipscreen?3:1); }
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ setDisplay(false); }
void DspCore::wake(void){ setDisplay(true); }

#endif
