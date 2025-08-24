#include "../core/options.h"
#if DSP_MODEL==DSP_SSD1327
#include "dspcore.h"
#include <Wire.h>
#include "../core/config.h"

#ifndef SCREEN_ADDRESS
  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; or scan it https://create.arduino.cc/projecthub/abdularbi17/how-to-scan-i2c-address-in-arduino-eaadda
#endif

#ifndef I2CFREQ_HZ
  #define I2CFREQ_HZ   6000000UL
#endif

TwoWire tw = TwoWire(0);

DspCore::DspCore(): Adafruit_SSD1327(128, 128, &tw, I2C_RST/*, I2CFREQ_HZ*/) {}

#define CLR_ITEM1    0xA
#define CLR_ITEM2    0x8
#define CLR_ITEM3    0x5
void DspCore::initDisplay() {
  tw.begin(I2C_SDA, I2C_SCL);
  if (!begin(SCREEN_ADDRESS)) {
    Serial.println(F("SSD1327 allocation failed"));
    for (;;);
  }
  config.theme.background = TFT_BG;
  config.theme.meta       = TFT_BG;
  config.theme.metabg     = TFT_LOGO;
  config.theme.metafill   = TFT_LOGO;
  config.theme.title1     = TFT_LOGO;
  config.theme.title2     = SILVER;
  config.theme.clock      = TFT_LOGO;
  config.theme.clockbg    = DARK_GRAY;
  config.theme.rssi       = TFT_FG;
  config.theme.weather    = ORANGE;
  config.theme.ip         = SILVER;
  config.theme.vol        = SILVER;
  config.theme.bitrate    = TFT_LOGO;
  config.theme.digit      = TFT_LOGO;
  config.theme.buffer     = TFT_FG;
  config.theme.volbarout  = TFT_FG;
  config.theme.volbarin   = SILVER;
  config.theme.playlist[0] = CLR_ITEM1;
  config.theme.playlist[1] = CLR_ITEM2;
  config.theme.playlist[2] = CLR_ITEM3;
  config.theme.playlist[3] = CLR_ITEM3;
  config.theme.playlist[4] = CLR_ITEM3;
  cp437(true);
  flip();
  invert();
  setTextWrap(false);
}

void DspCore::clearDsp(bool black){ fillScreen(black?0:config.theme.background); }
void DspCore::flip(){ if(ROTATE_90) setRotation(config.store.flipscreen?3:1); else setRotation(config.store.flipscreen?2:0); }
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ oled_command(SSD1327_DISPLAYOFF); }
void DspCore::wake(void){ oled_command(SSD1327_DISPLAYON); }

#endif
