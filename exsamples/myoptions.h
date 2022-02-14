#ifndef myoptions_h
#define myoptions_h

/*
 * HWID's:
 * 0: with yellow-blue SSD1306
 * 1: white SSD1306 without controls
 * 2: ST7735 with encoder
 * 3: Nokia 5110 dev board
 * 4: VS1053 dev
 * 5: VS1053 UNO3 Shield
 */
#define HWID          2

/******************************************/

#if HWID==0
#define DSP_MODEL     2
#define I2S_DOUT      22
#define BTN_LEFT      16
#define BTN_CENTER    5
#define BTN_RIGHT     17

#elif HWID==1
#define DSP_MODEL     2
#define I2S_DOUT      22

#elif HWID==2
#define DSP_MODEL     1
#define ENC_BTNL      13
#define ENC_BTNB      12
#define ENC_BTNR      14

#elif HWID==3
#define DSP_MODEL     3
#define BTN_LEFT      13
#define BTN_CENTER    12
#define BTN_RIGHT     14

#elif HWID==4
#define DSP_MODEL     3
#define VS1053_CS     27
#define I2S_DOUT      255
//#define VS1053_RST    14

#elif HWID==5
#define DSP_MODEL     3
#define TFT_RST       -1	// connecting to esp reset pin
#define I2S_DOUT      255
#define VS1053_CS     27
#define VS1053_DCS    14
#define VS1053_DREQ   26
#define VS1053_RST    12

#endif

/******************************************/

#endif
