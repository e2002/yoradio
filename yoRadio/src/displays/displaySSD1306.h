#ifndef displaySSD1306_h
#define displaySSD1306_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#if DSP_MODEL==DSP_SSD1306
  #include "fonts/bootlogo21x32.h"
#endif

typedef GFXcanvas1 Canvas;
typedef Adafruit_SSD1306 yoDisplay;

#include "tools/commongfx.h"

#if __has_include("conf/displaySSD1306conf_custom.h")
  #include "conf/displaySSD1306conf_custom.h"
#else
  #if DSP_MODEL==DSP_SSD1306
    #include "conf/displaySSD1306conf.h"
  #else
    #include "conf/displaySSD1306x32conf.h"
  #endif
#endif

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
