/*************************************************************************************
    ST7789 320x170 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayST7789conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayST7789conf_h
#define displayST7789conf_h

#define DSP_WIDTH       320
#define TFT_FRAMEWDT    3
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2

#if BITRATE_FULL
  #define TITLE_FIX 39
#else
  #define TITLE_FIX 0
#endif

#define bootLogoTop     40

#define HIDE_DATE
//#define RSSI_DIGIT true
//#define HIDE_IP
//#define HIDE_TITLE2
//#define HIDE_HEAPBAR
//#define HIDE_VU

#ifndef BATTERY_OFF
  #define RSSI_DIGIT true
  #define BatX      276		// X coordinate for batt. (Координата X для батарейки)
  #define BatY      150		// Y coordinate for batt. (Координата Y для батарейки)
  #define BatFS     2	// FontSize for batt. (Размер шрифта для батарейки)
  #define ProcX     274	// X coordinate for percent (Координата X для процентов заряда)
  #define ProcY     133	// Y coordinate for percent (Координата Y для процентов заряда)
  #define ProcFS    2	// FontSize for percent (Размер шрифта для процентов заряда)
  #define VoltX      147	// X coordinate for voltage (Координата X для напряжения)
  #define VoltY      146	// Y coordinate for voltage (Координата Y для напряжения)
  #define VoltFS     1	// FontSize for voltage (Размер шрифта для напряжения)
#endif

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf    PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 3, WA_LEFT }, 140, false, MAX_WIDTH, 5000, 5, 30 };
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT, 32, 2, WA_LEFT }, 140, false, DSP_WIDTH-TITLE_FIX-3, 5000, 4, 30 };
const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT, 51, 2, WA_LEFT }, 140, false, DSP_WIDTH-TITLE_FIX-3, 5000, 4, 30 };
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 112, 2, WA_LEFT }, 140, false, MAX_WIDTH, 1000, 4, 30 };
const ScrollConfig apTitleConf   PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 3, WA_CENTER }, 140, false, MAX_WIDTH, 0, 4, 20 };
const ScrollConfig apSettConf    PROGMEM = {{ TFT_FRAMEWDT, 170-20, 1, WA_LEFT }, 140, false, MAX_WIDTH, 0, 4, 30 };
const ScrollConfig weatherConf  PROGMEM = {{ TFT_FRAMEWDT, 70, 2, WA_LEFT }, 300, false, MAX_WIDTH, 0, 4, 30 };	// Weather (погода)

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig  metaBGConf       PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 28, false };
const FillConfig  metaBGConfInv  PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 2, false };
#ifdef BATTERY_OFF
  const FillConfig  volbarConf         PROGMEM = {{ 0, 156, 0, WA_LEFT }, 320, 8, true };
  const WidgetConfig   rssiConf     PROGMEM = { 0, 139, 2, WA_RIGHT };
#else
  const FillConfig  volbarConf         PROGMEM = {{ 0, 156, 0, WA_LEFT }, 273, 8, true };
  const WidgetConfig   rssiConf     PROGMEM = { 47, 146, 1, WA_RIGHT };
#endif
const FillConfig  playlBGConf      PROGMEM = {{ 0, 107, 0, WA_LEFT }, DSP_WIDTH, 24, false };
const FillConfig  heapbarConf     PROGMEM = {{ 0, 165, 0, WA_LEFT }, 273, 5, true };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 150, 1, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { TFT_FRAMEWDT, 32,  2, WA_RIGHT};
const WidgetConfig voltxtConf     PROGMEM = {  TFT_FRAMEWDT, 214,  1, WA_CENTER };
const WidgetConfig  iptxtConf     PROGMEM = { 50, 146, 1, WA_LEFT };
const WidgetConfig numConf        PROGMEM = {  0, 120, 72, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { TFT_FRAMEWDT, 40, 2, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { TFT_FRAMEWDT, 65, 2, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { TFT_FRAMEWDT, 100, 2, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { TFT_FRAMEWDT, 125, 2, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { 30, 141, 52, WA_RIGHT };  /* 52 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { 0, 91, 1, WA_LEFT };
const WidgetConfig bootWdtConf    PROGMEM = { TFT_FRAMEWDT, 120, 2, WA_CENTER };

const ProgressConfig  bootPrgConf  PROGMEM = { 100, 14, 4 };
//const BitrateConfig  fullbitrateConf   PROGMEM = {{DSP_WIDTH-TFT_FRAMEWDT-34, 36, 1, WA_LEFT}, 40 };
const BitrateConfig fullbitrateConf PROGMEM = {{DSP_WIDTH-TITLE_FIX, 29, 2, WA_CENTER}, TITLE_FIX };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf   	PROGMEM = { 20, 63, 4, 1, 9, 3 };
//const VUBandsConfig bandsConf     PROGMEM = { (MAX_WIDTH-TFT_FRAMEWDT*3)/2, 3, TFT_FRAMEWDT, 1, 10, 2 };	//Boombox

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "rssi %d dBm";
//const char          iptxtFmt[]    PROGMEM = "\010 %s";
const char          iptxtFmt[]    PROGMEM = "%s";
//const char         voltxtFmt[]    PROGMEM = "\023\025%d";
const char         voltxtFmt[]    PROGMEM = "";
const char        bitrateFmt[]    PROGMEM = "%d kBs";

/* MOVES  */                             /* { left, top, width (0 - auto, -1 - lock )} */
const MoveConfig    clockMove     PROGMEM = { 12, 141, -1 };
const MoveConfig   weatherMove    PROGMEM = { TFT_FRAMEWDT, 100, -1 };
const MoveConfig   weatherMoveVU  PROGMEM = { TFT_FRAMEWDT, 100, -1 };

#endif
