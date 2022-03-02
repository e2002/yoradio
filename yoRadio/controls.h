#ifndef controls_h
#define controls_h


void initControls();
void loopControls();
void onEncClick();
void onEncDoubleClick();
void onEncLPStart();
void encoderLoop();
void irLoop();
void irNum(byte num);
void irBlink();
void controlsEvent(bool toRight);

void onLeftClick();
void onLeftDoubleClick();
void onRightClick();
void onRightDoubleClick();

#endif
