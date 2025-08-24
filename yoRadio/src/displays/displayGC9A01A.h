#ifndef displayGC9A01A_h
#define displayGC9A01A_h
#include "../core/options.h"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "../Adafruit_GC9A01A/Adafruit_GC9A01A.h"
#include "fonts/bootlogo99x64.h"
#include "fonts/dsfont52.h"

typedef GFXcanvas16 Canvas;
typedef Adafruit_GC9A01A yoDisplay;

#include "tools/commongfx.h"

#if __has_include("conf/displayGC9A01Aconf_custom.h")
  #include "conf/displayGC9A01Aconf_custom.h"
#else
  #include "conf/displayGC9A01Aconf.h"
#endif

#endif
