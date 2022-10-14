#ifndef displaySSD1305_h
#define displaySSD1305_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

#define DSP_OLED

typedef GFXcanvas1 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displaySSD1305conf_custom.h")
  #include "conf/displaySSD1305conf_custom.h"
#else
  #include "conf/displaySSD1305conf.h"
#endif

class DspCore: public Adafruit_SSD1305 {
#include "tools/commongfx.h"
};

extern DspCore dsp;

/*
 * OLED COLORS
 */
#define BOOT_PRG_COLOR    WHITE
#define BOOT_TXT_COLOR    WHITE
#define PINK              WHITE
#define SILVER            WHITE
#define TFT_BG            BLACK
#define TFT_FG            WHITE
#define TFT_LOGO          WHITE

#endif
