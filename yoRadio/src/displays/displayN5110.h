#ifndef displayN5110_h
#define displayN5110_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "fonts/TinyFont5.h"
#include "fonts/TinyFont6.h"
#include "fonts/bootlogo21x28.h"
#include "fonts/dsfont19.h"

typedef GFXcanvas1 Canvas;
typedef Adafruit_PCD8544 yoDisplay;

#include "tools/commongfx.h"

#if __has_include("conf/displayN5110conf_custom.h")
  #include "conf/displayN5110conf_custom.h"
#else
  #include "conf/displayN5110conf.h"
#endif

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
