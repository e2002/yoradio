#ifndef displayGC9106_h
#define displayGC9106_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "../Adafruit_GC9106Ex/Adafruit_GC9106Ex.h"       // https://github.com/prenticedavid/Adafruit_GC9102_kbv

#if CLOCKFONT_MONO
  #include "fonts/DS_DIGI28pt7b_mono.h"                          // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#else
  #include "fonts/DS_DIGI28pt7b.h"
#endif
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

typedef GFXcanvas16 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displayGC9106conf_custom.h")
  #include "conf/displayGC9106conf_custom.h"
#else
  #include "conf/displayGC9106conf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

class DspCore: public Adafruit_GC9106Ex {
#include "tools/commongfx.h"
};

extern DspCore dsp;

#endif
