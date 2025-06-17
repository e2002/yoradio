/*************************************************************************************
    ILI9341 320x240 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayILI9341conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayILI9341conf_h
#define displayILI9341conf_h

#define DSP_WIDTH       320
#define TFT_FRAMEWDT    4
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2

#define bootLogoTop     68

#ifndef BATTERY_OFF
  #define BatX      TFT_FRAMEWDT+1	// X coordinate for batt. (Координата X для батарейки)
  #define BatY      164				// Y coordinate for batt. (Координата Y для батарейки)
  #define BatFS     2				// FontSize for batt. (Размер шрифта для батарейки)
  #define ProcX     TFT_FRAMEWDT	// X coordinate for percent (Координата X для процентов заряда)
  #define ProcY     146				// Y coordinate for percent (Координата Y для процентов заряда)
  #define ProcFS    2				// FontSize for percent (Размер шрифта для процентов заряда)
  #define VoltX      180				// X coordinate for voltage (Координата X для напряжения)		(TFT_FRAMEWDT)
  #define VoltY      208				// Y coordinate for voltage (Координата X для напряжения)		(183)
  #define VoltFS     2				// FontSize for voltage (Размер шрифта для напряжения)
#endif

/* SROLLS  */                       /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf    PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT+4, 3, WA_CENTER }, 140, false, MAX_WIDTH, 5000, 5, 30 };
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, 40, 2, WA_LEFT }, 140, false, MAX_WIDTH, 5000, 4, 30 };
const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT, 60, 2, WA_LEFT }, 140, false, MAX_WIDTH, 5000, 4, 30 };
const ScrollConfig playlistConf  PROGMEM = {{ TFT_FRAMEWDT, 112, 2, WA_LEFT }, 140, false, MAX_WIDTH, 1000, 4, 30 };
const ScrollConfig apTitleConf  PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 3, WA_CENTER }, 140, false, MAX_WIDTH, 0, 4, 20 };
const ScrollConfig apSettConf   PROGMEM = {{ TFT_FRAMEWDT, 240-24, 2, WA_LEFT }, 140, false, MAX_WIDTH, 0, 4, 30 };
const ScrollConfig weatherConf  PROGMEM = {{ 0, 74, 2, WA_LEFT }, 300, false, DSP_WIDTH, 0, 4, 40 };	// Weather (погода)

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 34, false };
const FillConfig   metaBGConfInv  PROGMEM = {{ TFT_FRAMEWDT, 32, 0, WA_LEFT }, MAX_WIDTH, 2, false };
const FillConfig   volbarConf     PROGMEM = {{ TFT_FRAMEWDT, 225, 0, WA_LEFT }, MAX_WIDTH, 8, true };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 107, 0, WA_LEFT }, DSP_WIDTH, 24, false };
const FillConfig  heapbarConf     PROGMEM = {{ TFT_FRAMEWDT, 235, 0, WA_LEFT }, MAX_WIDTH, 5, true };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 188, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { TFT_FRAMEWDT, 20, 1, WA_RIGHT };
const WidgetConfig voltxtConf     PROGMEM = { TFT_FRAMEWDT, 214, 1, WA_CENTER };
const WidgetConfig  iptxtConf     PROGMEM = { TFT_FRAMEWDT, 208, 2, WA_LEFT };
const WidgetConfig   rssiConf     PROGMEM = { TFT_FRAMEWDT, 208, 2, WA_RIGHT };
const WidgetConfig numConf        PROGMEM = { 0, 150, 52, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { TFT_FRAMEWDT, 66, 2, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { TFT_FRAMEWDT, 90, 2, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { TFT_FRAMEWDT, 130, 2, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { TFT_FRAMEWDT, 154, 2, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { TFT_FRAMEWDT+20, 152, 52, WA_RIGHT };  /* 52 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT, 183, 1, WA_CENTER };

const WidgetConfig bootWdtConf    PROGMEM = { TFT_FRAMEWDT, 162, 2, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 100, 14, 4 };
const BitrateConfig fullbitrateConf PROGMEM = {{TFT_FRAMEWDT+3, 100, 2, WA_RIGHT}, 42 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
//const VUBandsConfig bandsConf     PROGMEM = { 150, 20, 8, 1, 10, 3 };
const VUBandsConfig bandsConf     PROGMEM = { (MAX_WIDTH-TFT_FRAMEWDT*3)/2, 20, TFT_FRAMEWDT, 0, 10, 3 };	//Boombox

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "WiFi %d";
//const char          iptxtFmt[]    PROGMEM = "\010 %s";
const char          iptxtFmt[]    PROGMEM = "%s";
//const char         voltxtFmt[]    PROGMEM = "\023\025%d";
const char         voltxtFmt[]    PROGMEM = "";
const char        bitrateFmt[]    PROGMEM = "%d kBs";

/* MOVES  */                             /* { left, top, width (0 - auto, -1 - lock )} */
const MoveConfig    clockMove     PROGMEM = { TFT_FRAMEWDT+20, 152, 0 };
const MoveConfig   weatherMove    PROGMEM = { TFT_FRAMEWDT, 80, 0 };
const MoveConfig   weatherMoveVU  PROGMEM = { TFT_FRAMEWDT, 80, 0 };

#endif
