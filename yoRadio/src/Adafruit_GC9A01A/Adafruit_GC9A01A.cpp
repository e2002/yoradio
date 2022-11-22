/*!
 * @file Adafruit_GC9A01A.cpp
 *
 * @mainpage GC9A01A TFT Display library for Adafruit_GFX
 *
 * @section intro_sec Introduction
 *
 * Library to provide GC9A01A display driver support in Adafruit_GFX.
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
 * GC9A01A adaptation by Phil "PaintYourDragon" Burgess.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */
#include "../core/options.h"
#if DSP_MODEL==DSP_GC9A01A

#include "Adafruit_GC9A01A.h"
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

#define MADCTL_MY 0x80  ///< Bottom to top
#define MADCTL_MX 0x40  ///< Right to left
#define MADCTL_MV 0x20  ///< Reverse Mode
#define MADCTL_ML 0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MH 0x04  ///< LCD refresh right to left

/*!
    @brief  Instantiate Adafruit GC9A01A driver with software SPI
    @param  cs    Chip select pin #
    @param  dc    Data/Command pin #
    @param  mosi  SPI MOSI pin #
    @param  sclk  SPI Clock pin #
    @param  rst   Reset pin # (optional, pass -1 if unused)
    @param  miso  SPI MISO pin # (optional, pass -1 if unused)
*/
Adafruit_GC9A01A::Adafruit_GC9A01A(int8_t cs, int8_t dc, int8_t mosi,
                                   int8_t sclk, int8_t rst, int8_t miso)
    : Adafruit_SPITFT(GC9A01A_TFTWIDTH, GC9A01A_TFTHEIGHT, cs, dc, mosi, sclk,
                      rst, miso) {}

/*!
    @brief  Instantiate Adafruit GC9A01A driver with hardware SPI using the
            default SPI peripheral.
    @param  cs   Chip select pin # (OK to pass -1 if CS tied to GND).
    @param  dc   Data/Command pin # (required).
    @param  rst  Reset pin # (optional, pass -1 if unused).
*/
Adafruit_GC9A01A::Adafruit_GC9A01A(int8_t cs, int8_t dc, int8_t rst)
    : Adafruit_SPITFT(GC9A01A_TFTWIDTH, GC9A01A_TFTHEIGHT, cs, dc, rst) {}

#if !defined(ESP8266)
/*!
    @brief  Instantiate Adafruit GC9A01A driver with hardware SPI using
            a specific SPI peripheral (not necessarily default).
    @param  spiClass  Pointer to SPI peripheral (e.g. &SPI or &SPI1).
    @param  dc        Data/Command pin # (required).
    @param  cs        Chip select pin # (optional, pass -1 if unused and
                      CS is tied to GND).
    @param  rst       Reset pin # (optional, pass -1 if unused).
*/
Adafruit_GC9A01A::Adafruit_GC9A01A(SPIClass *spiClass, int8_t dc, int8_t cs,
                                   int8_t rst)
    : Adafruit_SPITFT(GC9A01A_TFTWIDTH, GC9A01A_TFTHEIGHT, spiClass, cs, dc,
                      rst) {}
#endif // end !ESP8266

