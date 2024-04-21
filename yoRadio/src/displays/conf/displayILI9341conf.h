/*************************************************************************************
    ILI9341 320x240 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayILI9341conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayILI9341conf_h
#define displayILI9341conf_h

#define DSP_WIDTH       320
#define TFT_FRAMEWDT    8
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2

#if BITRATE_FULL
  #define TITLE_FIX 44
#else
  #define TITLE_FIX 0
#endif
#define bootLogoTop     68

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 3, WA_LEFT }, 140, true, MAX_WIDTH, 5000, 5, 30 };
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, 50, 2, WA_LEFT }, 140, true, MAX_WIDTH-TITLE_FIX, 5000, 4, 30 };
const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT, 70, 2, WA_LEFT }, 140, true, MAX_WIDTH-TITLE_FIX, 5000, 4, 30 };
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 112, 2, WA_LEFT }, 140, true, MAX_WIDTH, 1000, 4, 30 };
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 3, WA_CENTER }, 140, false, MAX_WIDTH, 0, 4, 20 };
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT, 240-TFT_FRAMEWDT-16, 2, WA_LEFT }, 140, false, MAX_WIDTH, 0, 4, 30 };
const ScrollConfig weatherConf    PROGMEM = {{ 8, 87, 2, WA_LEFT }, 140, true, MAX_WIDTH, 0, 4, 30 };

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 38, false };
const FillConfig   metaBGConfInv  PROGMEM = {{ 0, 38, 0, WA_LEFT }, DSP_WIDTH, 1, false };
const FillConfig   volbarConf     PROGMEM = {{ TFT_FRAMEWDT, 240-TFT_FRAMEWDT-6, 0, WA_LEFT }, MAX_WIDTH, 6, true };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 107, 0, WA_LEFT }, DSP_WIDTH, 24, false };
const FillConfig  heapbarConf     PROGMEM = {{ 0, 239, 0, WA_LEFT }, DSP_WIDTH, 1, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 182, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { 70, 191, 1, WA_LEFT };
const WidgetConfig voltxtConf     PROGMEM = { 0, 214, 1, WA_CENTER };
const WidgetConfig  iptxtConf     PROGMEM = { TFT_FRAMEWDT, 214, 1, WA_LEFT };
const WidgetConfig   rssiConf     PROGMEM = { TFT_FRAMEWDT, 214-6, 2, WA_RIGHT };
const WidgetConfig numConf        PROGMEM = { 0, 120+30, 52, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { TFT_FRAMEWDT, 66, 2, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { TFT_FRAMEWDT, 90, 2, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { TFT_FRAMEWDT, 130, 2, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { TFT_FRAMEWDT, 154, 2, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { 12, 176, 52, WA_RIGHT };  /* 52 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT, 100, 1, WA_LEFT };

const WidgetConfig bootWdtConf    PROGMEM = { 0, 162, 1, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 90, 14, 4 };
const BitrateConfig fullbitrateConf PROGMEM = {{DSP_WIDTH-TFT_FRAMEWDT-38, 43, 2, WA_LEFT}, 42 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = { 24, 100, 4, 2, 10, 2 };

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "WiFi %d";
const char          iptxtFmt[]    PROGMEM = "IP %s";
const char         voltxtFmt[]    PROGMEM = "Vol %d";
const char        bitrateFmt[]    PROGMEM = "%d kBs";

/* MOVES  */                             /* { left, top, width } */
const MoveConfig    clockMove     PROGMEM = { 0, 176, -1 };
const MoveConfig   weatherMove    PROGMEM = { 8, 97, MAX_WIDTH };
const MoveConfig   weatherMoveVU  PROGMEM = { 70, 97, 250 };

#endif
