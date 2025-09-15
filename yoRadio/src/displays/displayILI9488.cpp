#include "../core/options.h"
#if DSP_MODEL==DSP_ILI9488 || DSP_MODEL==DSP_ILI9486
#include "dspcore.h"
#include "../core/config.h"

#if DSP_HSPI
  DspCore::DspCore(): ILI9486_SPI(&SPI2, TFT_CS, TFT_DC, TFT_RST) {}
#else
  DspCore::DspCore(): ILI9486_SPI(TFT_CS, TFT_DC, TFT_RST) {}
#endif

void DspCore::initDisplay() {
  setSpiKludge(false);
  init();
  cp437(true);
  setTextWrap(false);
  setTextSize(1);
  fillScreen(0x0000);
  invert();
  flip();
}

void DspCore::clearDsp(bool black){ fillScreen(black?0:config.theme.background); }
void DspCore::flip(){ setRotation(config.store.flipscreen?3:1); }
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ sendCommand(ILI9488_SLPIN); delay(150); sendCommand(ILI9488_DISPOFF); delay(150); }
void DspCore::wake(void){ sendCommand(ILI9488_DISPON); delay(150); sendCommand(ILI9488_SLPOUT); delay(150); }

#endif
