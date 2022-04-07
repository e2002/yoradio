#ifndef myoptions_h
#define myoptions_h

/*******************************************************

Copy this file in the project root directory (next to the yoRadio.ino file)
Uncomment the lines you need, to override the default value and set the values according to the connected equipment.

The connection tables are located here https://github.com/e2002/yoradio#connection-tables

********************************************************/

/*  DSP_MODEL. See description/available values in the options.h file  */
/*  This option is required. Use DSP_DUMMY if no display is connected */
#define DSP_MODEL         DSP_DUMMY
/******************************************/

/*  SPI PINS. SCL(SCK, CLK) must be connected to pin 18
              SDA(MOSI, DIN, SDI) must be connected to pin 23  */
//#define TFT_CS            5                 /*  SPI CS pin  */
//#define TFT_RST           15                /*  SPI RST pin.  set to -1 and connect to Esp EN pin */
//#define TFT_DC            4                 /*  SPI DC pin  */
/******************************************/

/*  I2C PINS  */
//#define I2C_SDA           21                /*  I2C SDA pin. It is best to connect to pin 21.  */
//#define I2C_SCL           22                /*  I2C SCL pin. It is best to connect to pin 22.  */
//#define I2C_RST           -1                /*  I2C RST pin. Set to -1 if not used  */

/*        I2S DAC                 */
//#define I2S_DOUT          27                /*  DIN connection. Should be set to 255 if the board is not used */
//#define I2S_BCLK          26                /*  BCLK Bit clock */
//#define I2S_LRC           25                /*  WSEL Left Right Clock */
/******************************************/

/*  VS1053 PINS. VS1053 SCK must be connected to pin 18
                 VS1053 MOSI must be connected to pin 23  */
//#define VS1053_CS         255               /*  Should be set to 255 if the board is not used */
//#define VS1053_DCS        25
//#define VS1053_DREQ       26
//#define VS1053_RST        -1                /*  Set to -1 if connected to Esp EN pin */
/******************************************/

/*  ENCODER  */
//#define ENC_BTNL              255           /*  Left rotation */
//#define ENC_BTNB              255           /*  Encoder button */
//#define ENC_BTNR              255           /*  Right rotation */
//#define ENC_INTERNALPULLUP    true          /*  Enable the weak pull up resistors */
//#define ENC_HALFQUARD         true          /*  Experiment  with it */
/******************************************/

/*  ENCODER2  */
//#define ENC2_BTNL              255           /*  Left rotation */
//#define ENC2_BTNB              255           /*  Encoder button */
//#define ENC2_BTNR              255           /*  Right rotation */
//#define ENC2_INTERNALPULLUP    true          /*  Enable the weak pull up resistors */
//#define ENC2_HALFQUARD         true          /*  Experiment  with it */
/******************************************/

/*  BUTTONS  */
//#define BTN_LEFT              255           /*  VolDown, Prev */
//#define BTN_CENTER            255           /*  Play, Stop, Show playlist */
//#define BTN_RIGHT             255           /*  VolUp, Next */
//#define BTN_INTERNALPULLUP    true          /*  Enable the weak pull up resistors */
//#define BTN_LONGPRESS_LOOP_DELAY    200     /*  Delay between calling DuringLongPress event */
//#define BTN_CLICK_TICKS    300              /*  Event Timing https://github.com/mathertel/OneButton#event-timing */
//#define BTN_PRESS_TICKS    500              /*  Event Timing https://github.com/mathertel/OneButton#event-timing */
/******************************************/

/*  LCD DISPLAY 1602  */
//#define LCD_RS                255           /*  RS Pin */
//#define LCD_E                 255           /*  E Pin  */
//#define LCD_D4                255           /*  D4 Pin */
//#define LCD_D5                255           /*  D5 Pin */
//#define LCD_D6                255           /*  D6 Pin */
//#define LCD_D7                255           /*  D7 Pin */
/******************************************/

/*  TOUCHSCREEN  */
/*  SPI PINS. CLK must be connected to pin 18
              DIN must be connected to pin 23
              DO  must be connected to pin 19
              IRQ - not connected */
//#define TS_CS                 255           /*  Touch screen CS pin
//#define TS_ROTATE             1             /*  Touch screen rotation. 0 - 0, 1 - 90, 2 - 180, 3 - 270 degrees */
//#define TS_DBG                false         /*  Generate debug to Serial output */
/******************************************/

/*  Other settings.  */
//#define DTYPE             INITR_BLACKTAB    /*  ST7735 display submodel */
                                            /*  Could be one of: */
                                            /*  INITR_BLACKTAB        1.8' https://aliexpress.com/item/1005002822797745.html  */
                                            /*  (See this note If INITR_BLACKTAB have a noisy line on one side of the screen https://github.com/e2002/yoradio#note-if-initr_blacktab-dsp-have-a-noisy-line-on-one-side-of-the-screen-then-in-adafruit_st7735cpp ) */
                                            /*  INITR_144GREENTAB     // 1.44' https://aliexpress.com/item/1005002822797745.html  */
                                            /*  INITR_MINI160x80      // 0.96' 160x80 ST7735S   https://????  */
                                            /*  INITR_GREENTAB  */
                                            /*  INITR_REDTAB  */
//#define LED_BUILTIN       2                 /*  LED Pin */
//#define TFT_ROTATE        3                 /*  Display rotation. 0 - 0, 1 - 90, 2 - 180, 3 - 270 degrees */
//#define TFT_CONTRAST      55                /*  Nokia 5110 contrast */
//#define TFT_INVERT        true              /*  Invert the display colors (usually true) */
//#define VOL_STEP          1                 /*  Volume control step */
//#define MUTE_PIN          255               /*  MUTE Pin */
//#define MUTE_VAL          HIGH              /*  Write this to MUTE_PIN when player is stopped */
/******************************************/

/*  IR control  */
//#define IR_PIN                255
//#define IR_DEBUG              0               /*  Set this to 1, capture ir codes from serial, insert the captured codes below and set this back to 0 */
//#define IR_TIMEOUT            80              /*  see kTimeout description in IRremoteESP8266 exsample https://github.com/crankyoldgit/IRremoteESP8266/blob/master/examples/IRrecvDumpV2/IRrecvDumpV2.ino */
//#define IR_TLP                40              /*  see kTolerancePercentage description in IRremoteESP8266 exsample https://github.com/crankyoldgit/IRremoteESP8266/blob/master/examples/IRrecvDumpV2/IRrecvDumpV2.ino */
//#define IR_CODE_PLAY          0xFF02FD
//#define IR_CODE_PREV          0xFF22DD
//#define IR_CODE_NEXT          0xFFC23DP
//#define IR_CODE_VOLUP         0xFF629D
//#define IR_CODE_VOLDN         0xFFA857
//#define IR_CODE_NUM0          0xFF4AB5
//#define IR_CODE_NUM1          0xFF6897
//#define IR_CODE_NUM2          0xFF9867
//#define IR_CODE_NUM3          0xFFB04F
//#define IR_CODE_NUM4          0xFF30CF
//#define IR_CODE_NUM5          0xFF18E7
//#define IR_CODE_NUM6          0xFF7A85
//#define IR_CODE_NUM7          0xFF10EF
//#define IR_CODE_NUM8          0xFF38C7
//#define IR_CODE_NUM9          0xFF5AA5
//#define IR_CODE_HASH          0xFF52AD         /*  Toggle playlist mode */
//#define IR_CODE_AST           0xFF42BD         /*  Not used */
#endif
