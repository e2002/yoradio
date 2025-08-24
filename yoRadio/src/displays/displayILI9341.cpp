#include "../core/options.h"
#if DSP_MODEL==DSP_ILI9341
#include "dspcore.h"
#include "../core/config.h"

#if DSP_HSPI
DspCore::DspCore(): Adafruit_ILI9341(&SPI2, TFT_DC, TFT_CS, TFT_RST) {}
#else
DspCore::DspCore(): Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST) {}
#endif

void DspCore::initDisplay() {
  begin();             /* SPI_DEFAULT_FREQ 40000000 */
  invert();
  cp437(true);
  flip();
  setTextWrap(false);
}

void DspCore::clearDsp(bool black){ fillScreen(black?0:config.theme.background); }
void DspCore::flip(){ setRotation(config.store.flipscreen?1:3); }
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ sendCommand(ILI9341_SLPIN); delay(150); sendCommand(ILI9341_DISPOFF); delay(150);}
void DspCore::wake(void){ sendCommand(ILI9341_DISPON); delay(150); sendCommand(ILI9341_SLPOUT); delay(150);}

#endif
