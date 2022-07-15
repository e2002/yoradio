#if ENABLE_VU_METER
#ifndef display_vu_h
#define display_vu_h
#include "player.h"

#ifdef VU_PARAMS
enum : uint16_t VU_PARAMS;
#else
/*****************************************************************************************************************************************************************************/
/*                vu left  |  vu top    | band width  | band height | band space |num of bands |max samples | horisontal  | Max Bands Color         |  Min Bands Color       */
/*****************************************************************************************************************************************************************************/
#if DSP_MODEL==DSP_ST7735 && DTYPE==INITR_BLACKTAB        /* ST7735 160x128 */
enum : uint16_t { VU_X = 4,   VU_Y = 50,  VU_BW = 10,   VU_BH = 44,   VU_BS = 2,  VU_NB = 8,    VU_BMS = 2,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = SILVER };
#elif DSP_MODEL==DSP_ST7735 && DTYPE==INITR_144GREENTAB   /* ST7735 128x128 */
enum : uint16_t { VU_X = 4,   VU_Y = 45,  VU_BW = 60,   VU_BH = 8,    VU_BS = 0,  VU_NB = 10,    VU_BMS = 3,   VU_HOR = 1,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY };
#elif DSP_MODEL==DSP_ILI9341                              /* ILI9341 320x240  */
enum : uint16_t { VU_X = 4,   VU_Y = 100, VU_BW = 20,   VU_BH = 86,   VU_BS = 4,  VU_NB = 10,    VU_BMS = 2,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY };
#elif DSP_MODEL==DSP_ST7789                               /* ST7789 320x240 */
enum : uint16_t { VU_X = 4,   VU_Y = 100, VU_BW = 20,   VU_BH = 86,   VU_BS = 4,  VU_NB = 10,    VU_BMS = 3,   VU_HOR = 0,   VU_COLOR_MAX = TFT_LOGO,   VU_COLOR_MIN = GRAY };
#else
#error YOUR DISPLAY DOES NOT SUPPORT ENABLE_VU_METER FEATURE YET
#endif
#endif //VU_PARAMS

/*****************************************************************************************************************************************************************************/

void drawVU(DspCore *dsp);

GFXcanvas16 gfxc(VU_BW*2+VU_BS,VU_BH);

void drawVU(DspCore *dsp){
  if((display.mode!=PLAYER && display.mode!=VOL)) return;
  player.getVUlevel();
  static uint16_t samples_cnt, measL, measR;
  samples_cnt++;
  uint16_t dimension = VU_HOR?VU_BW:VU_BH;
  uint8_t L = map((VS1053_CS!=255)?player.vuLeft:log(player.vuLeft)*38+45, 255, 0, 0, dimension);
  uint8_t R = map((VS1053_CS!=255)?player.vuRight:log(player.vuRight)*38+45, 255, 0, 0, dimension);
  if(player.isRunning()){
    if(L>measL) measL=L;
    if(R>measR) measR=R;
  }else{
    if(measL<255) measL+=2;
    if(measR<255) measR+=2;
  }
#if VS1053_CS==255
  if(samples_cnt<VU_BMS) return;
#endif
  samples_cnt=0;
  uint8_t h=(dimension/VU_NB)-2;
  for(int i=0; i<dimension; i++){
    if(i%(dimension/VU_NB)==0){
      if(VU_HOR){
        uint16_t bandColor = (i>VU_BW-(VU_BW/VU_NB)*4)?VU_COLOR_MAX:VU_COLOR_MIN;
        gfxc.fillRect(i, 0, h, VU_BH, bandColor);
        gfxc.fillRect(i+VU_BW+VU_BS, 0, h, VU_BH, bandColor);
      }else{
        uint16_t bandColor = (i<(VU_BH/VU_NB)*3)?VU_COLOR_MAX:VU_COLOR_MIN;
        gfxc.fillRect(0, i, VU_BW, h, bandColor);
        gfxc.fillRect(VU_BW+VU_BS, i, VU_BW, h, bandColor);
      }
    }
  }
  if(VU_HOR){
    gfxc.fillRect(VU_BW-measL, 0, measL, VU_BW, TFT_BG);
    gfxc.fillRect(VU_BW*2+VU_BS-measR, 0, measR, VU_BW, TFT_BG);
    dsp->drawRGBBitmap (VU_X, VU_Y, gfxc.getBuffer(), 120, VU_BH);
  }else{
    gfxc.fillRect(0, 0, VU_BW, measL, TFT_BG);
    gfxc.fillRect(VU_BW+VU_BS, 0, VU_BW, measR, TFT_BG);
    dsp->drawRGBBitmap (VU_X, VU_Y, gfxc.getBuffer(), VU_BW*2+VU_BS, VU_BH);
  }
  if(player.isRunning()){
    measL=0;
    measR=0;
  }
}

#endif
#endif
