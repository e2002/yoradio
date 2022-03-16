#ifndef controls_h
#define controls_h

boolean checklpdelay(int m, unsigned long &tstamp);

void initControls();
void loopControls();
void encoderLoop();
void irLoop();
void irNum(byte num);
void irBlink();
void controlsEvent(bool toRight);

void onBtnClick(int id);
void onBtnDoubleClick(int id);
void onBtnDuringLongPress(int id);
void onBtnLongPressStart(int id);
void onBtnLongPressStop(int id);

#endif
