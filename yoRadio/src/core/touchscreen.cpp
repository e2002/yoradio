#include "options.h"
#if (TS_MODEL!=TS_MODEL_UNDEFINED) && (DSP_MODEL!=DSP_DUMMY)

#include "touchscreen.h"
#include "config.h"
#include "controls.h"
#include "display.h"
#include "../displays/dspcore.h"
#include "player.h"

#ifndef TS_X_MIN
  #define TS_X_MIN              400
#endif
#ifndef TS_X_MAX
  #define TS_X_MAX              3800
#endif
#ifndef TS_Y_MIN
  #define TS_Y_MIN              260
#endif
#ifndef TS_Y_MAX
  #define TS_Y_MAX              3800
#endif
#ifndef TS_STEPS
  #define TS_STEPS              40
#endif

#if TS_MODEL==TS_MODEL_XPT2046
  #ifdef TS_SPIPINS
    SPIClass  TSSPI(HSPI);
  #endif
  #include <XPT2046_Touchscreen.h>
  XPT2046_Touchscreen ts(TS_CS);
  typedef TS_Point TSPoint;
#elif TS_MODEL==TS_MODEL_GT911
  #include "TAMC_GT911.h"
  TAMC_GT911 ts = TAMC_GT911(TS_SDA, TS_SCL, TS_INT, TS_RST, DSP_WIDTH, DSP_HEIGHT);
  typedef TP_Point TSPoint;
#endif

void TouchScreen::init(){
  
#if TS_MODEL==TS_MODEL_XPT2046
  #ifdef TS_SPIPINS
    TSSPI.begin(TS_SPIPINS);
    ts.begin(TSSPI);
  #else
    #if TS_HSPI
      ts.begin(SPI2);
    #else
      ts.begin();
    #endif
  #endif
  ts.setRotation(config.store.fliptouch?3:1);
#endif
#if TS_MODEL==TS_MODEL_GT911
  ts.begin();
  ts.setRotation(config.store.fliptouch?0:2);
#endif
  _width  = dsp.width();
  _height = dsp.height();
#if TS_MODEL==TS_MODEL_GT911
  ts.setResolution(_width, _height);
#endif
}

tsDirection_e TouchScreen::_tsDirection(uint16_t x, uint16_t y) {
  int16_t dX = x - _oldTouchX;
  int16_t dY = y - _oldTouchY;
  if (abs(dX) > 20 || abs(dY) > 20) {
    if (abs(dX) > abs(dY)) {
      if (dX > 0) {
        return TSD_RIGHT;
      } else {
        return TSD_LEFT;
      }
    } else {
      if (dY > 0) {
        return TSD_DOWN;
      } else {
        return TSD_UP;
      }
    }
  } else {
    return TDS_REQUEST;
  }
}

void TouchScreen::flip(){
#if TS_MODEL==TS_MODEL_XPT2046
  ts.setRotation(config.store.fliptouch?3:1);
#endif
#if TS_MODEL==TS_MODEL_GT911
  ts.setRotation(config.store.fliptouch?0:2);
#endif
}

