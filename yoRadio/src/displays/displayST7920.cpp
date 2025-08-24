#include "../core/options.h"
#if DSP_MODEL==DSP_ST7920
#include "dspcore.h"
#include "../core/config.h"

#ifndef DEF_SPI_FREQ
  #define DEF_SPI_FREQ        8000000UL
#endif

#if DSP_HSPI
  DspCore::DspCore(): ST7920(&SPI2, TFT_CS, DEF_SPI_FREQ) {}
#else
  DspCore::DspCore(): ST7920(&SPI, TFT_CS, DEF_SPI_FREQ) {}
#endif

void DspCore::initDisplay() {
#include "tools/oledcolorfix.h"
  begin();
  cp437(true);
  flip();
  invert();
  setTextWrap(false);
}

void DspCore::clearDsp(bool black){ fillScreen(TFT_BG); }
void DspCore::flip(){ setRotation(config.store.flipscreen?2:0); }
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ doSleep(true); }
void DspCore::wake(void){ doSleep(false); }

#endif
