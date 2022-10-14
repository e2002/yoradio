#ifndef displaySSD1327_h
#define displaySSD1327_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1327.h>
#include "fonts/DS_DIGI28pt7b.h"          // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

#define DSP_OLED

typedef GFXcanvas1 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displaySSD1327conf_custom.h")
  #include "conf/displaySSD1327conf_custom.h"
#else
  #include "conf/displaySSD1327conf.h"
#endif

class DspCore: public Adafruit_SSD1327 {
#include "tools/commongfx.h"
};

extern DspCore dsp;

      /*
      SSD1327_GRAYTABLE,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
      0x07, 0x08, 0x10, 0x18, 0x20, 0x2f, 0x38, 0x3f,
      */

/*
 * TFT COLORS
 */
#define BOOT_PRG_COLOR    0x07
#define BOOT_TXT_COLOR    0x3f
#define DARK_GRAY   0x01
#define SILVER      0x07
#define TFT_BG      0x00
#define TFT_FG      0x08
#define TFT_LOGO    0x3f
#define ORANGE      0x05
#define PINK        0x02

#endif
