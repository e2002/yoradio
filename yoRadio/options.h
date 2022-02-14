#ifndef options_h
#define options_h

#define VERSION "0.4.251"

/* DISPLAY MODEL
 * 0 - DUMMY
 * 1 - ST7735
 * 2 - SSD1306
 * 3 - NOKIA5110
 */
#define DSP_MODEL  1

/*
 * TFT DISPLAY
 */
/**************
 * GND  | GND *
 * VCC  | +5v *
 * SCL  | D18 *
 * SDA  | D23 *
 * ************
 */
#define TFT_CS        5
#define TFT_RST       15   // Or set to -1 and connect to Esp EN pin
//#define TFT_RST    -1    // we use the seesaw for resetting to save a pin
#define TFT_DC        4
/*
 * OLED I2C DISPLAY
 */
#define I2C_SDA 13
#define I2C_SCL 14
#define I2C_RST -1
/*
 * VS1053
 */
#define VS1053_CS     255 // 27
#define VS1053_DCS    25
#define VS1053_DREQ   26
#define VS1053_RST    -1    // set to -1 if connected to Esp EN pin
/*
 * I2S DAC
 */
#define I2S_DOUT      27  // DIN connection
#define I2S_BCLK      26  // BCLK Bit clock
#define I2S_LRC       25  // WSEL Left Right Clock
/*
 * ENCODER
 */
#define ENC_BTNL      255
#define ENC_BTNB      255
#define ENC_BTNR      255
/*
 * BUTTONS
 */
#define BTN_LEFT      255
#define BTN_CENTER    255
#define BTN_RIGHT     255
/*
 * ESP DEVBOARD
 */
#define LED_BUILTIN 2

#if __has_include("myoptions.h")
#include "myoptions.h"
#endif

#endif
