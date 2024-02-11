/*!
 * @file Adafruit_NV3030B.cpp
 *
 * @mainpage NV3030B TFT Display library for Adafruit_GFX
 *
 * @section intro_sec Introduction
 *
 * Library to provide NV3030B display driver support in Adafruit_GFX.
 * ADAFRUIT DOES NOT PROVIDE TECHNICAL SUPPORT FOR THESE DISPLAYS NOR
 * THIS CODE. This was adapted from a prior library (Adafruit_ILI9341)
 * for use with an IPS screen from BuyDisplay.com.
 *
 * These displays use SPI to communicate, 4 or 5 pins are required
 * to interface (RST is optional).
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section dependencies Dependencies
 *
 * This library depends on <a href="https://github.com/adafruit/Adafruit_GFX">
 * Adafruit_GFX</a> being present on your system. Please make sure you have
 * installed the latest version before using this library.
 *
 * @section author Author
 *
 * Written by Limor "ladyada" Fried for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */
#include "../core/options.h"
#if DSP_MODEL==DSP_NV3030B

#include "Adafruit_NV3030B.h"
#ifndef ARDUINO_STM32_FEATHER
#include "pins_arduino.h"
#ifndef RASPI
#include "wiring_private.h"
#endif
#endif
#include <limits.h>

#if defined(ARDUINO_ARCH_ARC32) || defined(ARDUINO_MAXIM)
#define SPI_DEFAULT_FREQ 16000000
// Teensy 3.0, 3.1/3.2, 3.5, 3.6
#elif defined(__MK20DX128__) || defined(__MK20DX256__) ||                      \
    defined(__MK64FX512__) || defined(__MK66FX1M0__)
#define SPI_DEFAULT_FREQ 40000000
#elif defined(__AVR__) || defined(TEENSYDUINO)
#define SPI_DEFAULT_FREQ 8000000
#elif defined(ESP8266) || defined(ESP32)
#define SPI_DEFAULT_FREQ 40000000
#elif defined(RASPI)
#define SPI_DEFAULT_FREQ 80000000
#elif defined(ARDUINO_ARCH_STM32F1)
#define SPI_DEFAULT_FREQ 36000000
#else
#define SPI_DEFAULT_FREQ 24000000 ///< Default SPI data clock frequency
#endif

#define MADCTL_MY 0x80  ///< Bottom to top (Page Address Order)
#define MADCTL_MX 0x40  ///< Right to left (Column Address Order)
#define MADCTL_MV 0x20  ///< Reverse Mode (Page/Column Order)
#define MADCTL_ML 0x10  ///< LCD refresh Bottom to top (Line Address Order)
#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order

/*!
    @brief  Instantiate Adafruit NV3030B driver with software SPI
    @param  cs    Chip select pin #
    @param  dc    Data/Command pin #
    @param  mosi  SPI MOSI pin #
    @param  sclk  SPI Clock pin #
    @param  rst   Reset pin # (optional, pass -1 if unused)
    @param  miso  SPI MISO pin # (optional, pass -1 if unused)
*/
Adafruit_NV3030B::Adafruit_NV3030B(int8_t cs, int8_t dc, int8_t mosi,
                                   int8_t sclk, int8_t rst, int8_t miso)
    : Adafruit_SPITFT(NV3030B_TFTWIDTH, NV3030B_TFTHEIGHT, cs, dc, mosi, sclk,
                      rst, miso) {}

/*!
    @brief  Instantiate Adafruit NV3030B driver with hardware SPI using the
            default SPI peripheral.
    @param  cs   Chip select pin # (OK to pass -1 if CS tied to GND).
    @param  dc   Data/Command pin # (required).
    @param  rst  Reset pin # (optional, pass -1 if unused).
*/
Adafruit_NV3030B::Adafruit_NV3030B(int8_t cs, int8_t dc, int8_t rst)
    : Adafruit_SPITFT(NV3030B_TFTWIDTH, NV3030B_TFTHEIGHT, cs, dc, rst) {}

