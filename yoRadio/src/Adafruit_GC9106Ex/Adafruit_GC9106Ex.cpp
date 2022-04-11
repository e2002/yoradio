/*!
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
#include "../../options.h"
#if DSP_MODEL==DSP_GC9106

#include "Adafruit_GC9106Ex.h"
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

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit GC9106 driver with software SPI
    @param    cs    Chip select pin #
    @param    dc    Data/Command pin #
    @param    mosi  SPI MOSI pin #
    @param    sclk  SPI Clock pin #
    @param    rst   Reset pin # (optional, pass -1 if unused)
    @param    miso  SPI MISO pin # (optional, pass -1 if unused)
*/
/**************************************************************************/
Adafruit_GC9106Ex::Adafruit_GC9106Ex(int8_t cs, int8_t dc, int8_t mosi,
                                   int8_t sclk, int8_t rst, int8_t miso)
    : Adafruit_SPITFT(GC9106_TFTWIDTH, GC9106_TFTHEIGHT, cs, dc, mosi, sclk,
                      rst, miso) {}

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit GC9106 driver with hardware SPI using the
            default SPI peripheral.
    @param  cs   Chip select pin # (OK to pass -1 if CS tied to GND).
    @param  dc   Data/Command pin # (required).
    @param  rst  Reset pin # (optional, pass -1 if unused).
*/
/**************************************************************************/
Adafruit_GC9106Ex::Adafruit_GC9106Ex(int8_t cs, int8_t dc, int8_t rst)
    : Adafruit_SPITFT(GC9106_TFTWIDTH, GC9106_TFTHEIGHT, cs, dc, rst) {}

#if !defined(ESP8266)
/**************************************************************************/
/*!
    @brief  Instantiate Adafruit GC9106 driver with hardware SPI using
            a specific SPI peripheral (not necessarily default).
    @param  spiClass  Pointer to SPI peripheral (e.g. &SPI or &SPI1).
    @param  dc        Data/Command pin # (required).
    @param  cs        Chip select pin # (optional, pass -1 if unused and
                      CS is tied to GND).
    @param  rst       Reset pin # (optional, pass -1 if unused).
*/
/**************************************************************************/
Adafruit_GC9106Ex::Adafruit_GC9106Ex(SPIClass *spiClass, int8_t dc, int8_t cs,
                                   int8_t rst)
    : Adafruit_SPITFT(GC9106_TFTWIDTH, GC9106_TFTHEIGHT, spiClass, cs, dc,
                      rst) {}
#endif // end !ESP8266

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit GC9106 driver using parallel interface.
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
/**************************************************************************/
Adafruit_GC9106Ex::Adafruit_GC9106Ex(tftBusWidth busWidth, int8_t d0, int8_t wr,
                                   int8_t dc, int8_t cs, int8_t rst, int8_t rd)
    : Adafruit_SPITFT(GC9106_TFTWIDTH, GC9106_TFTHEIGHT, busWidth, d0, wr, dc,
                      cs, rst, rd) {}

// clang-format off
static const uint8_t PROGMEM initcmd[] = {
    //  (COMMAND_BYTE), n, data_bytes....
    0x01, 0x80,         // Soft reset, then delay 150 ms
    (0x28), 0,            //Display Off
    (0xfe), 0,       //GC9106_ENAB1
    (0xfe), 0,
    (0xfe), 0,
    (0xef), 0,       //GC9106_ENAB2
    (0xb3), 1, 0x03, //GC9106_ACCESS_F0_F1
    (0x36), 1, 0xd8, //USER_MADCTL
    (0x3a), 1, 0x05, //USER_COLMOD
    (0xb6), 1, 0x11, //GC9106_ACCESS_A3_AA_AC
    (0xac), 1, 0x0b, //undocumented
    (0xb4), 1, 0x21, //GC9106_INVCTR
    (0xb0), 1, 0x00, //GC9106_ACCESS_C0_C1_C2_C3_C6
    (0xb2), 1, 0x00, //GC9106_ACCESS_E4_EB
    (0xb1), 1, 0xc0, //GC9106_ACCESS_E6_E7
    (0xe6), 2, 0x50, 0x43, //GC9106_VREG1 [50 43]
    (0xe7), 2, 0x56, 0x43, //GC9106_VREG2 [38 43]
    (0xF0), 14, 0x1f, 0x41, 0x1B, 0x55, 0x36, 0x3d, 0x3e, 0x0, 0x16, 0x08, 0x09, 0x15, 0x14, 0xf,
    (0xF1), 14, 0x1f, 0x41, 0x1B, 0x55, 0x36, 0x3d, 0x3e, 0x0, 0x16, 0x08, 0x09, 0x15, 0x14, 0xf,
    (0xfe), 0,       //GC9106_ENAB1
    (0xff), 0,       //???
    (0x35), 1, 0x00, //USER_TEON
    (0x44), 1, 0x00, //GC9106_SETSCANLINE
	(0x11),	0x80, //USER_SLPOUT
    (0x29), 0,       //USER_DISPON
    (0x2A), 4, /***Set Column Address***/ 0x00, 0x18, 0x00, 0x67,
    (0x2B), 4, /***Set Page Address***/ 0x00, 0x00, 0x00, 0x9f,
    //(0x2c), 0,       //USER_MEMWR
    0x11, 0x80, // Exit Sleep, then delay 150 ms
    0x29, 0x80, // Main screen turn on, delay 150 ms
  0x00                                   // End of list
};
// clang-format on

