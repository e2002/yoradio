/*************************************************************************************
    SSD1306 128x32 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displaySSD1306conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displaySSD1306conf_h
#define displaySSD1306conf_h

#define DSP_WIDTH       128
#define DSP_HEIGHT      32
#define TFT_FRAMEWDT    1
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2

#define HIDE_IP
#define HIDE_TITLE2
#define HIDE_HEAPBAR
#define HIDE_VU

#define bootLogoTop     68

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 1, WA_LEFT }, 140, true, MAX_WIDTH-6*5-2, 5000, 2, 25 };
const ScrollConfig title1Conf     PROGMEM = {{ 0, 11, 1, WA_LEFT }, 140, true, DSP_WIDTH-6*4, 5000, 2, 25 };
//const ScrollConfig title2Conf     PROGMEM = {{ 0, 26, 1, WA_LEFT }, 140, true, DSP_WIDTH, 5000, 2, 35 };
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 14, 1, WA_LEFT }, 140, true, MAX_WIDTH, 500, 2, 25 };
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT, 1, 1, WA_CENTER }, 140, false, MAX_WIDTH, 0, 2, 25 };
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT, 32-7, 1, WA_LEFT }, 140, false, MAX_WIDTH, 0, 2, 25 };
const ScrollConfig weatherConf    PROGMEM = {{ 0, 20, 1, WA_LEFT }, 140, true, DSP_WIDTH-6*4, 0, 2, 25 }; // ПОГОДА!!

/* BACKGROUNGC9106DS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH/*-6*5-3*/, 9, false };
const FillConfig   volbarConf     PROGMEM = {{ 0, 32-1-1-1, 0, WA_LEFT }, DSP_WIDTH, 3, true };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 13, 0, WA_LEFT }, DSP_WIDTH, 9, false };
//const FillConfig  heapbarConf     PROGMEM = {{ 0, 63, 0, WA_LEFT }, DSP_WIDTH, 1, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 32-8, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { 0, 11, 1, WA_RIGHT };
const WidgetConfig voltxtConf     PROGMEM = { 0, 20, 1, WA_RIGHT };
const WidgetConfig  iptxtConf     PROGMEM = { 0, 64-11, 1, WA_LEFT };
const WidgetConfig   rssiConf     PROGMEM = { 0, 64-11, 1, WA_RIGHT };
const WidgetConfig numConf        PROGMEM = { 0, 12, 2, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { 0, 9, 1, WA_LEFT };
const WidgetConfig apName2Conf    PROGMEM = { 0, 9, 1, WA_RIGHT };
const WidgetConfig apPassConf     PROGMEM = { 0, 17, 1, WA_LEFT };
const WidgetConfig apPass2Conf    PROGMEM = { 0, 17, 1, WA_RIGHT };
const WidgetConfig  clockConf     PROGMEM = { 0,  1, 1, WA_RIGHT };
//const WidgetConfig vuConf         PROGMEM = { 1, 28, 1, WA_LEFT };

const WidgetConfig bootWdtConf    PROGMEM = { 0, 32-8*2-5, 1, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 90, 10, 4 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = { 12, 48, 2, 1, 8, 3 };

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "%d";
const char          iptxtFmt[]    PROGMEM = "%s";
const char         voltxtFmt[]    PROGMEM = "%d";
const char        bitrateFmt[]    PROGMEM = "%d";

/* MOVES  */                             /* { left, top, width } */
const MoveConfig    clockMove     PROGMEM = { 0, 0, -1 };
const MoveConfig   weatherMove    PROGMEM = { 0, 0, -1 };
const MoveConfig   weatherMoveVU  PROGMEM = { 0, 0, -1 };

#endif