#if !defined(ESP8266)
/*!
    @brief  Instantiate Adafruit NV3030B driver with hardware SPI using
            a specific SPI peripheral (not necessarily default).
    @param  spiClass  Pointer to SPI peripheral (e.g. &SPI or &SPI1).
    @param  dc        Data/Command pin # (required).
    @param  cs        Chip select pin # (optional, pass -1 if unused and
                      CS is tied to GND).
    @param  rst       Reset pin # (optional, pass -1 if unused).
*/
Adafruit_NV3030B::Adafruit_NV3030B(SPIClass *spiClass, int8_t dc, int8_t cs,
                                   int8_t rst)
    : Adafruit_SPITFT(NV3030B_TFTWIDTH, NV3030B_TFTHEIGHT, spiClass, cs, dc,
                      rst) {}
#endif // end !ESP8266

/*!
    @brief  Instantiate Adafruit NV3030B driver using parallel interface.
    @param  busWidth  If tft16 (enumeration in Adafruit_SPITFT.h), is a
                      16-bit interface, else 8-bit.
    @param  d0        Data pin 0 (MUST be a byte- or word-aligned LSB of a
                      PORT register -- pins 1-n are extrapolated from this).
    @param  wr        Write strobe pin # (required).
    @param  dc        Data/Command pin # (required).
    @param  cs        Chip select pin # (optional, pass -1 if unused and CS
                      is tied to GND).
    @param  rst       Reset pin # (optional, pass -1 if unused).
    @param  rd        Read strobe pin # (optional, pass -1 if unused).
*/
Adafruit_NV3030B::Adafruit_NV3030B(tftBusWidth busWidth, int8_t d0, int8_t wr,
                                   int8_t dc, int8_t cs, int8_t rst, int8_t rd)
    : Adafruit_SPITFT(NV3030B_TFTWIDTH, NV3030B_TFTHEIGHT, busWidth, d0, wr, dc,
                      cs, rst, rd) {}

// clang-format off
static const uint8_t PROGMEM initcmd[] = {
  0xFD, 2, 0x06, 0x08, //enter read/write private register
  0x61, 2, 0x07, 0x04, //dvdd setting
  0x62, 4, 0x00, 0x44, 0x40, 0x01, //bias setting
  0x63, 4, 0x41, 0x07, 0x12, 0x12, //vgl setting
  0x64, 1, 0x37, //vgh setting
  0x65, 3, 0x09, 0x10, 0x21, //VSP
  0x66, 3, 0x09, 0x10, 0x21, //VSN
  0x67, 2, 0x20, 0x40, //pump clock set
  0x68, 4, 0x90, 0x4C, 0x7C, 0x06, //gamma ref 1
  0xB1, 3, 0x0F, 0x02, 0x01, //frame rate
  0xB4, 1, 0x01, //display pol control
  0xB5, 4, 0x02, 0x02, 0x0A, 0x14, //blanking porch
  0xB6, 5, 0x04, 0x01, 0x9F, 0x00, 0x02, //display function
  0xDF, 1, 0x11, //gofc_gamma_en_sel=1
  0xE2, 6, 0x03, 0x00, 0x00, 0x26, 0x27, 0x3F, //gamma positive 3
  0xE5, 6, 0x3F, 0x27, 0x26, 0x00, 0x00, 0x03, //gamma negative 3
  0xE1, 2, 0x00, 0x57, //gamma positive 2
  0xE4, 2, 0x58, 0x00, //gamma negative 2
  0xE0, 8, 0x01, 0x03, 0x0D, 0x0E, 0x0E, 0x0C, 0x15, 0x19, //gamma positive 1
  0xE3, 8, 0x1A, 0x16, 0x0C, 0x0F, 0x0E, 0x0D, 0x02, 0x01, //gamma negative 1
  0xE6, 2, 0x00, 0xFF, //SRC_CTRL1
  0xE7, 6, 0x01, 0x04, 0x03, 0x03, 0x00, 0x12, //SRC_CTRL2
  0xE8, 3, 0x00, 0x70, 0x00, //SRC_CTRL3
  0xEC, 1, 0x52, //Gate driver timing
  0xF1, 3, 0x01, 0x01, 0x02, //tearing effect
  0xF6, 4, 0x09, 0x10, 0x00, 0x00, //interface control
  0xFD, 2, 0xFA, 0xFC, //exit read/write private register
  NV3030B_PIXFMT, 1, 0x05,
  NV3030B_TEON, 1, 0x00,
  NV3030B_MADCTL, 1, 0x08,
  NV3030B_INVON, 0,
  NV3030B_SLPOUT, 0x80, // Exit sleep
  NV3030B_DISPON, 0x80, // Display on
  0x00 // End of list
};
// clang-format on

