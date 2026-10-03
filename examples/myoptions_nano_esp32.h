#ifndef myoptions_h
#define myoptions_h

/*******************************************************
  yoRadio hardware configuration example for the Arduino Nano ESP32

  Copy this file to yoRadio/myoptions.h (next to yoRadio.ino) and adjust
  it to your wiring. See README.md for the connection tables.

  Board:   Arduino Nano ESP32 (ESP32-S3, 16MB flash, 8MB PSRAM)
  Build:   arduino-cli compile --fqbn esp32:esp32:nano_nora:PartitionScheme=spiffs,PinNumbers=byGPIONumber .

  All pin numbers below are ESP32-S3 GPIO numbers (PinNumbers=byGPIONumber).
  The Nano header label for each pin is shown in the comment.
********************************************************/

/*        DISPLAY: 0.96" 128x64 SSD1306 OLED over I2C      */
#define DSP_MODEL   DSP_SSD1306
#define I2C_SDA     11      // Nano A4 / SDA
#define I2C_SCL     12      // Nano A5 / SCL
// Default I2C address is 0x3C. If your module answers on 0x3D, uncomment:
// #define SCREEN_ADDRESS 0x3D

/*        ROTARY ENCODER                                   */
#define ENC_BTNL    8       // Nano D5  - encoder CLK (A)
#define ENC_BTNR    9       // Nano D6  - encoder DT  (B)
#define ENC_BTNB    10      // Nano D7  - encoder SW  (push button)
#define ENC_INTERNALPULLUP  true   // use internal pull-ups; set false if your module has its own
#define ENC_HALFQUARD       false  // set true if one detent moves two steps

/*        I2S DAC (e.g. PCM5102 / MAX98357A)               */
#define I2S_BCLK    5       // Nano D2  - BCK
#define I2S_LRC     6       // Nano D3  - LRCK / WS
#define I2S_DOUT    7       // Nano D4  - DIN

/*        ONBOARD LED                                      */
// The Nano's LED_BUILTIN is GPIO48, shared with SPI SCK (D13).
// Nothing here uses SPI, so the LED can stay enabled. If you later add an
// SPI device (SD card, VS1053, SPI display) set this to false.
#define USE_BUILTIN_LED  true

/*        NOT USED                                         */
#define VS1053_CS   255     // no VS1053 decoder
#define SDC_CS      255     // no SD card

#endif
