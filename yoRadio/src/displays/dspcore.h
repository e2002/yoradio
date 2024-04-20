#ifndef dspcore_h
#define dspcore_h
#include "../core/options.h"

/* definition to expand macro then apply to pragma message */
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var)

//#pragma message(VAR_NAME_VALUE(DSP_MODEL))

#if DSP_MODEL==DSP_DUMMY
  #define DUMMYDISPLAY
  #define DSP_NOT_FLIPPED
  #include "tools/l10n.h"
#elif DSP_MODEL==DSP_ST7735
  #include "displayST7735.h"
#elif DSP_MODEL==DSP_SSD1306 || DSP_MODEL==DSP_SSD1306x32
  #include "displaySSD1306.h"
#elif DSP_MODEL==DSP_NOKIA5110
  #include "displayN5110.h"
#elif DSP_MODEL==DSP_ST7789 || DSP_MODEL==DSP_ST7789_240
  #include "displayST7789.h"
#elif DSP_MODEL==DSP_SH1106
  #include "displaySH1106.h"
#elif DSP_MODEL==DSP_1602I2C || DSP_MODEL==DSP_2004I2C
  #include "displayLC1602.h"
#elif DSP_MODEL==DSP_SSD1327
  #include "displaySSD1327.h"
#elif DSP_MODEL==DSP_ILI9341
  #include "displayILI9341.h"
#elif DSP_MODEL==DSP_SSD1305 || DSP_MODEL==DSP_SSD1305I2C
  #include "displaySSD1305.h"
#elif DSP_MODEL==DSP_SH1107
  #include "displaySH1106.h"
#elif DSP_MODEL==DSP_1602 || DSP_MODEL==DSP_2004
  #include "displayLC1602.h"
#elif DSP_MODEL==DSP_GC9106
  #include "displayGC9106.h"
#elif DSP_MODEL==DSP_CUSTOM
  #include "displayCustom.h"
#elif DSP_MODEL==DSP_ILI9225
  #include "displayILI9225.h"
#elif DSP_MODEL==DSP_ST7796
  #include "displayST7796.h"
#elif DSP_MODEL==DSP_GC9A01A
  #include "displayGC9A01A.h"
#elif DSP_MODEL==DSP_ILI9488 || DSP_MODEL==DSP_ILI9486
  #include "displayILI9488.h"
#elif DSP_MODEL==DSP_SSD1322
  #include "displaySSD1322.h"
#elif DSP_MODEL==DSP_ST7920
  #include "displayST7920.h"
#endif

//extern DspCore dsp;

#endif
