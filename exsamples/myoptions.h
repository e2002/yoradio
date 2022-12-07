#ifndef myoptions_h
#define myoptions_h

/*******************************************************

Copy this file in the project root directory (next to the yoRadio.ino file)
Uncomment the lines you need, to override the default value and set the values according to the connected equipment.

The connection tables are located here https://github.com/e2002/yoradio#connection-tables

********************************************************/
//#define LED_BUILTIN     255               /*  Onboard LED Pin (turn OFF by default) */
//#define LED_INVERT      false             /*  Invert Onboard LED? */
#define L10N_LANGUAGE     EN                /*  Language (EN, RU). More info in yoRadio/locale/displayL10n_(en|ru).h  */

/*  DSP_MODEL. See description/available values in https://github.com/e2002/yoradio/wiki/Available-display-models  */
/*  This option is required. Use DSP_DUMMY if no display is connected */
#define DSP_MODEL         DSP_DUMMY
/*
 * !!! Important !!!
 * if you use colored TFT displays with the esp32 wroom, due to lack of memory, you must modify the file Arduino/libraries/AsyncTCP/src/AsyncTCP.cpp
 * replace the line 221
 * xTaskCreateUniversal(_async_service_task, "async_tcp", 8192 * 2, NULL, 3, &_async_service_task_handle, CONFIG_ASYNC_TCP_RUNNING_CORE);
 * with
 * xTaskCreateUniversal(_async_service_task, "async_tcp", 8192 / 2, NULL, 3, &_async_service_task_handle, CONFIG_ASYNC_TCP_RUNNING_CORE);
*/
/******************************************/

/*  VSPI PINS. SCL(SCK, CLK) must be connected to pin 18
               SDA(MOSI, DIN, SDI) must be connected to pin 23  */
//#define TFT_CS            5                 /*  SPI CS pin  */
//#define TFT_RST           15                /*  SPI RST pin.  set to -1 and connect to Esp EN pin */
//#define TFT_DC            4                 /*  SPI DC/RS pin  */
/*  HSPI PINS. SCL(SCK, CLK) must be connected to pin 14
               SDA(MOSI, DIN, SDI) must be connected to pin 13  */
//#define DSP_HSPI          false             /*  Use HSPI for display  */
/******************************************/

/*  NEXTION  */
//#define NEXTION_RX    255                   /*  Nextion RX pin  */
//#define NEXTION_TX    255                   /*  Nextion TX pin  */
  
/*  I2C PINS  */
//#define I2C_SDA           21                /*  I2C SDA pin. It is best to connect to pin 21.  */
//#define I2C_SCL           22                /*  I2C SCL pin. It is best to connect to pin 22.  */
//#define I2C_RST           -1                /*  I2C RST pin. Set to -1 if not used  */

/*        I2S DAC                 */
//#define I2S_DOUT          27                /*  DIN connection. Should be set to 255 if the board is not used */
//#define I2S_BCLK          26                /*  BCLK Bit clock */
//#define I2S_LRC           25                /*  WSEL Left Right Clock */
/******************************************/

/*  VS1053 VSPI PINS. VS1053 SCK must be connected to pin 18
                      VS1053 MISO must be connected to pin 19
                      VS1053 MOSI must be connected to pin 23  */
//#define VS1053_CS         255               /*  XCS pin. Should be set to 255 if the board is not used */
//#define VS1053_DCS        25                /*  XDCS pin.  */
//#define VS1053_DREQ       26                /*  DREQ pin.  */
//#define VS1053_RST        -1                /*  XRESET pin. Set to -1 if connected to Esp EN pin */
/*  VS1053 HSPI PINS. VS1053 SCK must be connected to pin 14
                      VS1053 MISO must be connected to pin 12
                      VS1053 MOSI must be connected to pin 13  */
//#define VS_HSPI           false             /*  Use HSPI for VS  */
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
//#define ENC2_HALFQUARD         false         /*  (true, false, 255) Experiment  with it */
/******************************************/

/*  BUTTONS  */
//#define BTN_LEFT              255           /*  VolDown, Prev */
//#define BTN_CENTER            255           /*  Play, Stop, Show playlist */
//#define BTN_RIGHT             255           /*  VolUp, Next */
//#define BTN_UP                255           /*  Prev, Move Up */
//#define BTN_DOWN              255           /*  Next, Move Down */
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
//#define TS_MODEL              TS_MODEL_UNDEFINED  /*  See description/available values in yoRadio/src/core/options.h  */

/*  Resistive SPI touch screen  */
/*  TS VSPI PINS. CLK must be connected to pin 18
                  DIN must be connected to pin 23
                  DO  must be connected to pin 19
                  IRQ - not connected */
//#define TS_CS                 255           /*  Touch screen CS pin  */
/*  TS HSPI PINS. CLK must be connected to pin 14
                  DIN must be connected to pin 13
                  DO  must be connected to pin 12
                  IRQ - not connected */
//#define TS_HSPI               false         /*  Use HSPI for Touch screen  */

/*  Capacitive I2C touch screen  */
//#define TS_SDA                33
//#define TS_SCL                32
//#define TS_INT                21
//#define TS_RST                25
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
//#define MUTE_PIN          255               /*  MUTE Pin */
//#define MUTE_VAL          HIGH              /*  Write this to MUTE_PIN when player is stopped */
//#define BRIGHTNESS_PIN    255               /*  Pin for adjusting the brightness of the display (output 0 - 3v3) */
//#define PLAYER_FORCE_MONO false             /*  mono option on boot - false stereo, true mono  */
//#define I2S_INTERNAL      false             /*  If true - use esp32 internal DAC  */
//#define ROTATE_90         false             /*  Optional 90 degree rotation for square displays */
//#define WAKE_PIN          255               /*  Wake Pin (for manual wakeup from sleep mode. can match with BTN_XXXX, ENC_BTNB, ENC2_BTNB.  must be one of: 0,2,4,12,13,14,15,25,26,27,32,33,34,35,36,39) */
                                              /*  For sample #define ENC_BTNB 36 - next line - #define WAKE_PIN ENC_BTNB  */
//#define LIGHT_SENSOR      255               /*  Light sensor  */
//#define AUTOBACKLIGHT(x)  *function*        /*  Autobacklight function. See options.h for exsample  */
//#define DSP_INVERT_TITLE  true              /* Invert title colors for OLED displays ?  */
/******************************************/

/*  IR control  */
//#define IR_PIN                255
//#define IR_TIMEOUT            80              /*  see kTimeout description in IRremoteESP8266 exsample https://github.com/crankyoldgit/IRremoteESP8266/blob/master/examples/IRrecvDumpV2/IRrecvDumpV2.ino */

/******************************************/


#endif
