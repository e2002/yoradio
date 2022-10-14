#ifndef displayILI9225_h
#define displayILI9225_h
#include "../core/options.h"
//==================================================
#include "Arduino.h"
#include "../ILI9225Fix/TFT_22_ILI9225Fix.h"
#include "fonts/DS_DIGI28pt7b.h"          // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

typedef GFXcanvas16 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displayILI9225conf_custom.h")
  #include "conf/displayILI9225conf_custom.h"
#else
  #include "conf/displayILI9225conf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

class DspCore: public TFT_22_ILI9225 {
#include "tools/commongfx.h"
};

extern DspCore dsp;

#endif
