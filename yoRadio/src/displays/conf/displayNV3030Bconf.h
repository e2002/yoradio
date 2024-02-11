/*************************************************************************************
    NV3030B 280x240 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayNV3030conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayNV3030Bconf_h
#define displayNV3030Bconf_h

#define DSP_WIDTH       280
#define TFT_FRAMEWDT    4
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2
#define HIDE_VOL

#if BITRATE_FULL
  #define TITLE_FIX 45
#else
  #define TITLE_FIX 0
#endif
#define bootLogoTop     68

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ 20+17+TFT_FRAMEWDT, 6+TFT_FRAMEWDT, 3, WA_LEFT }, 140, true, MAX_WIDTH-20, 5000, 2, 45 };
const ScrollConfig title1Conf     PROGMEM = {{ 20+TFT_FRAMEWDT, 46, 2, WA_LEFT }, 140, true, MAX_WIDTH-TITLE_FIX, 5000, 2, 45 };
const ScrollConfig title2Conf     PROGMEM = {{ 20+TFT_FRAMEWDT, 70, 2, WA_LEFT }, 140, true, MAX_WIDTH-TITLE_FIX, 5000, 2, 45 };
const ScrollConfig playlistConf   PROGMEM = {{ 20+TFT_FRAMEWDT, 112, 2, WA_LEFT }, 140, true, MAX_WIDTH, 1000, 4, 30 };
const ScrollConfig apTitleConf    PROGMEM = {{ 20+TFT_FRAMEWDT, TFT_FRAMEWDT, 3, WA_CENTER }, 140, false, MAX_WIDTH, 0, 4, 30 };
const ScrollConfig apSettConf     PROGMEM = {{ 20+TFT_FRAMEWDT, 240-TFT_FRAMEWDT-16, 2, WA_LEFT }, 140, false, MAX_WIDTH, 0, 4, 30 };
const ScrollConfig weatherConf    PROGMEM = {{ 20+TFT_FRAMEWDT, 87, 2, WA_LEFT }, 140, true, MAX_WIDTH, 0, 2, 45 };

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 20, 0, 0, WA_LEFT }, DSP_WIDTH, 38, false };
const FillConfig   metaBGConfInv  PROGMEM = {{ 20, 38, 0, WA_LEFT }, DSP_WIDTH, 1, false };
const FillConfig   volbarConf     PROGMEM = {{ 20+20+TFT_FRAMEWDT, 240-TFT_FRAMEWDT-10, 0, WA_LEFT }, MAX_WIDTH-40, 6, true };
const FillConfig  playlBGConf     PROGMEM = {{ 20, 107, 0, WA_LEFT }, DSP_WIDTH, 24, false };
const FillConfig  heapbarConf     PROGMEM = {{ 20+30, 238, 0, WA_LEFT }, DSP_WIDTH-60, 1, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 20, 182, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { 18+TFT_FRAMEWDT, 197, 1, WA_RIGHT };
const WidgetConfig voltxtConf     PROGMEM = { 20, 214, 1, WA_CENTER };
const WidgetConfig  iptxtConf     PROGMEM = { 20+8+TFT_FRAMEWDT, 208, 2, WA_LEFT };
const WidgetConfig   rssiConf     PROGMEM = { 20+8+TFT_FRAMEWDT, 214-6, 2, WA_RIGHT };
const WidgetConfig numConf        PROGMEM = { 20, 120+30, 52, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { 20+TFT_FRAMEWDT, 66, 2, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { 20+TFT_FRAMEWDT, 90, 2, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { 20+TFT_FRAMEWDT, 130, 2, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { 20+TFT_FRAMEWDT, 154, 2, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { 12, 176, 52, WA_RIGHT };  /* 52 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { 20+TFT_FRAMEWDT, 100, 1, WA_LEFT };

const WidgetConfig bootWdtConf    PROGMEM = { 20, 162, 1, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 20+90, 14, 4 };
const BitrateConfig fullbitrateConf PROGMEM = {{DSP_WIDTH-TFT_FRAMEWDT-22, 43, 2, WA_LEFT}, 42 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = { 21, 100, 4, 2, 10, 2 };

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "WiFi %d";
const char          iptxtFmt[]    PROGMEM = "\010 %s";
const char         voltxtFmt[]    PROGMEM = "\023\025%d";
const char        bitrateFmt[]    PROGMEM = "%d kBs";

/* MOVES  */                             /* { left, top, width } */
const MoveConfig    clockMove     PROGMEM = { 8, 176, -1 };
const MoveConfig   weatherMove    PROGMEM = { 20+TFT_FRAMEWDT, 97, MAX_WIDTH };
const MoveConfig   weatherMoveVU  PROGMEM = { 20+64+TFT_FRAMEWDT, 97, 250 };

#endif
