#include "Arduino.h"
#include "controls.h"
#include "options.h"
#include "config.h"
#include "player.h"
#include "display.h"
#include "netserver.h"

long encOldPosition  = 0;
long enc2OldPosition  = 0;
int lpId = -1;

#define ISPUSHBUTTONS BTN_LEFT!=255 || BTN_CENTER!=255 || BTN_RIGHT!=255 || ENC_BTNB!=255 || BTN_UP!=255 || BTN_DOWN!=255 || ENC2_BTNB!=255
#if ISPUSHBUTTONS
#include "OneButton.h"
OneButton button[] {{BTN_LEFT, true, BTN_INTERNALPULLUP}, {BTN_CENTER, true, BTN_INTERNALPULLUP}, {BTN_RIGHT, true, BTN_INTERNALPULLUP}, {ENC_BTNB, true, ENC_INTERNALPULLUP}, {BTN_UP, true, BTN_INTERNALPULLUP}, {BTN_DOWN, true, BTN_INTERNALPULLUP}, {ENC2_BTNB, true, ENC2_INTERNALPULLUP}};
constexpr uint8_t nrOfButtons = sizeof(button) / sizeof(button[0]);
#endif

#if ENC_HALFQUARD==false
#define ENCODER_STEPS 4
#elif ENC_HALFQUARD==true
#define ENCODER_STEPS 2
#elif ENC_HALFQUARD==255
#define ENCODER_STEPS 1
#endif
#if ENC2_HALFQUARD==false
#define ENCODER2_STEPS 4
#elif ENC2_HALFQUARD==true
#define ENCODER2_STEPS 2
#elif ENC2_HALFQUARD==255
#define ENCODER2_STEPS 1
#endif

#if (ENC_BTNL!=255 && ENC_BTNR!=255) || (ENC2_BTNL!=255 && ENC2_BTNR!=255)
#include "src/yoEncoder/yoEncoder.h"
#if (ENC_BTNL!=255 && ENC_BTNR!=255)
yoEncoder encoder = yoEncoder(ENC_BTNL, ENC_BTNR, ENCODER_STEPS, ENC_INTERNALPULLUP);
#endif
#if (ENC2_BTNL!=255 && ENC2_BTNR!=255)
yoEncoder encoder2 = yoEncoder(ENC2_BTNL, ENC2_BTNR, ENCODER2_STEPS, ENC2_INTERNALPULLUP);
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

#if ENC_BTNL!=255
void IRAM_ATTR readEncoderISR()
{
  encoder.readEncoder_ISR();
}
#endif
#if ENC2_BTNL!=255
void IRAM_ATTR readEncoder2ISR()
{
  encoder2.readEncoder_ISR();
}
#endif

void initControls() {
  
#if ENC_BTNL!=255
  encoder.begin();
  encoder.setup(readEncoderISR);
  encoder.setBoundaries(0, 254, true);
  encoder.setAcceleration(VOL_ACCELERATION);
#endif
#if ENC2_BTNL!=255
  encoder2.begin();
  encoder2.setup(readEncoder2ISR);
  encoder2.setBoundaries(0, 254, true);
  encoder2.setAcceleration(VOL_ACCELERATION);
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
  if(display.mode==LOST || display.mode==UPDATING) return;
  if (ctrls_on_loop) ctrls_on_loop();
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
  int8_t encoderDelta = encoder.encoderChanged();
  if (encoderDelta!=0)
  {
    controlsEvent(encoderDelta > 0, encoderDelta);
  }
}
#endif

#if ENC2_BTNL!=255
void encoder2Loop() {
  int8_t encoderDelta = encoder2.encoderChanged();
  if (encoderDelta!=0)
  {
    uint8_t bp = 2;
    if (ENC2_BTNB != 255) {
      bp = digitalRead(ENC2_BTNB);
    }
    if (bp == HIGH && display.mode == PLAYER) {
      display.putRequest({NEWMODE, STATIONS});
      while(display.mode != STATIONS) {delay(10);}
    }
    controlsEvent(encoderDelta > 0, encoderDelta);
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
    if(irResults.value<256) return;
    if (netserver.irRecordEnable) {
      Serial.print(resultToHumanReadableBasic(&irResults));
      Serial.println("--------------------------");
      config.ircodes.irVals[config.irindex][config.irchck]=irResults.value;
      netserver.irToWs(typeToString(irResults.decode_type, irResults.repeat).c_str(), irResults.value);
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
    for(int target=0; target<17; target++){
      for(int j=0; j<3; j++){
        if(config.ircodes.irVals[target][j]==irResults.value){
          switch (target){
            case IR_PLAY: {
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
            case IR_PREV: {
                player.prev();
                break;
              }
            case IR_NEXT: {
                player.next();
                break;
              }
            case IR_UP: {
                controlsEvent(display.mode == STATIONS ? false : true);
                irVolRepeat = 1;
                break;
              }
            case IR_DOWN: {
                controlsEvent(display.mode == STATIONS ? true : false);
                irVolRepeat = 2;
                break;
              }
            case IR_HASH: {
                if (display.mode == NUMBERS) {
                  display.putRequest({RETURNTITLE, 0});
                  display.putRequest({NEWMODE, PLAYER});
                  display.numOfNextStation = 0;
                  break;
                }
                display.putRequest({NEWMODE, display.mode == PLAYER ? STATIONS : PLAYER});
                break;
              }
            case IR_0: {
                irNum(0);
                break;
              }
            case IR_1: {
                irNum(1);
                break;
              }
            case IR_2: {
                irNum(2);
                break;
              }
            case IR_3: {
                irNum(3);
                break;
              }
            case IR_4: {
                irNum(4);
                break;
              }
            case IR_5: {
                irNum(5);
                break;
              }
            case IR_6: {
                irNum(6);
                break;
              }
            case IR_7: {
                irNum(7);
                break;
              }
            case IR_8: {
                irNum(8);
                break;
              }
            case IR_9: {
                irNum(9);
                break;
              }
            case IR_AST: {
                break;
              }
          } /* switch (target) */
          target=17;
          break;
        } /* if(config.ircodes.irVals[target][j]==irResults.value) */
      }   /* for(int j=0; j<3; j++) */
    }     /* for(int target=0; target<16; target++) */
  }       /* if (irrecv.decode(&irResults)) */
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
        default:
            break;
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

void controlsEvent(bool toRight, int8_t volDelta) {
  if (display.mode == NUMBERS) {
    display.numOfNextStation = 0;
    display.putRequest({NEWMODE, PLAYER});
  }
  if (display.mode != STATIONS) {
    display.putRequest({NEWMODE, VOL});
    if(volDelta!=0){
      int nv = config.store.volume+volDelta;
      if(nv<0) nv=0;
      if(nv>254) nv=254;
      player.setVol((byte)nv, false);
    }else{
      player.stepVol(toRight);
    }
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