/*!
    @brief  Initialize NV3030B chip. Connects to the NV3030B over SPI
            and sends initialization commands.
    @param  freq  Desired SPI clock frequency
*/
void Adafruit_NV3030B::begin(uint32_t freq) {

  if (!freq)
    freq = SPI_DEFAULT_FREQ;
  initSPI(freq);

  uint8_t cmd, x, numArgs;
  const uint8_t *addr = initcmd;
  while ((cmd = pgm_read_byte(addr++)) > 0) {
    x = pgm_read_byte(addr++);
    numArgs = x & 0x7F;
    sendCommand(cmd, addr, numArgs);
    addr += numArgs;
    if (x & 0x80)
      delay(150);
  }

  _width = NV3030B_TFTWIDTH;
  _height = NV3030B_TFTHEIGHT;
}

/*!
    @brief  Set origin of (0,0) and orientation of TFT display
    @param  m  The index for rotation, from 0-3 inclusive
*/
void Adafruit_NV3030B::setRotation(uint8_t m) {
  rotation = m % 4; // can't be higher than 3
  switch (rotation) 
  {
  case 0:
    m = (MADCTL_BGR);
    _width = NV3030B_TFTWIDTH;
    _height = NV3030B_TFTHEIGHT;
    break;
  case 1:
    m = (MADCTL_MX | MADCTL_MV | MADCTL_BGR);
    _width = NV3030B_TFTHEIGHT;
    _height = NV3030B_TFTWIDTH;
    break;
  case 2:
    m = (MADCTL_MX | MADCTL_MY | MADCTL_BGR);
    _width = NV3030B_TFTWIDTH;
    _height = NV3030B_TFTHEIGHT;
    break;
  case 3:
    m = (MADCTL_MY | MADCTL_MV | MADCTL_BGR);
    _width = NV3030B_TFTHEIGHT;
    _height = NV3030B_TFTWIDTH;
    break;
  }
  sendCommand(NV3030B_MADCTL, &m, 1);
}

/*!
    @brief  Enable/Disable display color inversion
    @param  invert True to invert, False to have normal color
*/
void Adafruit_NV3030B::invertDisplay(bool invert) {
  sendCommand(invert ? NV3030B_INVON : NV3030B_INVOFF);
}

/*!
    @brief  Set the "address window" - the rectangle we will write to RAM
            with the next chunk of SPI data. The GC9A01A will automatically
            wrap the data as each row is filled.
    @param  x1  TFT memory 'x' origin
    @param  y1  TFT memory 'y' origin
    @param  w   Width of rectangle
    @param  h   Height of rectangle
*/
void Adafruit_NV3030B::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w,
                                     uint16_t h) {
  uint16_t x2 = (x1 + w - 1), y2 = (y1 + h - 1);
  writeCommand(NV3030B_CASET); // Column address set
  SPI_WRITE16(x1);
  SPI_WRITE16(x2);
  writeCommand(NV3030B_PASET); // Row address set
  SPI_WRITE16(y1);
  SPI_WRITE16(y2);
  writeCommand(NV3030B_RAMWR); // Write to RAM
}

void Adafruit_NV3030B::enableDisplay(boolean enable) {
  sendCommand(enable ? NV3030B_DISPON : NV3030B_DISPOFF);
}

void Adafruit_NV3030B::enableSleep(boolean enable) {
  sendCommand(enable ? NV3030B_SLPIN : NV3030B_SLPOUT);
}
#endif
