#include "Arduino.h"
#include "controls.h"
#include "options.h"
#include "config.h"
#include "player.h"
#include "display.h"

long encOldPosition  = 0;
long enc2OldPosition  = 0;
int lpId = -1;

#define ISPUSHBUTTONS BTN_LEFT!=255 || BTN_CENTER!=255 || BTN_RIGHT!=255 || ENC_BTNB!=255 || BTN_UP!=255 || BTN_DOWN!=255 || ENC2_BTNB!=255
#if ISPUSHBUTTONS
#include "OneButton.h"
OneButton button[] {{BTN_LEFT, true, BTN_INTERNALPULLUP}, {BTN_CENTER, true, BTN_INTERNALPULLUP}, {BTN_RIGHT, true, BTN_INTERNALPULLUP}, {ENC_BTNB, true, ENC_INTERNALPULLUP}, {BTN_UP, true, BTN_INTERNALPULLUP}, {BTN_DOWN, true, BTN_INTERNALPULLUP}, {ENC2_BTNB, true, ENC2_INTERNALPULLUP}};
constexpr uint8_t nrOfButtons = sizeof(button) / sizeof(button[0]);
#endif

#if (ENC_BTNL!=255 && ENC_BTNR!=255) || (ENC2_BTNL!=255 && ENC2_BTNR!=255)
#include <ESP32Encoder.h>
#if (ENC_BTNL!=255 && ENC_BTNR!=255)
ESP32Encoder encoder;
#endif
#if (ENC2_BTNL!=255 && ENC2_BTNR!=255)
ESP32Encoder encoder2;
#endif
#endif

#if TS_CS!=255
#include <XPT2046_Touchscreen.h>
XPT2046_Touchscreen ts(TS_CS);
#endif

#if IR_PIN!=255
#include <assert.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

byte irVolRepeat = 0;
const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = IR_TIMEOUT;
const uint16_t kMinUnknownSize = 12;
const uint8_t kTolerancePercentage = IR_TLP;  //kTolerance;  // kTolerance is normally 25%
#define LEGACY_TIMING_INFO false

IRrecv irrecv(IR_PIN, kCaptureBufferSize, kTimeout, true);
decode_results irResults;
#endif

void initControls() {
#if ENC_BTNL!=255
  encoder.useInternalWeakPullResistors = ENC_INTERNALPULLUP ? UP : DOWN;
  if (ENC_HALFQUARD) {
    encoder.attachHalfQuad(ENC_BTNL, ENC_BTNR);
  } else {
    encoder.attachFullQuad(ENC_BTNL, ENC_BTNR);
  }
#endif
#if ENC2_BTNL!=255
  encoder2.useInternalWeakPullResistors = ENC2_INTERNALPULLUP ? UP : DOWN;
  if (ENC2_HALFQUARD) {
    encoder2.attachHalfQuad(ENC2_BTNL, ENC2_BTNR);
  } else {
    encoder2.attachFullQuad(ENC2_BTNL, ENC2_BTNR);
  }
#endif
#if ISPUSHBUTTONS
  for (int i = 0; i < nrOfButtons; i++)
  {
    if ((i == 0 && BTN_LEFT == 255) || (i == 1 && BTN_CENTER == 255) || (i == 2 && BTN_RIGHT == 255) || (i == 3 && ENC_BTNB == 255) || (i == 4 && BTN_UP == 255) || (i == 5 && BTN_DOWN == 255) || (i == 6 && ENC2_BTNB == 255)) continue;
    button[i].attachClick([](void* p) {
      onBtnClick((int)p);
    }, (void*)i);
    button[i].attachDoubleClick([](void* p) {
      onBtnDoubleClick((int)p);
    }, (void*)i);
    button[i].attachLongPressStart([](void* p) {
      onBtnLongPressStart((int)p);
    }, (void*)i);
    button[i].attachLongPressStop([](void* p) {
      onBtnLongPressStop((int)p);
    }, (void*)i);
    button[i].setClickTicks(BTN_CLICK_TICKS);
    button[i].setPressTicks(BTN_PRESS_TICKS);
  }
#endif
#if TS_CS!=255
  ts.begin();
  ts.setRotation(TS_ROTATE);
#endif
#if IR_PIN!=255
  pinMode(IR_PIN, INPUT);
  assert(irutils::lowLevelSanityCheck() == 0);
#if DECODE_HASH
  irrecv.setUnknownThreshold(kMinUnknownSize);
#endif  // DECODE_HASH
  irrecv.setTolerance(kTolerancePercentage);
  irrecv.enableIRIn();
#endif // IR_PIN!=255
}

