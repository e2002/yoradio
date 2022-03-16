#include "Arduino.h"
#include "controls.h"
#include "options.h"
#include "config.h"
#include "player.h"
#include "display.h"

long encOldPosition  = 0;
int lpId = -1;

#define ISPUSHBUTTONS BTN_LEFT!=255 || BTN_LEFT!=255 || BTN_RIGHT!=255 || ENC_BTNB!=255
#if ISPUSHBUTTONS
#include "OneButton.h"
OneButton button[] {{BTN_LEFT, true, BTN_INTERNALPULLUP}, {BTN_CENTER, true, BTN_INTERNALPULLUP}, {BTN_RIGHT, true, BTN_INTERNALPULLUP}, {ENC_BTNB, true, ENC_INTERNALPULLUP}};
constexpr uint8_t nrOfButtons = sizeof(button) / sizeof(button[0]);
#endif

#if ENC_BTNL!=255 && ENC_BTNR!=255
#include <ESP32Encoder.h>
ESP32Encoder encoder;
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
  ESP32Encoder::useInternalWeakPullResistors = ENC_INTERNALPULLUP ? UP : DOWN;
  if (ENC_HALFQUARD) {
    encoder.attachHalfQuad(ENC_BTNL, ENC_BTNR);
  } else {
    encoder.attachFullQuad(ENC_BTNL, ENC_BTNR);
  }
#endif
#if ISPUSHBUTTONS
  for (int i = 0; i < nrOfButtons; i++)
  {
    if ((i == 0 && BTN_LEFT == 255) || (i == 1 && BTN_CENTER == 255) || (i == 2 && BTN_RIGHT == 255) || (i == 3 && ENC_BTNB == 255)) continue;
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
#if ENC_BTNL!=255
  encoderLoop();
#endif
#if ISPUSHBUTTONS
  for (unsigned i = 0; i < nrOfButtons; i++)
  {
    if ((i == 0 && BTN_LEFT == 255) || (i == 1 && BTN_CENTER == 255) || (i == 2 && BTN_RIGHT == 255) || (i == 3 && ENC_BTNB == 255)) continue;
    button[i].tick();
    if (lpId >= 0) {
      onBtnDuringLongPress(lpId);
    }
  }
#endif
#if IR_PIN!=255
  irLoop();
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
  if (display.mode == PLAYER) display.swichMode(NUMBERS);
  if (display.numOfNextStation > UINT16_MAX / 10) return;
  s = display.numOfNextStation * 10 + num;
  if (s > config.store.countStation) return;
  display.numOfNextStation = s;
  display.drawNextStationNum(s);
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
            display.swichMode(PLAYER);
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
            display.swichMode(PLAYER);
            display.numOfNextStation = 0;
            break;
          }
          display.swichMode(display.mode == PLAYER ? STATIONS : PLAYER);
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

void onBtnLongPressStart(int id) {
  switch (id) {
    case 0:
    case 2: {
        lpId = id;
        break;
      }
    case 1:
    case 3: {
        display.swichMode(display.mode == PLAYER ? STATIONS : PLAYER);
        break;
      }
  }
}

void onBtnLongPressStop(int id) {
  switch (id) {
    case 0:
    case 2: {
        lpId = -1;
        break;
      }
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
    switch (id) {
      case 0: {
          controlsEvent(false);
          break;
        }
      case 2: {
          controlsEvent(true);
          break;
        }
    }
  }
}

void controlsEvent(bool toRight) {
  if (display.mode == NUMBERS) {
    display.numOfNextStation = 0;
    display.swichMode(PLAYER);
  }
  if (display.mode != STATIONS) {
    display.swichMode(VOL);
    player.stepVol(toRight);
  }
  if (display.mode == STATIONS) {
    int p = toRight ? display.currentPlItem + 1 : display.currentPlItem - 1;
    if (p < 1) p = 1;
    if (p > config.store.countStation) p = config.store.countStation;
    display.currentPlItem = p;
    display.clear();
    display.drawPlaylist();
  }
}

void onBtnClick(int id) {
  switch (id) {
    case 0: {
        controlsEvent(false);
        break;
      }
    case 1:
    case 3: {
        if (display.mode == NUMBERS) {
          display.numOfNextStation = 0;
          display.swichMode(PLAYER);
        }
        if (display.mode == PLAYER) {
          player.toggle();
        }
        if (display.mode == STATIONS) {
          display.swichMode(PLAYER);
          player.play(display.currentPlItem);
        }
        break;
      }
    case 2: {
        controlsEvent(true);
        break;
      }
  }
}

void onBtnDoubleClick(int id) {
  switch (id) {
    case 0: {
        if (display.mode != PLAYER) return;
        player.prev();
        break;
      }
    case 1:
    case 3: {
        display.swichMode(display.mode == PLAYER ? STATIONS : PLAYER);
        break;
      }
    case 2: {
        if (display.mode != PLAYER) return;
        player.next();
        break;
      }
  }
}
