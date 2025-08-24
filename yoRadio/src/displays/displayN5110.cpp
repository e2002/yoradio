#include "../core/options.h"
#if DSP_MODEL==DSP_NOKIA5110
#include "dspcore.h"
#include <Wire.h>
#include "../core/config.h"

#if DSP_HSPI
DspCore::DspCore(): Adafruit_PCD8544(TFT_DC, TFT_CS, TFT_RST, &SPI2) {}
#else
DspCore::DspCore(): Adafruit_PCD8544(TFT_DC, TFT_CS, TFT_RST) {}
#endif

void DspCore::initDisplay() {
  begin();
  setReinitInterval(0);
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
  config.theme.plcurrent     = TFT_BG;
  config.theme.plcurrentbg   = TFT_FG;
  config.theme.plcurrentfill = TFT_FG;
  for(byte i=0;i<5;i++) config.theme.playlist[i] = TFT_FG;
  
  setContrast(config.store.contrast);
  cp437(true);
  invert();
  flip();
  setTextWrap(false);
}

void DspCore::clearDsp(bool black){ fillScreen(TFT_BG); }
void DspCore::flip(){ setRotation(config.store.flipscreen?2:0); }
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ command( PCD8544_FUNCTIONSET | PCD8544_POWERDOWN); }
void DspCore::wake(void){ initDisplay(); }

#endif