void loopControls() {
  if(display.mode==LOST) return;
#if ENC_BTNL!=255
  encoderLoop();
#endif
#if ENC2_BTNL!=255
  encoder2Loop();
#endif
#if ISPUSHBUTTONS
  for (unsigned i = 0; i < nrOfButtons; i++)
  {
    if ((i == 0 && BTN_LEFT == 255) || (i == 1 && BTN_CENTER == 255) || (i == 2 && BTN_RIGHT == 255) || (i == 3 && ENC_BTNB == 255) || (i == 4 && BTN_UP == 255) || (i == 5 && BTN_DOWN == 255) || (i == 6 && ENC2_BTNB == 255)) continue;
    button[i].tick();
    if (lpId >= 0) {
      if (DSP_MODEL == DSP_DUMMY && (lpId == 4 || lpId == 5)) continue;
      onBtnDuringLongPress(lpId);
      yield();
    }
    yield();
  }
#endif
#if IR_PIN!=255
  irLoop();
#endif
#if TS_CS!=255
  touchLoop();
#endif
  yield();
}

#if ENC_BTNL!=255
void encoderLoop() {
  long encNewPosition = encoder.getCount() / 2;
  if (encNewPosition != 0 && encNewPosition != encOldPosition) {
    encOldPosition = encNewPosition;
    encoder.setCount(0);
    controlsEvent(encNewPosition > 0);
  }
}
#endif

#if ENC2_BTNL!=255
void encoder2Loop() {
  long encNewPosition = encoder2.getCount() / 2;
  if (encNewPosition != 0 && encNewPosition != enc2OldPosition) {
    enc2OldPosition = encNewPosition;
    encoder2.setCount(0);
    uint8_t bp = 2;
    if (ENC2_BTNB != 255) {
      bp = digitalRead(ENC2_BTNB);
    }
    if (bp == HIGH && display.mode == PLAYER) {
      display.putRequest({NEWMODE, STATIONS});
      while(display.mode != STATIONS) {delay(5);}
    }
    controlsEvent(encNewPosition > 0);
  }
}
#endif

#if IR_PIN!=255
void irBlink() {
  if (player.mode == STOPPED) {
    for (byte i = 0; i < 7; i++) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
    }
  }
}

void irNum(byte num) {
  uint16_t s;
  if (display.numOfNextStation == 0 && num == 0) return;
  display.putRequest({NEWMODE, NUMBERS});
  if (display.numOfNextStation > UINT16_MAX / 10) return;
  s = display.numOfNextStation * 10 + num;
  if (s > config.store.countStation) return;
  display.numOfNextStation = s;
  display.putRequest({NEXTSTATION, s});
}

