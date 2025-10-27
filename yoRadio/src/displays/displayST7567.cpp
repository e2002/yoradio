#include "../core/options.h"
#if DSP_MODEL==DSP_ST7567
#include "dspcore.h"
#include <Wire.h>
#include "../core/config.h"

#define LOGO_WIDTH 21
#define LOGO_HEIGHT 32


#if DSP_HSPI
  DspCore::DspCore(): ST7567(128, 64, &SPI2, TFT_DC, TFT_RST, TFT_CS) {}
#else
  DspCore::DspCore(): ST7567(128, 64, &SPI, TFT_DC, TFT_RST, TFT_CS) {}
#endif

void DspCore::initDisplay() {
#include "tools/oledcolorfix.h"
  begin();
  setContrast(config.store.contrast);
  cp437(true);
  flip();
  invert();
  setTextWrap(false);
}

void DspCore::clearDsp(bool black) {
  fillScreen(TFT_BG);
}

void DspCore::flip(){
  setRotation(config.store.flipscreen?2:0);
}

void DspCore::invert(){
  invertDisplay(config.store.invertdisplay);
}

void DspCore::sleep(void) {
	ST7567::sleep();
}

void DspCore::wake(void) {
	ST7567::wake();
}

#endif
