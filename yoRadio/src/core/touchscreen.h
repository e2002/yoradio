#ifndef touchscreen_h
#define touchscreen_h
#include "Arduino.h"

enum tsDirection_e { TSD_STAY, TSD_LEFT, TSD_RIGHT, TSD_UP, TSD_DOWN, TDS_REQUEST };

class TouchScreen {
  public:
    TouchScreen() {}
    void init();
    void loop();
    void flip();
  private:
    uint16_t _oldTouchX, _oldTouchY, _width, _height;
    uint32_t _touchdelay;
    bool _checklpdelay(int m, uint32_t &tstamp);
    tsDirection_e _tsDirection(uint16_t x, uint16_t y);
    bool _istouched();
};

extern TouchScreen touchscreen;

#endif