void irLoop() {
  if (irrecv.decode(&irResults)) {
    if (IR_DEBUG) {
      Serial.print(resultToHumanReadableBasic(&irResults));
      return;
    }
    if (!irResults.repeat/* && irResults.command!=0*/) {
      irVolRepeat = 0;
    }
    switch (irVolRepeat) {
      case 1: {
          controlsEvent(display.mode == STATIONS ? false : true);
          break;
        }
      case 2: {
          controlsEvent(display.mode == STATIONS ? true : false);
          break;
        }
    }
    switch (irResults.value) {
      case IR_CODE_PLAY: {
          irBlink();
          if (display.mode == NUMBERS) {
            display.putRequest({NEWMODE, PLAYER});
            player.play(display.numOfNextStation);
            display.numOfNextStation = 0;
            break;
          }
          onBtnClick(1);
          break;
        }
      case IR_CODE_PREV: {
          player.prev();
          break;
        }
      case IR_CODE_NEXT: {
          player.next();
          break;
        }
      case IR_CODE_VOLUP: {
          controlsEvent(display.mode == STATIONS ? false : true);
          irVolRepeat = 1;
          break;
        }
      case IR_CODE_VOLDN: {
          controlsEvent(display.mode == STATIONS ? true : false);
          irVolRepeat = 2;
          break;
        }
      case IR_CODE_HASH: {
          if (display.mode == NUMBERS) {
            display.putRequest({RETURNTITLE, 0});
            display.putRequest({NEWMODE, PLAYER});
            display.numOfNextStation = 0;
            break;
          }
          display.putRequest({NEWMODE, display.mode == PLAYER ? STATIONS : PLAYER});
          break;
        }
      case IR_CODE_NUM0: {
          irNum(0);
          break;
        }
      case IR_CODE_NUM1: {
          irNum(1);
          break;
        }
      case IR_CODE_NUM2: {
          irNum(2);
          break;
        }
      case IR_CODE_NUM3: {
          irNum(3);
          break;
        }
      case IR_CODE_NUM4: {
          irNum(4);
          break;
        }
      case IR_CODE_NUM5: {
          irNum(5);
          break;
        }
      case IR_CODE_NUM6: {
          irNum(6);
          break;
        }
      case IR_CODE_NUM7: {
          irNum(7);
          break;
        }
      case IR_CODE_NUM8: {
          irNum(8);
          break;
        }
      case IR_CODE_NUM9: {
          irNum(9);
          break;
        }
    }
  }
}
#endif // if IR_PIN!=255

#if TS_CS!=255
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

boolean wastouched = true;
unsigned long touchdelay;
uint16_t touchVol, touchStation;
uint16_t oldTouchP[2];
tsDirection_e direct;
unsigned long touchLongPress;

