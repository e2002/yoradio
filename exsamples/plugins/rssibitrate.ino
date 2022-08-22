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
  int16_t vTop = dsp->height() - TFT_FRAMEWDT * 2 - TFT_LINEHGHT - 2;     /* vTop - Y cordnate of the bitrate string */;
  sprintf(buf, "RSSI:000dBm");
  dsp->setTextSize(1);
  dsp->getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  dsp->fillRect(dsp->width() - w - TFT_FRAMEWDT /* left */, vTop /* top */, w /* width */, TFT_LINEHGHT-2 /* height */, config.theme.background /* background color */);
  sprintf(buf, "%dkBits", config.station.bitrate);
  dsp->getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  if(cnt<2){
    cnt++;
    return true;                                                          /* print RSSI and retrn */
  }
  cnt++;
  if(cnt>3) cnt=0;
  dsp->setTextColor(config.theme.rssi,config.theme.background);
  dsp->setCursor(dsp->width() - w - TFT_FRAMEWDT, vTop);
  dsp->print(buf);                                                        /* print bitrate */
  return false;                                                           /* disable to print RSSI */
}

#endif
