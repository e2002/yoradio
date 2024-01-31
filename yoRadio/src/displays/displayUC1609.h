#ifndef displayUC1609_h
#define displayUC1609_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "../UC1609/UC1609.h"
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

#define DSP_OLED

typedef GFXcanvas1 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displayUC1609conf_custom.h")
  #include "conf/displayUC1609conf_custom.h"
#else
  #include "conf/displayUC1609conf.h"
#endif

class DspCore: public UC1609 {
#include "tools/commongfx.h"
};

extern DspCore dsp;

/*
 * OLED COLORS
 */
#define BOOT_PRG_COLOR    BLACK
#define BOOT_TXT_COLOR    BLACK
#define SILVER            BLACK
#define TFT_BG            WHITE
#define TFT_FG            BLACK
#define TFT_LOGO          BLACK

#endif
