#include "controls.h"
#include "options.h"
#include "config.h"
#include "player.h"
#include "display.h"

long encOldPosition  = 0;

#if BTN_LEFT!=255 || BTN_LEFT!=255 || BTN_RIGHT!=255 || ENC_BTNL!=255
#include "OneButton.h"
#endif

#if ENC_BTNL!=255
#include <ESP32Encoder.h>
ESP32Encoder encoder;
OneButton encbutton(ENC_BTNB, true);
#endif

#if BTN_LEFT!=255
OneButton btnleft(BTN_LEFT, true, BTN_INTERNALPULLUP);
#endif
#if BTN_CENTER!=255
OneButton btncenter(BTN_CENTER, true, BTN_INTERNALPULLUP);
#endif
#if BTN_RIGHT!=255
OneButton btnright(BTN_RIGHT, true, BTN_INTERNALPULLUP);
#endif

#if IR_PIN!=255
#include <assert.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

byte irVolRepeat=0;
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
  encbutton.attachClick(onEncClick);
  encbutton.attachDoubleClick(onEncDoubleClick);
  encbutton.attachLongPressStart(onEncLPStart);
#endif
#if BTN_LEFT!=255
  btnleft.attachClick(onLeftClick);
  btnleft.attachDoubleClick(onLeftDoubleClick);
#endif
#if BTN_CENTER!=255
  btncenter.attachClick(onEncClick);
  btncenter.attachDoubleClick(onEncDoubleClick);
  btncenter.attachLongPressStart(onEncLPStart);
#endif
#if BTN_RIGHT!=255
  btnright.attachClick(onRightClick);
  btnright.attachDoubleClick(onRightDoubleClick);
#endif
#if IR_PIN!=255
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
  encbutton.tick();
  encoderLoop();
#endif
#if BTN_LEFT!=255
  btnleft.tick();
#endif
#if BTN_CENTER!=255
  btncenter.tick();
#endif
#if BTN_RIGHT!=255
  btnright.tick();
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
void irLoop() {
  if (irrecv.decode(&irResults)) {
    if(IR_DEBUG) {
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
          onEncClick();
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
          display.swichMode(display.mode == PLAYER ? STATIONS : PLAYER);
          break;
        }
    }
  }
}
#endif

void onEncClick() {
  if (display.mode == PLAYER) {
    player.toggle();
  }
  if (display.mode == STATIONS) {
    display.swichMode(PLAYER);
    player.play(display.currentPlItem);
  }
}


void onEncDoubleClick() {
  display.swichMode(display.mode == PLAYER ? STATIONS : PLAYER);
}

void onEncLPStart() {
  display.swichMode(display.mode == PLAYER ? STATIONS : PLAYER);
}

void controlsEvent(bool toRight) {
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

void onLeftClick() {
  controlsEvent(false);
}

void onLeftDoubleClick() {
  display.swichMode(PLAYER);
  player.prev();
}

void onRightClick() {
  controlsEvent(true);
}

void onRightDoubleClick() {
  display.swichMode(PLAYER);
  player.next();
}
