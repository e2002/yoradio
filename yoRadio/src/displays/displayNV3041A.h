#ifndef displayNV3041A_h
#define displayNV3041A_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "Arduino_GFX_Library.h"

#if CLOCKFONT_MONO
  #include "fonts/DS_DIGI56pt7b_mono.h"        // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#else
  #include "fonts/DS_DIGI56pt7b.h"
#endif
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

typedef GFXcanvas16 Canvas;
#define drawRGBBitmap draw16bitRGBBitmap
#define drawBitmap draw16bitRGBBitmap

#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displayNV3041Aconf_custom.h")
  #include "conf/displayNV3041Aconf_custom.h"
#else
  #include "conf/displayNV3041Aconf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

#define NV3041A_SLPIN     0x10
#define NV3041A_SLPOUT    0x11
#define NV3041A_DISPOFF   0x28
#define NV3041A_DISPON    0x29

class DspCore: public Arduino_NV3041A {
#include "tools/commongfx.h"    
};

extern DspCore dsp;

#endif
