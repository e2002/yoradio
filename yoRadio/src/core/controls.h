#ifndef controls_h
#define controls_h
#include "options.h"

#if (ENC_BTNL!=255 && ENC_BTNR!=255) || (ENC2_BTNL!=255 && ENC2_BTNR!=255)
  #include "../yoEncoder/yoEncoder.h"
#endif

enum controlEvt_e { EVT_BTNLEFT, EVT_BTNCENTER, EVT_BTNRIGHT, EVT_ENCBTNB, EVT_BTNUP, EVT_BTNDOWN, EVT_ENC2BTNB, EVT_BTNMODE };

//enum tsDirection_e { TSD_STAY, TSD_LEFT, TSD_RIGHT, TSD_UP, TSD_DOWN, TDS_REQUEST };

#if IR_PIN!=255
enum : uint8_t { IR_UP=0, IR_PREV=1, IR_PLAY=2, IR_NEXT=3, IR_DOWN=4, IR_1=5, IR_2=6, IR_3=7, IR_4=8, IR_5=9, IR_6=10, IR_7=11, IR_8=12, IR_9=13, IR_AST=14, IR_0=15, IR_HASH=16 };
#endif

boolean checklpdelay(int m, unsigned long &tstamp);

void initControls();
void loopControls();
#if (ENC_BTNL!=255 && ENC_BTNR!=255) || (ENC2_BTNL!=255 && ENC2_BTNR!=255)
void encodersLoop(yoEncoder *enc, bool first=true);
#endif
void encoder1Loop();
void encoder2Loop();
void irLoop();
//void touchLoop();
void irNumber(uint8_t num);
void irBlink();
void controlsEvent(bool toRight, int8_t volDelta = 0);

void onBtnClick(int id);
void onBtnDoubleClick(int id);
void onBtnDuringLongPress(int id);
void onBtnLongPressStart(int id);
void onBtnLongPressStop(int id);
//tsDirection_e tsDirection(uint16_t x, uint16_t y);

void setIRTolerance(uint8_t tl);
void setEncAcceleration(uint16_t acc);
void flipTS();

extern __attribute__((weak)) void ctrls_on_loop();

#endif
