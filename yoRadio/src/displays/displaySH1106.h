#ifndef displaySH1106_h
#define displaySH1106_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

#define DSP_OLED

typedef GFXcanvas1 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displaySH1106conf_custom.h")
  #include "conf/displaySH1106conf_custom.h"
#else
  #include "conf/displaySH1106conf.h"
#endif

#if DSP_MODEL==DSP_SH1106
class DspCore: public Adafruit_SH1106G {
#else
class DspCore: public Adafruit_SH1107 {
#endif
#include "tools/commongfx.h"
};

extern DspCore dsp;

/*
 * OLED COLORS
 */
#define BOOT_PRG_COLOR    SH110X_WHITE
#define BOOT_TXT_COLOR    SH110X_WHITE
#define PINK              SH110X_WHITE
#define SILVER            SH110X_WHITE
#define TFT_BG            SH110X_BLACK
#define TFT_FG            SH110X_WHITE
#define TFT_LOGO          SH110X_WHITE

#endif
