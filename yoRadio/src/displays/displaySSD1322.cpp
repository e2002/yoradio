#include "../core/options.h"
#if DSP_MODEL==DSP_SSD1322
#include "dspcore.h"
#include "../core/config.h"

#ifndef DEF_SPI_FREQ
  #define DEF_SPI_FREQ        16000000UL      /*  set it to 0 for system default */
#endif
#ifndef SSD1322_GRAYSCALE
  #define SSD1322_GRAYSCALE   false
#endif

#if DSP_HSPI
  DspCore::DspCore(): Jamis_SSD1322(256, 64, &SPI2, TFT_DC, TFT_RST, TFT_CS, DEF_SPI_FREQ) {}
#else
  DspCore::DspCore(): Jamis_SSD1322(256, 64, &SPI, TFT_DC, TFT_RST, TFT_CS, DEF_SPI_FREQ) {}
#endif

void DspCore::initDisplay() {
#if !SSD1322_GRAYSCALE
  #include "tools/oledcolorfix.h"
#else
    config.theme.background = TFT_BG;
  #if DSP_INVERT_TITLE
    config.theme.meta       = TFT_BG;
    config.theme.metabg     = GRAY_9;
    config.theme.metafill   = GRAY_9;
  #else
    config.theme.meta       = GRAY_9;
    config.theme.metabg     = TFT_BG;
    config.theme.metafill   = TFT_BG;
  #endif  
    config.theme.clock      = TFT_FG;
    config.theme.clockbg    = GRAY_1;
    config.theme.weather    = GRAY_2;
    config.theme.title1     = GRAY_B;
    config.theme.title2     = GRAY_3;
    config.theme.rssi       = GRAY_5;
    config.theme.ip         = GRAY_2;
    config.theme.vol        = TFT_FG;
    config.theme.bitrate    = TFT_FG;
    config.theme.digit      = TFT_FG;
    config.theme.buffer     = TFT_FG;
    config.theme.volbarout  = GRAY_9;
    config.theme.volbarin   = GRAY_9;
    config.theme.plcurrent     = TFT_BG;
    config.theme.plcurrentbg   = GRAY_7;
    config.theme.plcurrentfill = GRAY_7;
    for(byte i=0;i<5;i++) config.theme.playlist[i] = GRAY_1;
#endif  //!SSD1322_GRAYSCALE

  begin();
  cp437(true);
  flip();
  invert();
  setTextWrap(false);
}

void DspCore::clearDsp(bool black){ clearDisplay(); }
void DspCore::flip(){ setRotation(config.store.flipscreen?2:0); }
void DspCore::invert(){ invertDisplay(config.store.invertdisplay); }
void DspCore::sleep(void){ oled_command(SSD1322_DISPLAYOFF); }
void DspCore::wake(void){ oled_command(SSD1322_DISPLAYON); }

#endif
