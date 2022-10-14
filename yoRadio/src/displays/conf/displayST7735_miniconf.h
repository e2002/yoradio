/*************************************************************************************
    ST7735 160x80 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayST7735conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayST7789conf_h
#define displayST7789conf_h

#define DSP_WIDTH       160
#define TFT_FRAMEWDT    1
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2
#define PLMITEMS        7
#define PLMITEMLENGHT   40
#define PLMITEMHEIGHT   19

#define HIDE_IP
#define HIDE_TITLE2
#define HIDE_VOL

#define bootLogoTop     68

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 2, WA_LEFT }, 140, true, MAX_WIDTH, 5000, 4, 30 };
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, 19, 1, WA_LEFT }, 140, true, MAX_WIDTH-6*3-4, 5000, 3, 30 };
//const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT, 36, 1, WA_LEFT }, 140, true, MAX_WIDTH, 5000, 2, 30 };
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 33, 2, WA_LEFT }, 140, true, MAX_WIDTH, 0, 3, 30 };
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 2, WA_CENTER }, 140, false, MAX_WIDTH, 0, 3, 30 };
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT, 80-TFT_FRAMEWDT-8, 1, WA_LEFT }, 140, false, MAX_WIDTH, 0, 3, 30 };
const ScrollConfig weatherConf    PROGMEM = {{ TFT_FRAMEWDT, 80-13, 1, WA_LEFT }, 140, true, MAX_WIDTH-6*3-4, 0, 3, 30 }; // ПОГОДА!!

/* BACKGROUNGC9106DS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 16, false };
const FillConfig   volbarConf     PROGMEM = {{ TFT_FRAMEWDT, 80-1-1-2, 0, WA_LEFT }, MAX_WIDTH, 2, false };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 30, 0, WA_LEFT }, DSP_WIDTH, 20, false };
const FillConfig  heapbarConf     PROGMEM = {{ 0, 79, 0, WA_LEFT }, DSP_WIDTH, 1, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 65, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { TFT_FRAMEWDT, 19, 1, WA_RIGHT };
//const WidgetConfig voltxtConf     PROGMEM = { 32, 108, 1, WA_RIGHT };
//const WidgetConfig  iptxtConf     PROGMEM = { TFT_FRAMEWDT, 108, 1, WA_LEFT };
const WidgetConfig   rssiConf     PROGMEM = { TFT_FRAMEWDT, 80-13, 1, WA_RIGHT };
const WidgetConfig numConf        PROGMEM = { 0, 29+32, 35, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { 0, 20, 1, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { 0, 32, 1, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { 0, 46, 1, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { 0, 58, 1, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { 0, 29+34, 35, WA_RIGHT };  /* 35 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { 1, 28, 1, WA_LEFT };

const WidgetConfig bootWdtConf    PROGMEM = { 0, 50, 1, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 90, 14, 4 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = { 12, 48, 2, 1, 8, 3 };

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "%d";
//const char          iptxtFmt[]    PROGMEM = "%s";
//const char         voltxtFmt[]    PROGMEM = "%d";
const char        bitrateFmt[]    PROGMEM = "%d";

/* MOVES  */                             /* { left, top, width } */
const MoveConfig    clockMove     PROGMEM = { 14, 29+34, 0};
const MoveConfig   weatherMove    PROGMEM = { TFT_FRAMEWDT, 80-13, MAX_WIDTH-6*3-4-30 };
const MoveConfig   weatherMoveVU  PROGMEM = { 30, 80-13, MAX_WIDTH-6*3-4-30 };

#endif
