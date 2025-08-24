#include "../core/options.h"
#if DSP_MODEL==DSP_SH1106 || DSP_MODEL==DSP_SH1107
#include "dspcore.h"
#include <Wire.h>
#include "../core/config.h"

#ifndef SCREEN_ADDRESS
  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32 or scan it https://create.arduino.cc/projecthub/abdularbi17/how-to-scan-i2c-address-in-arduino-eaadda
#endif

#ifndef I2CFREQ_HZ
  #define I2CFREQ_HZ   4000000UL
#endif

TwoWire I2CSH1106 = TwoWire(0);
#if DSP_MODEL==DSP_SH1106
DspCore::DspCore(): Adafruit_SH1106G(128, 64, &I2CSH1106, -1, I2CFREQ_HZ) {

}
#else
DspCore::DspCore(): Adafruit_SH1107(64, 128, &I2CSH1106, -1) {

}
#endif

void DspCore::initDisplay() {
  I2CSH1106.begin(I2C_SDA, I2C_SCL);
  if (!begin(SCREEN_ADDRESS, true)) {
    Serial.println(F("SH110X allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
#include "tools/oledcolorfix.h"
  cp437(true);
  flip();
  invert();
  setTextWrap(false);
}

void DspCore::clearDsp(bool black){ fillScreen(TFT_BG); }
void DspCore::flip(){
#if DSP_MODEL==DSP_SH1107
  setRotation(config.store.flipscreen?3:1);
#endif
#if DSP_MODEL==DSP_SH1106
  setRotation(config.store.flipscreen?2:0);
#endif
}
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ oled_command(SH110X_DISPLAYOFF); }
void DspCore::wake(void){ oled_command(SH110X_DISPLAYON); }

#endif
