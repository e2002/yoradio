/*************************************************************************************
    LCD2004 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayLCD2004conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayLCD2004conf_h
#define displayLCD2004conf_h

#define DSP_WIDTH       20
#define DSP_HEIGHT      4
#define TFT_FRAMEWDT    0
#define MAX_WIDTH       20
#define PLMITEMS        4

#define HIDE_IP
#define HIDE_VOLBAR
#define HIDE_HEAPBAR
#define HIDE_RSSI
#define HIDE_VU
#define META_MOVE

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
#define SDELTA      2
#define STIME       300
const ScrollConfig      metaConf   PROGMEM = {{ 0, 0, 1, WA_LEFT }, 140, true, MAX_WIDTH-6, 2000, SDELTA, STIME };
const ScrollConfig    title1Conf   PROGMEM = {{ 0, 1, 1, WA_LEFT }, 140, true, MAX_WIDTH-4, 2000, SDELTA, STIME };
const ScrollConfig    title2Conf   PROGMEM = {{ 0, 2, 1, WA_LEFT }, 140, true, MAX_WIDTH,   2000, SDELTA, STIME };
const ScrollConfig  playlistConf   PROGMEM = {{ 1, 1, 1, WA_LEFT }, 140, true, MAX_WIDTH-1, 2000, SDELTA, STIME };
const ScrollConfig   weatherConf   PROGMEM = {{ 0, 3, 1, WA_LEFT }, 140, false, MAX_WIDTH-4, 2000, SDELTA, STIME };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig     voltxtConf  PROGMEM = { 0, 3, 1, WA_RIGHT };
const char            voltxtFmt[]  PROGMEM = "%d";

const WidgetConfig    bootstrConf  PROGMEM = { 0, 1, 1, WA_CENTER };
const WidgetConfig    bitrateConf  PROGMEM = { 0, 1, 1, WA_RIGHT };
const WidgetConfig        numConf  PROGMEM = { 0, 2, 1, WA_CENTER };
const WidgetConfig      clockConf  PROGMEM = { 0, 0, 1, WA_RIGHT };
const WidgetConfig    bootWdtConf  PROGMEM = { 0, 2, 1, WA_CENTER };
const ProgressConfig  bootPrgConf  PROGMEM = { 250, 10, 4 };

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char        bitrateFmt[]    PROGMEM = "%d";
//#define WEATHER_FMT_SHORT
//const char        weatherFmt[]    PROGMEM = "%.1fC %dmm %s%%";

/* MOVES  */                             /* { left, top, width } */
const MoveConfig   metaMove       PROGMEM = { 0, 0, MAX_WIDTH };
#endif
