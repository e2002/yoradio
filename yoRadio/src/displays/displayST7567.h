#ifndef displayST7567_h
#define displayST7567_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "../ST7567/ST7567.h"
#include "fonts/bootlogo21x32.h"

typedef GFXcanvas1 Canvas;
typedef ST7567 yoDisplay;

#include "tools/commongfx.h"

#if __has_include("conf/displayST7567conf_custom.h")
  #include "conf/displayST7567conf_custom.h"
#else
  #include "conf/displayST7567conf.h"
#endif

/*
 * OLED COLORS
 */
#define BOOT_PRG_COLOR    BLACK
#define BOOT_TXT_COLOR    BLACK
#define PINK              BLACK
#define SILVER            BLACK
#define TFT_BG            WHITE
#define TFT_FG            BLACK
#define TFT_LOGO          BLACK

#endif