tsDirection_e tsDirection(uint16_t x, uint16_t y) {
  int16_t dX = x - oldTouchP[0];
  int16_t dY = y - oldTouchP[1];
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

void touchLoop() {
  if (!checklpdelay(100, touchdelay)) return;
  boolean istouched = ts.touched();
  if (istouched) {
    TS_Point p = ts.getPoint();
    uint16_t touchX = map(p.x, TS_X_MIN, TS_X_MAX, 0, display.screenwidth);
    uint16_t touchY = map(p.y, TS_Y_MIN, TS_Y_MAX, 0, display.screenheight);
    if (!wastouched) { /*     START TOUCH     */
      oldTouchP[0] = touchX;
      oldTouchP[1] = touchY;
      touchVol = touchX;
      touchStation = touchY;
      direct = TDS_REQUEST;
      touchLongPress=millis();
    } else { /*     SWIPE TOUCH     */
      direct = tsDirection(touchX, touchY);
      switch (direct) {
        case TSD_LEFT:
        case TSD_RIGHT: {
            touchLongPress=millis();
            if(display.mode==PLAYER || display.mode==VOL){
              int16_t xDelta = map(abs(touchVol - touchX), 0, display.screenwidth, 0, TS_STEPS);
              display.putRequest({NEWMODE, VOL});
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
            if(display.mode==PLAYER || display.mode==STATIONS){
              int16_t yDelta = map(abs(touchStation - touchY), 0, display.screenheight, 0, TS_STEPS);
              display.putRequest({NEWMODE, STATIONS});
              if (yDelta>1) {
                controlsEvent((touchStation - touchY)>0);
                touchStation = touchY;
              }
            }
            break;
          }
      }
    }
    if (TS_DBG) {
      Serial.print(", x = ");
      Serial.print(p.x);
      Serial.print(", y = ");
      Serial.println(p.y);
    }
  } else {
    if (wastouched) {/*     END TOUCH     */
      if (direct == TDS_REQUEST) {
        if(millis()-touchLongPress < BTN_PRESS_TICKS*2){
          onBtnClick(EVT_BTNCENTER);
        }else{
          display.putRequest({NEWMODE, display.mode == PLAYER ? STATIONS : PLAYER});
        }
      }
      direct = TSD_STAY;
    }
  }
  wastouched = istouched;
}
#endif // if TS_CS!=255

void onBtnLongPressStart(int id) {
  switch ((controlEvt_e)id) {
    case EVT_BTNLEFT:
    case EVT_BTNRIGHT:
    case EVT_BTNUP:
    case EVT_BTNDOWN: {
        lpId = id;
        break;
      }
    case EVT_BTNCENTER:
    case EVT_ENCBTNB: {
        display.putRequest({NEWMODE, display.mode == PLAYER ? STATIONS : PLAYER});
        break;
      }
    case EVT_ENC2BTNB: {
        display.putRequest({NEWMODE, display.mode == PLAYER ? VOL : PLAYER});
        break;
      }
    default:
        break;
  }
}

void onBtnLongPressStop(int id) {
  switch ((controlEvt_e)id) {
    case EVT_BTNLEFT:
    case EVT_BTNRIGHT:
    case EVT_BTNUP:
    case EVT_BTNDOWN: {
        lpId = -1;
        break;
      }
    default:
        break;
  }
}

unsigned long lpdelay;
boolean checklpdelay(int m, unsigned long &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void onBtnDuringLongPress(int id) {
  if (checklpdelay(BTN_LONGPRESS_LOOP_DELAY, lpdelay)) {
    switch ((controlEvt_e)id) {
      case EVT_BTNLEFT: {
          controlsEvent(false);
          break;
        }
      case EVT_BTNRIGHT: {
          controlsEvent(true);
          break;
        }
      case EVT_BTNUP:
      case EVT_BTNDOWN: {
          if (display.mode == PLAYER) {
            display.putRequest({NEWMODE, STATIONS});
          }
          if (display.mode == STATIONS) {
            controlsEvent(id == EVT_BTNDOWN);
          }
          break;
        }
      default:
          break;
    }
  }
}

void controlsEvent(bool toRight) {
  if (display.mode == NUMBERS) {
    display.numOfNextStation = 0;
    display.putRequest({NEWMODE, PLAYER});
  }
  if (display.mode != STATIONS) {
    display.putRequest({NEWMODE, VOL});
    player.stepVol(toRight);
  }
  if (display.mode == STATIONS) {
    display.resetQueue();
    display.putRequest({DRAWPLAYLIST, toRight});
    
  }
}

void onBtnClick(int id) {
  switch ((controlEvt_e)id) {
    case EVT_BTNLEFT: {
        controlsEvent(false);
        break;
      }
    case EVT_BTNCENTER:
    case EVT_ENCBTNB:
    case EVT_ENC2BTNB: {
        if (display.mode == NUMBERS) {
          display.numOfNextStation = 0;
          display.putRequest({NEWMODE, PLAYER});
        }
        if (display.mode == PLAYER) {
          player.toggle();
        }
        if (display.mode == STATIONS) {
          display.putRequest({NEWMODE, PLAYER});
          player.play(display.currentPlItem);
        }
        break;
      }
    case EVT_BTNRIGHT: {
        controlsEvent(true);
        break;
      }
    case EVT_BTNUP:
    case EVT_BTNDOWN: {
        if (DSP_MODEL == DSP_DUMMY) {
          if (id == EVT_BTNUP) {
            player.next();
          } else {
            player.prev();
          }
        } else {
          if (display.mode == PLAYER) {
            display.putRequest({NEWMODE, STATIONS});
          }
          if (display.mode == STATIONS) {
            controlsEvent(id == EVT_BTNDOWN);
          }
        }
        break;
      }
  }
}

void onBtnDoubleClick(int id) {
  switch ((controlEvt_e)id) {
    case EVT_BTNLEFT: {
        if (display.mode != PLAYER) return;
        player.prev();
        break;
      }
    case EVT_BTNCENTER:
    case EVT_ENCBTNB:
    case EVT_ENC2BTNB: {
        display.putRequest({NEWMODE, display.mode == PLAYER ? VOL : PLAYER});
        break;
      }
    case EVT_BTNRIGHT: {
        if (display.mode != PLAYER) return;
        player.next();
        break;
      }
    default:
        break;
  }
}
