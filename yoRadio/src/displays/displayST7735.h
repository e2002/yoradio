#ifndef displayST7735_h
#define displayST7735_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "fonts/bootlogo62x40.h"
#include "fonts/dsfont35.h"

typedef GFXcanvas16 Canvas;
typedef Adafruit_ST7735 yoDisplay;

#include "tools/commongfx.h"

#if __has_include("conf/displayST7735conf_custom.h")
  #include "conf/displayST7735conf_custom.h"
#else
  #if DTYPE==INITR_MINI160x80
    #include "conf/displayST7735_miniconf.h"
  #elif DTYPE==INITR_144GREENTAB
    #include "conf/displayST7735_144conf.h"
  #else
    #include "conf/displayST7735_blackconf.h"
  #endif
#endif



#endif
