#include "../core/options.h"
#if DSP_MODEL==DSP_GC9A01A
#include "dspcore.h"
#include "../core/config.h"

#if DSP_HSPI
DspCore::DspCore(): Adafruit_GC9A01A(&SPI2, TFT_CS, TFT_DC, TFT_RST) {}
#else
DspCore::DspCore(): Adafruit_GC9A01A(TFT_CS, TFT_DC, TFT_RST) {}
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
void DspCore::flip(){ if(ROTATE_90) setRotation(config.store.flipscreen?3:1); else setRotation(config.store.flipscreen?2:0); }
void DspCore::invert(){ invertDisplay(!config.store.invertdisplay); }
void DspCore::sleep(void){enableSleep(true); delay(150); enableDisplay(false); delay(150); }
void DspCore::wake(void){ enableDisplay(true); delay(150); enableSleep(false); delay(150); }

#endif
