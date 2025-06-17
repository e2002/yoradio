/*************************************************************************************
    ST7735 160x128 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayST7735conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/
#ifndef displayST7735conf_h
#define displayST7735conf_h

#define DSP_WIDTH       160
#define TFT_FRAMEWDT    4
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2

#if BITRATE_FULL
  #define TITLE_FIX 24
#else
  #define TITLE_FIX 0
#endif
#define bootLogoTop     68

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 2, WA_LEFT }, 140, true, MAX_WIDTH, 5000, 3, 30 };
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, 26, 1, WA_LEFT }, 140, true, MAX_WIDTH-24, 5000, 3, 30 };
const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT, 36, 1, WA_LEFT }, 140, true, MAX_WIDTH-TITLE_FIX, 5000, 3, 30 };
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 56, 1, WA_LEFT }, 140, true, MAX_WIDTH, 1000, 4, 30 };
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 2, WA_CENTER }, 140, false, MAX_WIDTH, 0, 4, 30 };
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT, 128-TFT_FRAMEWDT-8, 1, WA_LEFT }, 140, false, MAX_WIDTH, 0, 3, 30 };
const ScrollConfig weatherConf    PROGMEM = {{ TFT_FRAMEWDT, 42, 1, WA_LEFT }, 140, true, MAX_WIDTH, 0, 3, 30 };

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 22, false };
const FillConfig   metaBGConfInv  PROGMEM = {{ 0, 22, 0, WA_LEFT }, DSP_WIDTH, 1, false };
const FillConfig   volbarConf     PROGMEM = {{ TFT_FRAMEWDT, 118, 0, WA_LEFT }, MAX_WIDTH, 5, true };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 52, 0, WA_LEFT }, DSP_WIDTH, 22, false };
const FillConfig  heapbarConf     PROGMEM = {{ 0, 127, 0, WA_LEFT }, DSP_WIDTH, 1, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 110, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { TFT_FRAMEWDT, 26, 1, WA_RIGHT };
//const WidgetConfig bitrateConf    PROGMEM = { TFT_FRAMEWDT, 99, 1, WA_LEFT };
const WidgetConfig voltxtConf     PROGMEM = { TFT_FRAMEWDT, 108, 1, WA_LEFT };
const WidgetConfig  iptxtConf     PROGMEM = { TFT_FRAMEWDT, 108, 1, WA_CENTER };
const WidgetConfig   rssiConf     PROGMEM = { TFT_FRAMEWDT, 108, 1, WA_RIGHT };
const WidgetConfig numConf        PROGMEM = { 0, 86, 35, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { 0, 40, 1, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { 0, 54, 1, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { 0, 74, 1, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { 0, 88, 1, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { 0, 94, 35, WA_RIGHT };  /* 35 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT, 54, 1, WA_LEFT };

const WidgetConfig bootWdtConf    PROGMEM = { 0, 90, 1, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 90, 14, 4 };
const BitrateConfig fullbitrateConf PROGMEM = {{DSP_WIDTH-TFT_FRAMEWDT-19, 23, 1, WA_LEFT}, 22 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = { 12, 50, 2, 1, 10, 1 };

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "%d";
const char          iptxtFmt[]    PROGMEM = " %s";
const char         voltxtFmt[]    PROGMEM = "\023\025%d";
const char        bitrateFmt[]    PROGMEM = "%d";
/* MOVES  */                             /* { left, top, width (0 - auto, -1 - lock } */
const MoveConfig    clockMove     PROGMEM = { 14, 94, 0};
const MoveConfig   weatherMove    PROGMEM = {TFT_FRAMEWDT, 48, MAX_WIDTH};
const MoveConfig   weatherMoveVU  PROGMEM = { 34, 48, 122 };

#endif
