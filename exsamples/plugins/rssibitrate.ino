/**************************************************************

    An example of replase a RSSI to bitrate information alternately.
    This file must be in the root directory of the sketch.

**************************************************************/

#if DSP_MODEL==DSP_ILI9225 /* your DSP_MODEL */

bool dsp_before_rssi(DspCore *dsp){
  static int8_t cnt;
  int16_t  x1, y1;
  char buf[20];                                                           /* buffer for the bitrate string */
  uint16_t w, h;                                                          /* width & height of the bitrate string */
  int16_t vTop = dsp->height() - TFT_FRAMEWDT * 2 - TFT_LINEHGHT - 2;     /* vTop - Y cordnate of the bitrate string */

  sprintf(buf, "%d kBits", config.station.bitrate);
  dsp->getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  dsp->setTextSize(1);
  dsp->fillRect(dsp->width() - w - TFT_FRAMEWDT-40 /* left */, vTop /* top */, w+40 /* width */, TFT_LINEHGHT-4 /* height */, TFT_BG /* background color */);
  if(cnt<2){
    cnt++;
    return true;                                                          /* print RSSI and retrn */
  }
  cnt++;
  if(cnt>3) cnt=0;
  dsp->setTextColor(SILVER,TFT_BG);
  dsp->setCursor(dsp->width() - w - TFT_FRAMEWDT, vTop);
  dsp->print(buf);                                                        /* print bitrate */
  
  return false;                                                           /* disable to print RSSI */
}

#endif
