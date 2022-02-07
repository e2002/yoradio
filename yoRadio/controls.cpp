#include "controls.h"
#include "options.h"
#include "config.h"
#include "player.h"
#include "display.h"

#include <ESP32Encoder.h>
#include "OneButton.h"

long encOldPosition  = 0;

#if ENC_BTNL!=255
ESP32Encoder encoder;
OneButton encbutton(ENC_BTNB, true);
#endif
#if BTN_LEFT!=255
OneButton btnleft(BTN_LEFT, true);
#endif
#if BTN_CENTER!=255
OneButton btncenter(BTN_CENTER, true);
#endif
#if BTN_RIGHT!=255
OneButton btnright(BTN_RIGHT, true);
#endif

void initControls() {
#if ENC_BTNL!=255
  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachHalfQuad(ENC_BTNL, ENC_BTNR);
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
