#ifndef displayGC9106_h
#define displayGC9106_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "../Adafruit_GC9106Ex/Adafruit_GC9106Ex.h"       // https://github.com/prenticedavid/Adafruit_GC9102_kbv
#include "fonts/bootlogo62x40.h"
#include "fonts/dsfont35.h"

typedef GFXcanvas16 Canvas;
typedef Adafruit_GC9106Ex yoDisplay;

#include "tools/commongfx.h"

#if __has_include("conf/displayGC9106conf_custom.h")
  #include "conf/displayGC9106conf_custom.h"
#else
  #include "conf/displayGC9106conf.h"
#endif

#endif
