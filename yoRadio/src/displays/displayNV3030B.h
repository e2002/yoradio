#ifndef displayNV3030B_h
#define displayNV3030B_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "../Adafruit_NV3030B/Adafruit_NV3030B.h"

#if CLOCKFONT_MONO
  #include "fonts/DS_DIGI42pt7b_mono.h"        // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#else
  #include "fonts/DS_DIGI42pt7b.h"
#endif
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

typedef GFXcanvas16 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displayNV3030Bconf_custom.h")
  #include "conf/displayNV3030Bconf_custom.h"
#else
  #include "conf/displayNV3030Bconf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

class DspCore: public Adafruit_NV3030B {
#include "tools/commongfx.h"
};

extern DspCore dsp;

#endif
