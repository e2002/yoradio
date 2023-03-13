#ifndef displayILI9488_h
#define displayILI9488_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "../ILI9488/ILI9486_SPI.h"

#if CLOCKFONT_MONO
  #include "fonts/DS_DIGI56pt7b_mono.h"        // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#else
  #include "fonts/DS_DIGI56pt7b.h"
#endif
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

typedef GFXcanvas16 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displayST7796conf_custom.h")
  #include "conf/displayILI9488conf_custom.h"
#else
  #include "conf/displayILI9488conf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

#define ILI9488_SLPIN     0x10
#define ILI9488_SLPOUT    0x11
#define ILI9488_DISPOFF   0x28
#define ILI9488_DISPON    0x29

class DspCore: public ILI9486_SPI {
#include "tools/commongfx.h"    
};

extern DspCore dsp;

#endif
