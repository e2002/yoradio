#include "../core/options.h"
#if DSP_MODEL==DSP_SSD1306 || DSP_MODEL==DSP_SSD1306x32
#include "dspcore.h"
#include <Wire.h>
#include "../core/config.h"

#ifndef SCREEN_ADDRESS
  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32 or scan it https://create.arduino.cc/projecthub/abdularbi17/how-to-scan-i2c-address-in-arduino-eaadda
#endif

#ifndef I2CFREQ_HZ
  #define I2CFREQ_HZ   4000000UL
#endif

TwoWire I2CSSD1306 = TwoWire(0);

DspCore::DspCore(): Adafruit_SSD1306(128, ((DSP_MODEL==DSP_SSD1306)?64:32), &I2CSSD1306, I2C_RST, I2CFREQ_HZ) { }

void DspCore::initDisplay() {
  I2CSSD1306.begin(I2C_SDA, I2C_SCL);
  if (!begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
#include "tools/oledcolorfix.h"
  cp437(true);
  flip();
  invert();
  setTextWrap(false);
}

void DspCore::clearDsp(bool black){ fillScreen(TFT_BG); }
void DspCore::flip(){ setRotation(config.store.flipscreen?2:0); }
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ ssd1306_command(SSD1306_DISPLAYOFF); }
void DspCore::wake(void){ ssd1306_command(SSD1306_DISPLAYON); }

#endif
