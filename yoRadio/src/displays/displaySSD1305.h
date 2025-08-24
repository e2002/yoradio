#ifndef displaySSD1305_h
#define displaySSD1305_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>
#include "fonts/bootlogo21x32.h"

typedef GFXcanvas1 Canvas;
typedef Adafruit_SSD1305 yoDisplay;

#include "tools/commongfx.h"

#if __has_include("conf/displaySSD1305conf_custom.h")
  #include "conf/displaySSD1305conf_custom.h"
#else
  #include "conf/displaySSD1305conf.h"
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
