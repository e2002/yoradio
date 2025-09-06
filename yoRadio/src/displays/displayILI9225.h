#ifndef displayILI9225_h
#define displayILI9225_h
#include "../core/options.h"
//==================================================
#include "Arduino.h"
#include "../Adafruit_ILI9225/Adafruit_ILI9225.h"
#include "fonts/bootlogo99x64.h"
#include "fonts/dsfont35.h"

typedef GFXcanvas16 Canvas;
typedef Adafruit_ILI9225 yoDisplay;

#include "tools/commongfx.h"

#if __has_include("conf/displayILI9225conf_custom.h")
  #include "conf/displayILI9225conf_custom.h"
#else
  #include "conf/displayILI9225conf.h"
#endif

#endif
