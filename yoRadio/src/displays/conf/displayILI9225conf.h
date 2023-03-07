/*************************************************************************************
    ILI9225 220x176 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayILI9225conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayILI9225conf_h
#define displayILI9225conf_h

#define DSP_WIDTH       220
#define DSP_HEIGHT      176
#define TFT_FRAMEWDT    4
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2

#define bootLogoTop     28
//#define DSP_QUEUE_TICKS 5

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 2, WA_LEFT }, 140, true, DSP_WIDTH+10, 5000, 4, 30 };
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, 28, 1, WA_LEFT }, 140, true, MAX_WIDTH, 5000, 3, 25 };
const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT, 40, 1, WA_LEFT }, 140, true, MAX_WIDTH, 5000, 3, 25 };
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 80, 2, WA_LEFT }, 140, true, DSP_WIDTH+10, 1000, 4, 30 };
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 2, WA_CENTER }, 140, false, MAX_WIDTH, 0, 4, 30 };
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT, DSP_HEIGHT-TFT_FRAMEWDT-16, 2, WA_LEFT }, 140, false, DSP_WIDTH+10, 0, 3, 25 };
const ScrollConfig weatherConf    PROGMEM = {{ TFT_FRAMEWDT, 50, 2, WA_LEFT }, 140, true, DSP_WIDTH+10, 0, 4, 30 };

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 22, false };
const FillConfig   metaBGConfInv  PROGMEM = {{ 0, 22, 0, WA_LEFT }, DSP_WIDTH, 1, false };
const FillConfig   volbarConf     PROGMEM = {{ TFT_FRAMEWDT, DSP_HEIGHT-TFT_FRAMEWDT-4, 0, WA_LEFT }, MAX_WIDTH, 4, true };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 76, 0, WA_LEFT }, DSP_WIDTH, 22, false };
const FillConfig  heapbarConf     PROGMEM = {{ 0, DSP_HEIGHT-1, 0, WA_LEFT }, DSP_WIDTH, 1, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 150, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { TFT_FRAMEWDT+6, DSP_HEIGHT-TFT_FRAMEWDT-14-14, 1, WA_RIGHT };
const WidgetConfig voltxtConf     PROGMEM = { TFT_FRAMEWDT, DSP_HEIGHT-TFT_FRAMEWDT-14, 1, WA_LEFT };
const WidgetConfig  iptxtConf     PROGMEM = { TFT_FRAMEWDT, DSP_HEIGHT-TFT_FRAMEWDT-14, 1, WA_CENTER };
const WidgetConfig   rssiConf     PROGMEM = { TFT_FRAMEWDT, DSP_HEIGHT-TFT_FRAMEWDT-14, 1, WA_RIGHT };
const WidgetConfig numConf        PROGMEM = { 0, 110, 35, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { TFT_FRAMEWDT, 38, 2, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { TFT_FRAMEWDT, 62, 2, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { TFT_FRAMEWDT, 102, 2, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { TFT_FRAMEWDT, 126, 2, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { 12, 120, 35, WA_RIGHT };  /* 35 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT, 58, 1, WA_LEFT };

const WidgetConfig bootWdtConf    PROGMEM = { 0, 130, 1, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 90, 14, 4 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = { 19, 90, 2, 2, 10, 2 };
/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "WiFi %d";
const char          iptxtFmt[]    PROGMEM = "%s";
const char         voltxtFmt[]    PROGMEM = "\023\025%d";
const char        bitrateFmt[]    PROGMEM = "%d kBs";

/* MOVES  */                             /* { left, top, width } */
const MoveConfig    clockMove     PROGMEM = { 2, 120, 0 };
const MoveConfig   weatherMove    PROGMEM = { TFT_FRAMEWDT, 58, DSP_WIDTH+10 };
const MoveConfig   weatherMoveVU  PROGMEM = { TFT_FRAMEWDT+46, 58, DSP_WIDTH+10-46 };

#endif
