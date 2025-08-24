#ifndef displayST7920_h
#define displayST7920_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "../ST7920/ST7920.h"
#include "fonts/bootlogo21x32.h"

typedef GFXcanvas1 Canvas;
typedef ST7920 yoDisplay;

#include "tools/commongfx.h"

#if __has_include("conf/displayST7920conf_custom.h")
  #include "conf/displayST7920conf_custom.h"
#else
  #include "conf/displayST7920conf.h"
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
