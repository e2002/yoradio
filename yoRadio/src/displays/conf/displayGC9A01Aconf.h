/*************************************************************************************
    ST7789 240x240 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayST7789conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayGC9A01Aconf_h
#define displayGC9A01Aconf_h

#define DSP_WIDTH       240
#define TFT_FRAMEWDT    8
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2
#define RSSI_DIGIT			true
#define bootLogoTop     68
#define HIDE_TITLE2
#define BOOMBOX_STYLE

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT+12, TFT_FRAMEWDT+28+20, 3, WA_CENTER }, 140, true, MAX_WIDTH-24, 5000, 5, 30 };
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, /*70*/90, 2, WA_CENTER }, 140, true, MAX_WIDTH, 5000, 4, 30 };
const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT, 90, 2, WA_CENTER }, 140, true, MAX_WIDTH, 5000, 4, 30 };
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 112, 2, WA_LEFT }, 140, true, MAX_WIDTH, 0, 2, 30 };
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT+12, TFT_FRAMEWDT+28+20, 3, WA_CENTER }, 140, false, MAX_WIDTH-24, 0, 4, 20 };
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT+32, 240-TFT_FRAMEWDT-34, 2, WA_LEFT }, 140, false, MAX_WIDTH-64, 0, 4, 30 };
const ScrollConfig weatherConf    PROGMEM = {{ TFT_FRAMEWDT+30, 37, 1, WA_LEFT }, 140, true, MAX_WIDTH-60, 0, 3, 30 };
/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 32+20, 0, WA_LEFT }, DSP_WIDTH, 30, false };
const FillConfig   metaBGConfInv  PROGMEM = {{ 0, 32+20+30, 0, WA_LEFT }, DSP_WIDTH, 1, false };
const FillConfig   volbarConf     PROGMEM = {{ TFT_FRAMEWDT+56, 240-TFT_FRAMEWDT-6, 0, WA_LEFT }, MAX_WIDTH-112, 6+TFT_FRAMEWDT+1, true };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 107, 0, WA_LEFT }, DSP_WIDTH, 24, false };
const FillConfig  heapbarConf     PROGMEM = {{ 0, 83, 0, WA_LEFT }, DSP_WIDTH, 1, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 182, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { 134, 23, 1, WA_RIGHT };
const WidgetConfig voltxtConf     PROGMEM = { 80, 12, 1, WA_CENTER };
const WidgetConfig  iptxtConf     PROGMEM = { TFT_FRAMEWDT, 214, 1, WA_CENTER };
const WidgetConfig   rssiConf     PROGMEM = { 134, 23, 1, WA_LEFT };
const WidgetConfig numConf        PROGMEM = { 0, 120+30+20, 52, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { TFT_FRAMEWDT, 96, 2, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { TFT_FRAMEWDT, 118, 2, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { TFT_FRAMEWDT, 146, 2, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { TFT_FRAMEWDT, 168, 2, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { 0, 176, 52, WA_CENTER };  /* 52 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT+20, 188, 1, WA_CENTER };

const WidgetConfig bootWdtConf    PROGMEM = { 0, 162, 1, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 90, 14, 4 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = { 90, 20, 6, 2, 10, 5 };

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "WIFI %d";
const char          iptxtFmt[]    PROGMEM = "%s";
const char         voltxtFmt[]    PROGMEM = "\023\025%d";
const char        bitrateFmt[]    PROGMEM = "%d KBS";

/* MOVES  */                             /* { left, top, width } */
const MoveConfig    clockMove     PROGMEM = { 0, 164, 0 };
const MoveConfig   weatherMove    PROGMEM = { TFT_FRAMEWDT, 202, -1 };
const MoveConfig   weatherMoveVU  PROGMEM = { TFT_FRAMEWDT, 202, -1/*MAX_WIDTH*/ };

#endif