void TouchScreen::loop(){
  uint16_t touchX, touchY;
  static bool wastouched = true;
  static uint32_t touchLongPress;
  static tsDirection_e direct;
  static uint16_t touchVol, touchStation;
  if (!_checklpdelay(20, _touchdelay)) return;
#if TS_MODEL==TS_MODEL_GT911
  ts.read();
#endif
  bool istouched = _istouched();
  if(istouched){
  #if TS_MODEL==TS_MODEL_XPT2046
    TSPoint p = ts.getPoint();
    touchX = map(p.x, TS_X_MIN, TS_X_MAX, 0, _width);
    touchY = map(p.y, TS_Y_MIN, TS_Y_MAX, 0, _height);
    if (config.store.dbgtouch) Serial.printf("raw: %d,%d mapped: %d,%d res: %d,%d \n",p.x,p.y,touchX,touchY,_width, _height);
  #elif TS_MODEL==TS_MODEL_GT911
    TSPoint p = ts.points[0];
    touchX = p.x;
    touchY = p.y;
    if (config.store.dbgtouch) Serial.printf("raw: %d,%d res: %d,%d \n",touchX,touchY,_width, _height);
  #endif
  if (!wastouched) { /*     START TOUCH     */
      _oldTouchX = touchX;
      _oldTouchY = touchY;
      touchVol = touchX;
      touchStation = touchY;
      direct = TDS_REQUEST;
      touchLongPress=millis();
    } else { /*     SWIPE TOUCH     */
      direct = _tsDirection(touchX, touchY);
      switch (direct) {
        case TSD_LEFT:
        case TSD_RIGHT: {
            touchLongPress=millis();
            if(display.mode()==PLAYER || display.mode()==VOL){
              int16_t xDelta = map(abs(touchVol - touchX), 0, _width, 0, TS_STEPS);
              display.putRequest(NEWMODE, VOL);
              if (xDelta>1) {
                controlsEvent((touchVol - touchX)<0);
                touchVol = touchX;
              }
            }
            break;
          }
        case TSD_UP:
        case TSD_DOWN: {
            touchLongPress=millis();
            if(display.mode()==PLAYER || display.mode()==STATIONS){
              int16_t yDelta = map(abs(touchStation - touchY), 0, _height, 0, TS_STEPS);
              display.putRequest(NEWMODE, STATIONS);
              if (yDelta>1) {
                controlsEvent((touchStation - touchY)<0);
                touchStation = touchY;
              }
            }
            break;
          }
        default:
            break;
      }
    }
  }else{

#define DSP_WIDTH_LOW (uint16_t)(DSP_WIDTH * 0.2)
#define DSP_WIDTH_HIGH (uint16_t)(DSP_WIDTH * (1-0.2))
#define DSP_HEIGHT_LOW (uint16_t)(DSP_HEIGHT * 0.2)
#define DSP_HEIGHT_HIGH (uint16_t)(DSP_HEIGHT * (1-0.2))

    if (wastouched) {/*     END TOUCH     */
      if (direct == TDS_REQUEST) {
        uint32_t pressTicks = millis()-touchLongPress;
        // short touch
	if( pressTicks < BTN_PRESS_TICKS*2){
          if(pressTicks > 50){
#if TOUCH_BUTTONS
	    if (config.store.dbgtouch) Serial.printf("oldTouch x: %d y: %d\n", _oldTouchX, _oldTouchY);
	    if (config.store.dbgtouch) Serial.printf("ul: %d,%d lr: %d,%d \n", DSP_WIDTH_LOW, DSP_HEIGHT_LOW, DSP_WIDTH_HIGH, DSP_HEIGHT_HIGH);
	    if(_oldTouchX < DSP_WIDTH_LOW && _oldTouchY < DSP_HEIGHT_LOW) {
	      if (config.store.dbgtouch) Serial.println("upper left");
	      onBtnDoubleClick(EVT_BTNLEFT);
	    }
	    if((_oldTouchX < DSP_WIDTH_LOW) && (_oldTouchY > DSP_HEIGHT_HIGH)) {
	      if (config.store.dbgtouch) Serial.println("lower left");
	      player.stepVol(false);
	    }
	    if(_oldTouchX > DSP_WIDTH_HIGH && _oldTouchY < DSP_HEIGHT_LOW){
	      if (config.store.dbgtouch) Serial.println("upper right");
	      onBtnDoubleClick(EVT_BTNRIGHT);
	    }
	    if(_oldTouchX > DSP_WIDTH_HIGH && _oldTouchY > DSP_HEIGHT_HIGH) {
	      if (config.store.dbgtouch) Serial.println("lower right");
	      player.stepVol(true);
	    }
	    if(_oldTouchX > DSP_WIDTH_LOW && _oldTouchX < DSP_WIDTH_HIGH &&
	       _oldTouchY > DSP_HEIGHT_LOW && _oldTouchY < DSP_HEIGHT_HIGH) {
	      if (config.store.dbgtouch) Serial.println("center");
	      onBtnClick(EVT_BTNCENTER);
	    }
#else
	      onBtnClick(EVT_BTNCENTER);
#endif // TOUCH_BUTTONS
	  }
	// long touch
	}else{
          display.putRequest(NEWMODE, display.mode() == PLAYER ? STATIONS : PLAYER);
        }
      }
      direct = TSD_STAY;
    }
  }
  wastouched = istouched;
}

bool TouchScreen::_checklpdelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

bool TouchScreen::_istouched(){
#if TS_MODEL==TS_MODEL_XPT2046
  return ts.touched();
#elif TS_MODEL==TS_MODEL_GT911
  return ts.isTouched;
#endif
}

#endif  // TS_MODEL!=TS_MODEL_UNDEFINED
