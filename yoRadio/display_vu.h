#if ENABLE_VU_METER
#ifndef display_vu_h
#define display_vu_h
#include "player.h"

#ifdef VU_PARAMS
enum : uint16_t VU_PARAMS;
#else
/*
 * vu left              - left position
 * vu top               - top position
 * band width           - width of band
 * band height          - height of band
 * band space           - space between bands
 * num of bands         - num of bands
 * max samples          - for i2s dac: count of measurements before fixing the value
 * horisontal           - bands orientation
 * Max/Min Bands Color  - color of bands
 */

/**********************************************************************************************************************************************************************************/
/*                vu left  |  vu top    | band width  | band height | band space |  num of bands | max samples | horisontal   | Max Bands Color          |  Min Bands Color       */
/**********************************************************************************************************************************************************************************/
#if DSP_MODEL==DSP_ST7735 && DTYPE==INITR_BLACKTAB        /* ST7735 160x128 */
enum : uint16_t { VU_X = 4,   VU_Y = 50,  VU_BW = 10,   VU_BH = 44,   VU_BS = 2,    VU_NB = 8,     VU_BMS = 2,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = SILVER };
#elif DSP_MODEL==DSP_ST7735 && DTYPE==INITR_144GREENTAB   /* ST7735 128x128 */
enum : uint16_t { VU_X = 4,   VU_Y = 45,  VU_BW = 60,   VU_BH = 8,    VU_BS = 0,    VU_NB = 10,    VU_BMS = 3,   VU_HOR = 1,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = SILVER };
#define GREENTAB128
#elif DSP_MODEL==DSP_ILI9341                              /* ILI9341 320x240  */
enum : uint16_t { VU_X = 4,   VU_Y = 100, VU_BW = 20,   VU_BH = 86,   VU_BS = 4,    VU_NB = 10,    VU_BMS = 2,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY   };
#elif DSP_MODEL==DSP_ST7789                               /* ST7789 320x240 */
enum : uint16_t { VU_X = 4,   VU_Y = 100, VU_BW = 20,   VU_BH = 86,   VU_BS = 4,    VU_NB = 10,    VU_BMS = 3,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY   };
#elif DSP_MODEL==DSP_ST7789_240                           /* ST7789 240x240 */
enum : uint16_t { VU_X = 4,   VU_Y = 90,  VU_BW = 120,  VU_BH = 20,   VU_BS = 0,    VU_NB = 12,    VU_BMS = 3,   VU_HOR = 1,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY   };
#elif DSP_MODEL==DSP_ILI9225                              /* ILI9225 220x176 */
enum : uint16_t { VU_X = 4,   VU_Y = 74,  VU_BW = 13,   VU_BH = 60,   VU_BS = 2,    VU_NB = 10,    VU_BMS = 2,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY   };
#elif (DSP_MODEL==DSP_ST7735 && DTYPE==INITR_MINI160x80) || (DSP_MODEL==DSP_GC9106)  /* ST7735 160x80, GC9106 160x80 */
enum : uint16_t { VU_X = 1,   VU_Y = 30,  VU_BW = 12,   VU_BH = 36,   VU_BS = 4,    VU_NB = 8,     VU_BMS = 2,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY   };
#else
#error YOUR DISPLAY DOES NOT SUPPORT ENABLE_VU_METER FEATURE YET
#endif
#endif //VU_PARAMS
/**********************************************************************************************************************************************************************************/

void drawVU(DspCore *dsp);

GFXcanvas16 gfxc(VU_BW*2+VU_BS,VU_BH);

void drawVU(DspCore *dsp){
  if(display.mode!=PLAYER && display.mode!=VOL) return;
#ifdef GREENTAB128
  if(display.mode==VOL) return;
#endif
  player.getVUlevel();
  static uint16_t samples_cnt, measL, measR;
  uint16_t bandColor;
  samples_cnt++;
  uint16_t dimension = VU_HOR?VU_BW:VU_BH;
  uint8_t L = map((VS1053_CS!=255)?player.vuLeft:log(player.vuLeft)*38+45, 255, 0, 0, dimension);
  uint8_t R = map((VS1053_CS!=255)?player.vuRight:log(player.vuRight)*38+45, 255, 0, 0, dimension);
  if(player.isRunning()){
    if(L>measL) measL=L;
    if(R>measR) measR=R;
  }else{
    if(measL<dimension) measL+=2;
    if(measR<dimension) measR+=2;
  }
#if VS1053_CS==255
  if(samples_cnt<VU_BMS) return;
#endif
  samples_cnt=0;
  uint8_t h=(dimension/VU_NB)-2;
  for(int i=0; i<dimension; i++){
    if(i%(dimension/VU_NB)==0){
      if(VU_HOR){
        bandColor = (i>VU_BW-(VU_BW/VU_NB)*4)?VU_COLOR_MAX:VU_COLOR_MIN;
        gfxc.fillRect(i, 0, h, VU_BH, bandColor);
        gfxc.fillRect(i+VU_BW+VU_BS, 0, h, VU_BH, bandColor);
      }else{
        bandColor = (i<(VU_BH/VU_NB)*3)?VU_COLOR_MAX:VU_COLOR_MIN;
        gfxc.fillRect(0, i, VU_BW, h, bandColor);
        gfxc.fillRect(VU_BW+VU_BS, i, VU_BW, h, bandColor);
      }
    }
  }
  if(VU_HOR){
    gfxc.fillRect(VU_BW-measL, 0, measL, VU_BW, TFT_BG);
    gfxc.fillRect(VU_BW*2+VU_BS-measR, 0, measR, VU_BW, TFT_BG);
    dsp->drawRGBBitmap(VU_X, (display.mode==VOL && DSP_MODEL==DSP_ST7789_240)?VU_Y-40:VU_Y, gfxc.getBuffer(), VU_BW*2+VU_BS, VU_BH);
  }else{
    gfxc.fillRect(0, 0, VU_BW, measL, TFT_BG);
    gfxc.fillRect(VU_BW+VU_BS, 0, VU_BW, measR, TFT_BG);
    dsp->drawRGBBitmap(VU_X, VU_Y, gfxc.getBuffer(), VU_BW*2+VU_BS, VU_BH);
  }
  if(player.isRunning()){
    measL=0;
    measR=0;
  }
}

#endif
#endif
