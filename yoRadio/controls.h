#ifndef controls_h
#define controls_h

enum controlEvt_e { EVT_BTNLEFT, EVT_BTNCENTER, EVT_BTNRIGHT, EVT_ENCBTNB, EVT_BTNUP, EVT_BTNDOWN, EVT_ENC2BTNB };

boolean checklpdelay(int m, unsigned long &tstamp);

void initControls();
void loopControls();
void encoderLoop();
void encoder2Loop();
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
