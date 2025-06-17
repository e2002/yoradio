/*************************************************************************************
    ST7796 480X320 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayST7789conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayST7789conf_h
#define displayST7789conf_h

#define DSP_WIDTH       480
#define DSP_HEIGHT      320
#define TFT_FRAMEWDT    10
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT*2

#if BITRATE_FULL
  #define TITLE_FIX 48
#else
  #define TITLE_FIX 0
#endif
#define bootLogoTop     110

#ifndef BATTERY_OFF
  #define BatX      325				// X coordinate for batt. (Координата X для батарейки)
  #define BatY      DSP_HEIGHT-38		// Y cordinate for batt. (Координата Y для батарейки)
  #define BatFS     2			// FontSize for batt. (Размер шрифта для батарейки)
  #define ProcX     375				// X coordinate for percent (Координата X для процентов заряда)
  #define ProcY     DSP_HEIGHT-38		// Y coordinate for percent (Координата Y для процентов заряда)
  #define ProcFS    2			// FontSize for percent (Размер шрифта для процентов заряда)
  #define VoltX      230				// X coordinate for voltage (Координата X для напряжения)
  #define VoltY      DSP_HEIGHT-38		// Y coordinate for voltage (Координата Y для напряжения)
  #define VoltFS     2			// FontSize for voltage (Размер шрифта для напряжения)
#endif

/* SROLLS  */                       /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf   PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT+1, 4, WA_LEFT }, 140, false, MAX_WIDTH+15, 5000, 7, 40 };
const ScrollConfig title1Conf   PROGMEM = {{ TFT_FRAMEWDT, 63, 3, WA_LEFT }, 140, false, MAX_WIDTH-TITLE_FIX+8, 5000, 7, 40 };
const ScrollConfig title2Conf    PROGMEM = {{ TFT_FRAMEWDT, 95, 3, WA_LEFT }, 140, false, MAX_WIDTH-TITLE_FIX+8, 5000, 7, 40 };
const ScrollConfig playlistConf  PROGMEM = {{ TFT_FRAMEWDT, 146, 3, WA_LEFT }, 140, false, MAX_WIDTH, 1000, 7, 40 };
const ScrollConfig apTitleConf  PROGMEM = {{ TFT_FRAMEWDT, TFT_FRAMEWDT, 4, WA_CENTER }, 140, false, MAX_WIDTH, 0, 7, 40 };
const ScrollConfig apSettConf  PROGMEM = {{ TFT_FRAMEWDT, 320-TFT_FRAMEWDT-16, 2, WA_LEFT }, 140, false, MAX_WIDTH, 0, 7, 40 };
const ScrollConfig weatherConf    PROGMEM = {{ TFT_FRAMEWDT, 126, 2, WA_LEFT }, 250, false, MAX_WIDTH+10, 0, 7, 40 };  // Weather (погода)

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ 0, 0, 0, WA_LEFT }, DSP_WIDTH, 50, false };
const FillConfig   metaBGConfInv  PROGMEM = {{ 0, 50, 0, WA_LEFT }, DSP_WIDTH, 2, false };
const FillConfig   volbarConf     PROGMEM = {{ 0, DSP_HEIGHT-19, 0, WA_LEFT }, DSP_WIDTH, 8, true };
const FillConfig  playlBGConf    PROGMEM = {{ 0, 138, 0, WA_LEFT }, DSP_WIDTH, 36, false };
const FillConfig  heapbarConf   PROGMEM = {{ 0, DSP_HEIGHT-7, 0, WA_LEFT }, DSP_WIDTH, 6, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, 235, 2, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { 0, 63, 2, WA_RIGHT };
const WidgetConfig voltxtConf     PROGMEM = { 0, DSP_HEIGHT-38, 2, WA_CENTER };
const WidgetConfig  iptxtConf     PROGMEM = { TFT_FRAMEWDT, DSP_HEIGHT-38, 2, WA_LEFT };
const WidgetConfig   rssiConf     PROGMEM = { TFT_FRAMEWDT-5, DSP_HEIGHT-38-6, 3, WA_RIGHT };
const WidgetConfig numConf        PROGMEM = { 0, 200, 70, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { TFT_FRAMEWDT, 88, 3, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { TFT_FRAMEWDT, 120, 3, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { TFT_FRAMEWDT, 173, 3, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { TFT_FRAMEWDT, 205, 3, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { TFT_FRAMEWDT+70, 237, 70, WA_RIGHT };  /* 70 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT, 137, 1, WA_LEFT };

const WidgetConfig bootWdtConf    PROGMEM = { 0, 216, 2, WA_CENTER };
const ProgressConfig bootPrgConf  PROGMEM = { 90, 14, 4 };
const BitrateConfig fullbitrateConf PROGMEM = {{DSP_WIDTH-TITLE_FIX-2, 66, 2, WA_LEFT}, TITLE_FIX+2 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = { 35, 130, 4, 2, 10, 10 };
//const VUBandsConfig bandsConf     PROGMEM = { (MAX_WIDTH-TFT_FRAMEWDT*3)/2, 20, TFT_FRAMEWDT, 0, 10, 5 };	//Boombox

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
const char           rssiFmt[]    PROGMEM = "WiFi %d";
const char          iptxtFmt[]    PROGMEM = "%s";
//const char         voltxtFmt[]    PROGMEM = "\023\025%d";
const char         voltxtFmt[]    PROGMEM = "";
const char        bitrateFmt[]    PROGMEM = "%d kBs";

/* MOVES  */                             /* { left, top, width (0 - auto, -1 - lock )} */
const MoveConfig    clockMove     PROGMEM = { TFT_FRAMEWDT+30, 237, 0 };
const MoveConfig   weatherMove    PROGMEM = { TFT_FRAMEWDT, 130, MAX_WIDTH+10 };
const MoveConfig   weatherMoveVU  PROGMEM = { 89, 130, MAX_WIDTH-75 };

#endif
