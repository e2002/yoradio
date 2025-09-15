#include "../core/options.h"
#if DSP_MODEL==DSP_GC9106
#include "dspcore.h"
#include "../core/config.h"

#if DSP_HSPI
DspCore::DspCore(): Adafruit_GC9106Ex(&SPI2, TFT_DC, TFT_CS, TFT_RST) {}
#else
DspCore::DspCore(): Adafruit_GC9106Ex(TFT_CS, TFT_DC, TFT_RST) {}
#endif

void DspCore::initDisplay() {
  begin();
  cp437(true);
  invert();
  flip();
  setTextWrap(false);
}

void DspCore::clearDsp(bool black){ fillScreen(black?0:config.theme.background); }
void DspCore::flip(){ setRotation(config.store.flipscreen?1:3); }
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ sendCommand(GC9106_SLPIN); delay(150); sendCommand(GC9106_DISPOFF); delay(150); }
void DspCore::wake(void){ sendCommand(GC9106_DISPON); delay(150); sendCommand(GC9106_SLPOUT); delay(150); }

#endif
