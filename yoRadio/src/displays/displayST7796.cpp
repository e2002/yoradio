#include "../core/options.h"
#if DSP_MODEL==DSP_ST7796
#include "dspcore.h"
#include "../core/config.h"

#if DSP_HSPI
DspCore::DspCore(): Adafruit_ST7796S_kbv(&SPI2, TFT_DC, TFT_CS, TFT_RST) {}
#else
DspCore::DspCore(): Adafruit_ST7796S_kbv(TFT_CS, TFT_DC, TFT_RST) {}
#endif

void DspCore::initDisplay() {
  begin();
  invert();
  cp437(true);
  flip();
  setTextWrap(false);
  setTextSize(1);
  fillScreen(0x0000);
}

void DspCore::clearDsp(bool black){ fillScreen(black?0:config.theme.background); }
void DspCore::flip(){ setRotation(config.store.flipscreen?3:1); }
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ sendCommand(ST7796S_SLPIN); delay(150); sendCommand(ST7796S_DISPOFF); delay(150); }
void DspCore::wake(void){ sendCommand(ST7796S_DISPON); delay(150); sendCommand(ST7796S_SLPOUT); delay(150); }

#endif
