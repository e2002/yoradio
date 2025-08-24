#ifndef displaySSD1322_h
#define displaySSD1322_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "../SSD1322/SSD1322.h"
#include "fonts/bootlogo21x32.h"
#include "fonts/dsfont35.h"

typedef GFXcanvas1 Canvas;
typedef Jamis_SSD1322 yoDisplay;

#include "tools/commongfx.h"

#if __has_include("conf/displaySSD1322conf_custom.h")
  #include "conf/displaySSD1322conf_custom.h"
#else
  #include "conf/displaySSD1322conf.h"
#endif

/*
 * OLED COLORS
 */

#define SILVER            WHITE
#define TFT_BG            BLACK
#define TFT_FG            WHITE
#define TFT_LOGO          WHITE

#endif
