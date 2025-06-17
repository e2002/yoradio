/*************************************************************************************
    Nokia 5110 84x48 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayN5110conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayN5110conf_h
#define displayN5110conf_h

#define DSP_WIDTH       84
#define TFT_FRAMEWDT    0
#define MAX_WIDTH       DSP_WIDTH
#define SCROLLDELAY     180

#define HIDE_TITLE2
#define HIDE_HEAPBAR
#define HIDE_IP
//#define HIDE_VOLBAR
#define HIDE_RSSI
#define HIDE_VU

#define bootLogoTop     0

#ifndef BATTERY_OFF
  #define BatX      TFT_FRAMEWDT		// X coordinate for batt. (Координата X для батарейки)
  #define BatY      TFT_FRAMEWDT		// Y coordinate for batt. (Координата Y для батарейки)
  #define ProcX     TFT_FRAMEWDT		// X coordinate for percent (Координата X для процентов заряда)
  #define ProcY     TFT_FRAMEWDT		// Y coordinate for percent (Координата Y для процентов заряда)
#endif

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 1, WA_LEFT }, 140, true, MAX_WIDTH, 5000, 5, SCROLLDELAY };
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, 8, 1, WA_LEFT }, 140, true, MAX_WIDTH-24, 5000, 5, SCROLLDELAY };
const ScrollConfig playlistConf   PROGMEM = {{ 2, 22, 1, WA_LEFT }, 140, true, MAX_WIDTH-4, 1000, 5, SCROLLDELAY };
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 1, WA_CENTER }, 140, false, MAX_WIDTH, 0, 5, SCROLLDELAY };
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT, 48-7, 1, WA_LEFT }, 140, false, MAX_WIDTH, 0, 5, SCROLLDELAY };
const ScrollConfig weatherConf    PROGMEM = {{ TFT_FRAMEWDT, 48-11, 1, WA_LEFT }, 140, true, MAX_WIDTH-6*3-2, 1000, 5, SCROLLDELAY };	// Weather (погода)

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig  playlBGConf     PROGMEM = {{ 0, 20, 0, WA_LEFT }, DSP_WIDTH, 11, false };
//const FillConfig   metaBGConf     PROGMEM = {{ MAX_WIDTH-22, 9, 0, WA_LEFT }, 1, 5, false };
const FillConfig   volbarConf     PROGMEM = {{ 0, 45, 0, WA_LEFT }, MAX_WIDTH, 3, true };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 48-7, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { TFT_FRAMEWDT, 8, 1, WA_RIGHT };
const WidgetConfig voltxtConf     PROGMEM = { 0, 48-11, 1, WA_RIGHT };
const WidgetConfig numConf        PROGMEM = { 0, 34, 19, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { 0, 8, 1, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { 0, 16, 1, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { 0, 24, 1, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { 0, 32, 1, WA_CENTER };
const WidgetConfig clockConf      PROGMEM = { 4, 35, 19, WA_RIGHT };  /* 19 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT, 50, 1, WA_LEFT };

const WidgetConfig bootWdtConf    PROGMEM = { 0, 48-7-10, 1, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 90, 10, 3 };

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "%d";
const char          iptxtFmt[]    PROGMEM = "%s";
const char         voltxtFmt[]    PROGMEM = "%d";
const char        bitrateFmt[]    PROGMEM = "%d";

/* MOVES  */                             /* { left, top, width (0 - auto, -1 - lock } */
const MoveConfig    clockMove     PROGMEM = { 0, 0, -1 };
const MoveConfig   weatherMove    PROGMEM = { 0, 0, -1 };
const MoveConfig   weatherMoveVU  PROGMEM = { 0, 0, -1 };

#endif