/**************************************************************************/
/*!
    @brief   Initialize GC9106 chip
    Connects to the GC9106 over SPI and sends initialization procedure commands
    @param    freq  Desired SPI clock frequency
*/
/**************************************************************************/
void Adafruit_GC9106Ex::begin(uint32_t freq) {

  if (!freq)
    freq = SPI_DEFAULT_FREQ;
  freq = 8000000;
  initSPI(freq);

  if (_rst < 0) {                 // If no hardware reset pin...
    sendCommand(GC9106_SWRESET); // Engage software reset
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

  _width = GC9106_TFTWIDTH;
  _height = GC9106_TFTHEIGHT;
  _colstart =24;
  _rowstart = 0;
}

/**************************************************************************/
/*!
    @brief   Set origin of (0,0) and orientation of TFT display
    @param   m  The index for rotation, from 0-3 inclusive
*/
/**************************************************************************/
void Adafruit_GC9106Ex::setRotation(uint8_t m) {
  rotation = m % 4; // can't be higher than 3
  switch (rotation) {
  case 0:
    m = (MADCTL_MX | MADCTL_ML | MADCTL_BGR);
    _width = GC9106_TFTWIDTH;
    _height = GC9106_TFTHEIGHT;
    _xstart = _colstart;
    _ystart = _rowstart;
    break;
  case 1:
    m = (MADCTL_MV | MADCTL_ML | MADCTL_BGR);
    _width = GC9106_TFTHEIGHT;
    _height = GC9106_TFTWIDTH;
    _ystart = _colstart;
    _xstart = _rowstart;
    break;
  case 2:
    m = (MADCTL_MY | MADCTL_BGR);
    _width = GC9106_TFTWIDTH;
    _height = GC9106_TFTHEIGHT;
    _xstart = _colstart;
    _ystart = _rowstart;
    break;
  case 3:
    m = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
    _width = GC9106_TFTHEIGHT;
    _height = GC9106_TFTWIDTH;
    _ystart = _colstart;
    _xstart = _rowstart;
    break;
  }
  m ^= 0x80; //.kbv
  sendCommand(GC9106_MADCTL, &m, 1);
  setScrollMargins(0, 0); //.kbv
  scrollTo(0);
}

/**************************************************************************/
/*!
    @brief   Enable/Disable display color inversion
    @param   invert True to invert, False to have normal color
*/
/**************************************************************************/
void Adafruit_GC9106Ex::invertDisplay(bool invert) {
  sendCommand(invert ? GC9106_INVON : GC9106_INVOFF);
}

/**************************************************************************/
/*!
    @brief   Scroll display memory
    @param   y How many pixels to scroll display by
*/
/**************************************************************************/
void Adafruit_GC9106Ex::scrollTo(uint16_t y) {
  uint8_t data[2];
  data[0] = y >> 8;
  data[1] = y & 0xff;
  sendCommand(GC9106_VSCRSADD, (uint8_t *)data, 2);
}

/**************************************************************************/
/*!
    @brief   Set the height of the Top and Bottom Scroll Margins
    @param   top The height of the Top scroll margin
    @param   bottom The height of the Bottom scroll margin
 */
/**************************************************************************/
void Adafruit_GC9106Ex::setScrollMargins(uint16_t top, uint16_t bottom) {
  // TFA+VSA+BFA must equal 480
  if (top + bottom <= GC9106_TFTHEIGHT) {
    uint16_t middle = GC9106_TFTHEIGHT - top - bottom;
    uint8_t data[6];
    data[0] = top >> 8;
    data[1] = top & 0xff;
    data[2] = middle >> 8;
    data[3] = middle & 0xff;
    data[4] = bottom >> 8;
    data[5] = bottom & 0xff;
    sendCommand(GC9106_VSCRDEF, (uint8_t *)data, 6);
  }
}

/**************************************************************************/
/*!
    @brief   Set the "address window" - the rectangle we will write to RAM with
   the next chunk of      SPI data writes. The GC9106 will automatically wrap
   the data as each row is filled
    @param   x1  TFT memory 'x' origin
    @param   y1  TFT memory 'y' origin
    @param   w   Width of rectangle
    @param   h   Height of rectangle
*/
/**************************************************************************/
void Adafruit_GC9106Ex::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w,
                                     uint16_t h) {
  x1 += _xstart;
  y1 += _ystart;
  uint16_t x2 = (x1 + w - 1), y2 = (y1 + h - 1);
  writeCommand(GC9106_CASET); // Column address set
  SPI_WRITE16(x1);
  SPI_WRITE16(x2);
  writeCommand(GC9106_PASET); // Row address set
  SPI_WRITE16(y1);
  SPI_WRITE16(y2);
  writeCommand(GC9106_RAMWR); // Write to RAM
}
#endif
