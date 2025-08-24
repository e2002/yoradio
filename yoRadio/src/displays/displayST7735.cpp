#include "../core/options.h"
#if DSP_MODEL==DSP_ST7735
#include "dspcore.h"
#include "../core/config.h"

#ifndef DEF_SPI_FREQ
#define DEF_SPI_FREQ        0 //26000000UL      /*  set it to 0 for system default */
#endif

#if DSP_HSPI
DspCore::DspCore(): Adafruit_ST7735(&SPI2, TFT_CS, TFT_DC, TFT_RST) {}
#else
DspCore::DspCore(): Adafruit_ST7735(&SPI, TFT_CS, TFT_DC, TFT_RST) {}
#endif

void DspCore::initDisplay() {
  initR(DTYPE);
  if(DEF_SPI_FREQ > 0) setSPISpeed(DEF_SPI_FREQ);
  cp437(true);
  invert();
  flip();
  setTextWrap(false);
}

void DspCore::clearDsp(bool black){ fillScreen(black?0:config.theme.background); }
void DspCore::flip(){ if(ROTATE_90) setRotation(config.store.flipscreen?2:0); else setRotation(config.store.flipscreen?3:1); }
void DspCore::invert(){ invertDisplay((DTYPE==INITR_MINI160x80)?!config.store.invertdisplay:config.store.invertdisplay); }
void DspCore::sleep(void){ enableSleep(true); delay(150); enableDisplay(false); delay(150); }
void DspCore::wake(void){ enableDisplay(true); delay(150); enableSleep(false); delay(150); }

#endif
