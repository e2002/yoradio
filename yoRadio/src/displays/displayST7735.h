#ifndef displayST7735_h
#define displayST7735_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "fonts/DS_DIGI28pt7b.h"          // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

typedef GFXcanvas16 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"


#if __has_include("conf/displayST7735conf_custom.h")
  #include "conf/displayST7735conf_custom.h"
#else
  #if DTYPE==INITR_MINI160x80
    #include "conf/displayST7735_miniconf.h"
  #elif DTYPE==INITR_144GREENTAB
    #include "conf/displayST7735_144conf.h"
  #else
    #include "conf/displayST7735_blackconf.h"
  #endif
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

class DspCore: public Adafruit_ST7735 {
#include "tools/commongfx.h"
};

extern DspCore dsp;

#endif
