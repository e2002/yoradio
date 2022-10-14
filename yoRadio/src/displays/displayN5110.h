#ifndef displayN5110_h
#define displayN5110_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "fonts/TinyFont5.h"
#include "fonts/TinyFont6.h"
#include "fonts/DS_DIGI15pt7b.h"
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

#define DSP_OLED

typedef GFXcanvas1 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displayN5110conf_custom.h")
  #include "conf/displayN5110conf_custom.h"
#else
  #include "conf/displayN5110conf.h"
#endif

class DspCore: public Adafruit_PCD8544 {
#include "tools/commongfx.h"
};

extern DspCore dsp;

/*
 * TFT COLORS
 */
#define BOOT_PRG_COLOR    BLACK
#define BOOT_TXT_COLOR    BLACK
#define SILVER            BLACK
#define TFT_BG            WHITE
#define TFT_FG            BLACK
#define TFT_LOGO          BLACK

#endif