/*!
    @brief  Instantiate Adafruit GC9A01A driver using parallel interface.
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
Adafruit_GC9A01A::Adafruit_GC9A01A(tftBusWidth busWidth, int8_t d0, int8_t wr,
                                   int8_t dc, int8_t cs, int8_t rst, int8_t rd)
    : Adafruit_SPITFT(GC9A01A_TFTWIDTH, GC9A01A_TFTHEIGHT, busWidth, d0, wr, dc,
                      cs, rst, rd) {}

// clang-format off
static const uint8_t PROGMEM initcmd[] = {
  GC9A01A_INREGEN2, 0,
  0xEB, 1, 0x14,
  GC9A01A_INREGEN1, 0,
  GC9A01A_INREGEN2, 0,
  0xEB, 1, 0x14,
  0x84, 1, 0x40,
  0x85, 1, 0xFF,
  0x86, 1, 0xFF,
  0x87, 1, 0xFF,
  0x88, 1, 0x0A,
  0x89, 1, 0x21,
  0x8A, 1, 0x00,
  0x8B, 1, 0x80,
  0x8C, 1, 0x01,
  0x8D, 1, 0x01,
  0x8E, 1, 0xFF,
  0x8F, 1, 0xFF,
  0xB6, 2, 0x00, 0x00,
  GC9A01A_MADCTL, 1, MADCTL_MX | MADCTL_BGR,
  GC9A01A_PIXFMT, 1, 0x05,
  0x90, 4, 0x08, 0x08, 0x08, 0x08,
  0xBD, 1, 0x06,
  0xBC, 1, 0x00,
  0xFF, 3, 0x60, 0x01, 0x04,
  GC9A01A1_VREG1A, 0x13,
  GC9A01A1_VREG1B, 0x13,
  GC9A01A1_VREG2A, 0x22,
  0xBE, 1, 0x11,
  ILI9341_GMCTRN1, 2, 0x10, 0x0E,
  0xDF, 3, 0x21, 0x0c, 0x02,
  GC9A01A_GAMMA1, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
  GC9A01A_GAMMA2, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
  GC9A01A_GAMMA3, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
  GC9A01A_GAMMA4, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
  0xED, 2, 0x1B, 0x0B,
  0xAE, 1, 0x77,
  0xCD, 1, 0x63,
  0x70, 9, 0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03,
  ILI9341_FRAMERATE, 1, 0x34,
  0x62, 12, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70,
            0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70,
  0x63, 12, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70,
            0x18, 0x13, 0x71, 0xF3, 0x70, 0x70,
  0x64, 7, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07,
  0x66, 10, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00,
  0x67, 10, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98,
  0x74, 7, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00,
  0x98, 2, 0x3e, 0x07,
  GC9A01A_TEON, 0,
  GC9A01A_INVON, 0,
  GC9A01A_SLPOUT, 0x80, // Exit sleep
  GC9A01A_DISPON, 0x80, // Display on
  0x00                  // End of list
};
// clang-format on

/*!
    @brief  Initialize GC9A01A chip. Connects to the GC9A01A over SPI
            and sends initialization commands.
    @param  freq  Desired SPI clock frequency
*/
void Adafruit_GC9A01A::begin(uint32_t freq) {

  if (!freq)
    freq = SPI_DEFAULT_FREQ;
  initSPI(freq);

  if (_rst < 0) {                 // If no hardware reset pin...
    sendCommand(GC9A01A_SWRESET); // Engage software reset
    delay(150);
  }

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

  _width = GC9A01A_TFTWIDTH;
  _height = GC9A01A_TFTHEIGHT;
}

/*!
    @brief  Set origin of (0,0) and orientation of TFT display
    @param  m  The index for rotation, from 0-3 inclusive
*/
void Adafruit_GC9A01A::setRotation(uint8_t m) {
  rotation = m % 4; // can't be higher than 3
  switch (rotation) 
  {
  case 0:
    m = (MADCTL_MX | MADCTL_BGR);
    _width = GC9A01A_TFTWIDTH;
    _height = GC9A01A_TFTHEIGHT;
    break;
  case 1:
    m = (MADCTL_MV | MADCTL_BGR);
    _width = GC9A01A_TFTHEIGHT;
    _height = GC9A01A_TFTWIDTH;
    break;
  case 2:
    m = (MADCTL_MY | MADCTL_BGR);
    _width = GC9A01A_TFTWIDTH;
    _height = GC9A01A_TFTHEIGHT;
    break;
  case 3:
    m = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
    _width = GC9A01A_TFTHEIGHT;
    _height = GC9A01A_TFTWIDTH;
    break;
  }
  sendCommand(GC9A01A_MADCTL, &m, 1);
}

/*!
    @brief  Enable/Disable display color inversion
    @param  invert True to invert, False to have normal color
*/
void Adafruit_GC9A01A::invertDisplay(bool invert) {
  sendCommand(invert ? GC9A01A_INVON : GC9A01A_INVOFF);
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
void Adafruit_GC9A01A::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w,
                                     uint16_t h) {
  uint16_t x2 = (x1 + w - 1), y2 = (y1 + h - 1);
  writeCommand(GC9A01A_CASET); // Column address set
  SPI_WRITE16(x1);
  SPI_WRITE16(x2);
  writeCommand(GC9A01A_PASET); // Row address set
  SPI_WRITE16(y1);
  SPI_WRITE16(y2);
  writeCommand(GC9A01A_RAMWR); // Write to RAM
}

void Adafruit_GC9A01A::enableDisplay(boolean enable) {
  sendCommand(enable ? GC9A01A_DISPON : GC9A01A_DISPOFF);
}

void Adafruit_GC9A01A::enableSleep(boolean enable) {
  sendCommand(enable ? GC9A01A_SLPIN : GC9A01A_SLPOUT);
}
#endif
