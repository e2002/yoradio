#include "../core/options.h"
#if DSP_MODEL==DSP_SSD1305 || DSP_MODEL==DSP_SSD1305I2C
#include "dspcore.h"
#include "../core/config.h"

#ifndef SCREEN_ADDRESS
  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32 or scan it https://create.arduino.cc/projecthub/abdularbi17/how-to-scan-i2c-address-in-arduino-eaadda
#endif

#ifndef DEF_SPI_FREQ
  #define DEF_SPI_FREQ        8000000UL      /*  set it to 0 for system default */
#endif

#if DSP_MODEL==DSP_SSD1305
  #if DSP_HSPI
    DspCore::DspCore(): Adafruit_SSD1305(128, 64, &SPI2, TFT_DC, TFT_RST, TFT_CS, DEF_SPI_FREQ) {}
  #else
    DspCore::DspCore(): Adafruit_SSD1305(128, 64, &SPI, TFT_DC, TFT_RST, TFT_CS, DEF_SPI_FREQ) {}
  #endif
#else
#include <Wire.h>
TwoWire I2CSSD1305 = TwoWire(0);
DspCore::DspCore(): Adafruit_SSD1305(128, 64, &I2CSSD1305, -1){

}
#endif

void DspCore::initDisplay() {
#if DSP_MODEL==DSP_SSD1305I2C
  I2CSSD1305.begin(I2C_SDA, I2C_SCL);
#endif
  if (!begin(SCREEN_ADDRESS)) {
    Serial.println(F("SSD1305 allocation failed"));
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
void DspCore::sleep(void){ oled_command(SSD1305_DISPLAYOFF); }
void DspCore::wake(void){ oled_command(SSD1305_DISPLAYON); }

#endif
