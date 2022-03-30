#ifndef controls_h
#define controls_h

enum controlEvt_e { EVT_BTNLEFT, EVT_BTNCENTER, EVT_BTNRIGHT, EVT_ENCBTNB, EVT_BTNUP, EVT_BTNDOWN, EVT_ENC2BTNB };

enum tsDirection_e { TSD_STAY, TSD_LEFT, TSD_RIGHT, TSD_UP, TSD_DOWN, TDS_REQUEST };


boolean checklpdelay(int m, unsigned long &tstamp);

void initControls();
void loopControls();
void encoderLoop();
void encoder2Loop();
void irLoop();
void touchLoop();
void irNum(byte num);
void irBlink();
void controlsEvent(bool toRight);

void onBtnClick(int id);
void onBtnDoubleClick(int id);
void onBtnDuringLongPress(int id);
void onBtnLongPressStart(int id);
void onBtnLongPressStop(int id);
tsDirection_e tsDirection(uint16_t x, uint16_t y);

#endif
