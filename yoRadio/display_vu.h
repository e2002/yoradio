#ifndef display_vu_h
#define display_vu_h
#include "player.h"

#ifdef VU_PARAMS2
enum : uint16_t VU_PARAMS2;
#else
/*
 * vu left              - left position
 * vu top               - top position
 * band width           - width of band
 * band height          - height of band
 * band space           - space between bands
 * num of bands         - num of bands
 * fade speed           - fade speed
 * horisontal           - bands orientation
 * Max/Min Bands Color  - color of bands
 */

/**********************************************************************************************************************************************************************************/
/*                vu left  |  vu top    | band width  | band height | band space |  num of bands | fade speed  | horisontal   | Max Bands Color          |  Min Bands Color       */
/**********************************************************************************************************************************************************************************/

#if DSP_MODEL==DSP_ST7735 && DTYPE==INITR_BLACKTAB        /* ST7735 160x128 */
enum : uint16_t { VU_X = 4,   VU_Y = 50,  VU_BW = 10,   VU_BH = 44,   VU_BS = 2,    VU_NB = 8,     VU_FS = 2,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY      };

#elif DSP_MODEL==DSP_ST7735 && DTYPE==INITR_144GREENTAB   /* ST7735 128x128 */
enum : uint16_t { VU_X = 4,   VU_Y = 97,  VU_BW = 60,   VU_BH = 8,    VU_BS = 0,    VU_NB = 10,    VU_FS = 2,   VU_HOR = 1,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = DARK_GRAY };
#define GREENTAB128

#elif DSP_MODEL==DSP_ILI9341                              /* ILI9341 320x240  */
enum : uint16_t { VU_X = 4,   VU_Y = 116, VU_BW = 24,   VU_BH = 80,   VU_BS = 4,    VU_NB = 8,     VU_FS = 5,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY      };

#elif DSP_MODEL==DSP_ST7789                               /* ST7789 320x240 */
enum : uint16_t { VU_X = 4,   VU_Y = 116, VU_BW = 24,   VU_BH = 80,   VU_BS = 4,    VU_NB = 8,     VU_FS = 5,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY      };

#elif DSP_MODEL==DSP_ST7789_240                           /* ST7789 240x240 */
enum : uint16_t { VU_X = 4,   VU_Y = 90,  VU_BW = 120,  VU_BH = 20,   VU_BS = 0,    VU_NB = 12,    VU_FS = 3,   VU_HOR = 1,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY      };

#elif DSP_MODEL==DSP_ILI9225                              /* ILI9225 220x176 */
enum : uint16_t { VU_X = 4,   VU_Y = 80,  VU_BW = 13,   VU_BH = 56,   VU_BS = 2,    VU_NB = 8,     VU_FS = 4,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY      };

#elif (DSP_MODEL==DSP_ST7735 && DTYPE==INITR_MINI160x80) || (DSP_MODEL==DSP_GC9106)  /* ST7735 160x80, GC9106 160x80 */
enum : uint16_t { VU_X = 1,   VU_Y = 30,  VU_BW = 12,   VU_BH = 36,   VU_BS = 4,    VU_NB = 8,     VU_FS = 2,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY      };

#endif
#endif //VU_PARAMS
/**********************************************************************************************************************************************************************************/

void drawVU(DspCore *dsp);

GFXcanvas16 gfxc(VU_BW*2+VU_BS,VU_BH);

void drawVU(DspCore *dsp){
  if(!config.store.vumeter) return;
  if(display.mode!=PLAYER && display.mode!=VOL) return;
#ifdef GREENTAB128
  if(display.mode==VOL) return;
#endif
#if !defined(USE_NEXTION) && I2S_DOUT==255
  player.getVUlevel();
#endif
  static uint16_t /*samples_cnt, */measL, measR;
  uint16_t bandColor;
  uint16_t dimension = VU_HOR?VU_BW:VU_BH;
  uint8_t L = map(player.vuLeft, 255, 0, 0, dimension);
  uint8_t R = map(player.vuRight, 255, 0, 0, dimension);
  if(player.isRunning()){
    measL=(L>=measL)?measL+VU_FS:L;
    measR=(R>=measR)?measR+VU_FS:R;
  }else{
    if(measL<dimension) measL+=VU_FS;
    if(measR<dimension) measR+=VU_FS;
  }
  if(measL>dimension) measL=dimension;
  if(measR>dimension) measR=dimension;
  uint8_t h=(dimension/VU_NB)-2;
  for(int i=0; i<dimension; i++){
    if(i%(dimension/VU_NB)==0){
      if(VU_HOR){
        #ifndef BOOMBOX_STYLE
        bandColor = (i>VU_BW-(VU_BW/VU_NB)*4)?VU_COLOR_MAX:VU_COLOR_MIN;
        gfxc.fillRect(i, 0, h, VU_BH, bandColor);
        gfxc.fillRect(i+VU_BW+VU_BS, 0, h, VU_BH, bandColor);
        #else
        bandColor = (i>(VU_BW/VU_NB))?VU_COLOR_MIN:VU_COLOR_MAX;
        gfxc.fillRect(i, 0, h, VU_BH, bandColor);
        bandColor = (i>VU_BW-(VU_BW/VU_NB)*3)?VU_COLOR_MAX:VU_COLOR_MIN;
        gfxc.fillRect(i+VU_BW+VU_BS, 0, h, VU_BH, bandColor);
        #endif
      }else{
        bandColor = (i<(VU_BH/VU_NB)*3)?VU_COLOR_MAX:VU_COLOR_MIN;
        gfxc.fillRect(0, i, VU_BW, h, bandColor);
        gfxc.fillRect(VU_BW+VU_BS, i, VU_BW, h, bandColor);
      }
    }
  }
  if(VU_HOR){
    #ifndef BOOMBOX_STYLE
    gfxc.fillRect(VU_BW-measL, 0, measL, VU_BW, TFT_BG);
    gfxc.fillRect(VU_BW*2+VU_BS-measR, 0, measR, VU_BW, TFT_BG);
    dsp->drawRGBBitmap(VU_X, (display.mode==VOL && DSP_MODEL==DSP_ST7789_240)?VU_Y-40:VU_Y, gfxc.getBuffer(), VU_BW*2+VU_BS, VU_BH);
    #else
    gfxc.fillRect(0, 0, VU_BW-(VU_BW-measL), VU_BW, TFT_BG);
    gfxc.fillRect(VU_BW*2+VU_BS-measR, 0, measR, VU_BW, TFT_BG);
    dsp->drawRGBBitmap(VU_X, (display.mode==VOL && DSP_MODEL==DSP_ST7789_240)?VU_Y-40:VU_Y, gfxc.getBuffer(), VU_BW*2+VU_BS, VU_BH);
    #endif
  }else{
    gfxc.fillRect(0, 0, VU_BW, measL, TFT_BG);
    gfxc.fillRect(VU_BW+VU_BS, 0, VU_BW, measR, TFT_BG);
    dsp->drawRGBBitmap(VU_X, VU_Y, gfxc.getBuffer(), VU_BW*2+VU_BS, VU_BH);
  }
}
#endif
