#ifndef options_h
#define options_h

#define VERSION "0.4.315"

#if __has_include("myoptions.h")
#include "myoptions.h" // <- write your variable values here 
#endif

/* DISPLAY MODEL
 * 0 - DUMMY
 * 1 - ST7735
 * 2 - SSD1306
 * 3 - NOKIA5110
 */
#ifndef DSP_MODEL
  #define DSP_MODEL  1
#endif

/*
 * The connection tables are located here https://github.com/e2002/yoradio#connection-tables
 */

/*        TFT DISPLAY             */
#ifndef TFT_CS
  #define TFT_CS        5
#endif
#ifndef TFT_RST
  #define TFT_RST       15   // Or set to -1 and connect to Esp EN pin
#endif
#ifndef TFT_DC
  #define TFT_DC        4
#endif

/*        OLED I2C DISPLAY        */
#ifndef I2C_SDA
  #define I2C_SDA 13
#endif
#ifndef I2C_SCL
  #define I2C_SCL 14
#endif
#ifndef I2C_RST
  #define I2C_RST -1
#endif

/*        VS1053                  */
#ifndef VS1053_CS
  #define VS1053_CS     255 // 27
#endif
#ifndef VS1053_DCS
  #define VS1053_DCS    25
#endif
#ifndef VS1053_DREQ
  #define VS1053_DREQ   26
#endif
#ifndef VS1053_RST
  #define VS1053_RST    -1    // set to -1 if connected to Esp EN pin
#endif

/*        I2S DAC                 */
#ifndef I2S_DOUT
  #define I2S_DOUT      27  // DIN connection
#endif
#ifndef I2S_BCLK
  #define I2S_BCLK      26  // BCLK Bit clock
#endif
#ifndef I2S_LRC
  #define I2S_LRC       25  // WSEL Left Right Clock
#endif

/*        ENCODER                 */
#ifndef ENC_BTNL
  #define ENC_BTNL              255
#endif
#ifndef ENC_BTNB
  #define ENC_BTNB              255
#endif
#ifndef ENC_BTNR
  #define ENC_BTNR              255
#endif
#ifndef ENC_INTERNALPULLUP                // Thanks for Buska1968. See this topic: https://4pda.to/forum/index.php?s=&showtopic=1010378&view=findpost&p=113385448
  #define ENC_INTERNALPULLUP    true
#endif
#ifndef ENC_HALFQUARD
  #define ENC_HALFQUARD         true
#endif

/*        BUTTONS                 */
#ifndef BTN_LEFT
  #define BTN_LEFT              255
#endif
#ifndef BTN_CENTER
  #define BTN_CENTER            255
#endif
#ifndef BTN_RIGHT
  #define BTN_RIGHT             255
#endif
#ifndef BTN_INTERNALPULLUP
  #define BTN_INTERNALPULLUP    true
#endif

/*        ESP DEVBOARD            */
#ifndef LED_BUILTIN
  #define LED_BUILTIN   2
#endif

/*        Other settings. You can overwrite them in the myoptions.h file        */
#ifndef TFT_ROTATE
  #define TFT_ROTATE    3   // display rotation. 0 - 0, 1 - 90, 2 - 180, 3 - 270 degrees
#endif
#ifndef TFT_CONTRAST
  #define TFT_CONTRAST  55  // Nokia 5110 contrast
#endif
#ifndef VOL_STEP
  #define VOL_STEP      1   // Encoder vol step
#endif

#ifndef MUTE_PIN
  #define MUTE_PIN      255   // MUTE Pin
#endif
#ifndef MUTE_VAL
  #define MUTE_VAL      HIGH  // Write this to MUTE_PIN when player is stopped
#endif

/*
*** ST7735 display submodel ***
INITR_BLACKTAB        // 1.8' https://aliexpress.ru/item/1005002822797745.html
    See this note If INITR_BLACKTAB have a noisy line on one side of the screen https://github.com/e2002/yoradio#note-if-initr_blacktab-dsp-have-a-noisy-line-on-one-side-of-the-screen-then-in-adafruit_st7735cpp
INITR_144GREENTAB     // 1.44' https://aliexpress.ru/item/1005002822797745.html
INITR_GREENTAB
INITR_REDTAB
 */
#ifndef DTYPE
  #define DTYPE INITR_BLACKTAB
#endif

/*        IR                      */
#ifndef IR_PIN
  #define IR_PIN                255
#endif
#ifndef IR_DEBUG
  #define IR_DEBUG              0         // 1 - for capture ir codes from serial
#endif
#ifndef IR_TIMEOUT
  #define IR_TIMEOUT            80        // kTimeout, see IRremoteESP8266 documentation
#endif
#ifndef IR_TLP
  #define IR_TLP                40        // kTolerancePercentage, see IRremoteESP8266 documentation
#endif
#ifndef IR_CODE_PLAY
  #define IR_CODE_PLAY          0xFF02FD
#endif
#ifndef IR_CODE_PREV
  #define IR_CODE_PREV          0xFF22DD
#endif
#ifndef IR_CODE_NEXT
  #define IR_CODE_NEXT          0xFFC23D
#endif
#ifndef IR_CODE_VOLUP
  #define IR_CODE_VOLUP         0xFF629D
#endif
#ifndef IR_CODE_VOLDN
  #define IR_CODE_VOLDN         0xFFA857
#endif
#ifndef IR_CODE_NUM0
  #define IR_CODE_NUM0          0xFF4AB5
#endif
#ifndef IR_CODE_NUM1
  #define IR_CODE_NUM1          0xFF6897
#endif
#ifndef IR_CODE_NUM2
  #define IR_CODE_NUM2          0xFF9867
#endif
#ifndef IR_CODE_NUM3
  #define IR_CODE_NUM3          0xFFB04F
#endif
#ifndef IR_CODE_NUM4
  #define IR_CODE_NUM4          0xFF30CF
#endif
#ifndef IR_CODE_NUM5
  #define IR_CODE_NUM5          0xFF18E7
#endif
#ifndef IR_CODE_NUM6
  #define IR_CODE_NUM6          0xFF7A85
#endif
#ifndef IR_CODE_NUM7
  #define IR_CODE_NUM7          0xFF10EF
#endif
#ifndef IR_CODE_NUM8
  #define IR_CODE_NUM8          0xFF38C7
#endif
#ifndef IR_CODE_NUM9
  #define IR_CODE_NUM9          0xFF5AA5
#endif
#ifndef IR_CODE_HASH
  #define IR_CODE_HASH          0xFF52AD    // Toggle playlist mode
#endif
#ifndef IR_CODE_AST
  #define IR_CODE_AST           0xFF42BD
#endif


#endif
