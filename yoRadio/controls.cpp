#include "controls.h"
#include "options.h"
#include "config.h"
#include "player.h"
#include "display.h"

#include <ESP32Encoder.h>
#include "OneButton.h"

long encOldPosition  = 0;

ESP32Encoder encoder;
OneButton encbutton(ENC_BTNB, true);

OneButton btnleft(BTN_LEFT, true);
OneButton btncenter(BTN_CENTER, true);
OneButton btnright(BTN_RIGHT, true);

void initControls() {
  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachHalfQuad(ENC_BTNL, ENC_BTNR);
  encbutton.attachClick(onEncClick);
  encbutton.attachDoubleClick(onEncDoubleClick);
  encbutton.attachLongPressStart(onEncLPStart);

  btnleft.attachClick(onLeftClick);
  btnleft.attachDoubleClick(onLeftDoubleClick);

  btncenter.attachClick(onEncClick);
  btncenter.attachDoubleClick(onEncDoubleClick);
  btncenter.attachLongPressStart(onEncLPStart);

  btnright.attachClick(onRightClick);
  btnright.attachDoubleClick(onRightDoubleClick);
}

void loopControls() {
  encbutton.tick();
  encoderLoop();
  yield();
}

void encoderLoop() {
  long encNewPosition = encoder.getCount() / 2;
  if (encNewPosition != 0 && encNewPosition != encOldPosition) {
    encOldPosition = encNewPosition;
    encoder.setCount(0);
    controlsEvent(encNewPosition > 0);
  }
}

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
