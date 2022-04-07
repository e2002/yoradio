/*
 * Adafruit_GC9106_kbv class inherits from Adafruit_GFX, Adafruit_SPITFT class and the Arduino Print class.
 * Adafruit_GC9106_kbv written by David Prentice
 *
 * Any use of Adafruit_GC9106_kbv class and examples is dependent on Adafruit and Arduino licenses
 * The license texts are in the accompanying license.txt file
 */

/*!
 * @file Adafruit_GC9106_kbv.h
 *
 * These displays use SPI to communicate, 4 or 5 pins are required
 * to interface (RST is optional IF YOU ADD A PULLUP RESISTOR).
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 *
 * This library depends on <a href="https://github.com/adafruit/Adafruit_GFX">
 * Adafruit_GFX</a> being present on your system. Please make sure you have
 * installed the latest version before using this library.
 *
 * Adafruit_GFX, Adafruit_SPITFT written by Limor "ladyada" Fried for Adafruit Industries.
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */

#ifndef _ADAFRUIT_GC9106_EX_H_
#define _ADAFRUIT_GC9106_EX_H_

#include "Adafruit_GFX.h"
#include "Arduino.h"
#include "Print.h"
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <SPI.h>

#define GC9106_TFTWIDTH 80  ///< GC9106 max TFT width
#define GC9106_TFTHEIGHT 160 ///< GC9106 max TFT height

#define GC9106_NOP 0x00     ///< No-op register
#define GC9106_SWRESET 0x01 ///< Software reset register
#define GC9106_RDDID 0x04   ///< Read display identification information
#define GC9106_RDDST 0x09   ///< Read Display Status

#define GC9106_SLPIN 0x10  ///< Enter Sleep Mode
#define GC9106_SLPOUT 0x11 ///< Sleep Out
#define GC9106_PTLON 0x12  ///< Partial Mode ON
#define GC9106_NORON 0x13  ///< Normal Display Mode ON

#define GC9106_RDMODE 0x0A     ///< Read Display Power Mode
#define GC9106_RDMADCTL 0x0B   ///< Read Display MADCTL
#define GC9106_RDPIXFMT 0x0C   ///< Read Display Pixel Format
#define GC9106_RDIMGFMT 0x0D   ///< Read Display Image Format
#define GC9106_RDSELFDIAG 0x0F ///< Read Display Self-Diagnostic Result

#define GC9106_INVOFF 0x20   ///< Display Inversion OFF
#define GC9106_INVON 0x21    ///< Display Inversion ON
#define GC9106_GAMMASET 0x26 ///< Gamma Set
#define GC9106_DISPOFF 0x28  ///< Display OFF
#define GC9106_DISPON 0x29   ///< Display ON

#define GC9106_CASET 0x2A ///< Column Address Set
#define GC9106_PASET 0x2B ///< Page Address Set
#define GC9106_RAMWR 0x2C ///< Memory Write
#define GC9106_RAMRD 0x2E ///< Memory Read

#define GC9106_PTLAR 0x30    ///< Partial Area
#define GC9106_VSCRDEF 0x33  ///< Vertical Scrolling Definition
#define GC9106_MADCTL 0x36   ///< Memory Access Control
#define GC9106_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define GC9106_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set


// Color definitions
#define TFT_BLACK 0x0000       ///<   0,   0,   0
#define TFT_NAVY 0x000F        ///<   0,   0, 123
#define TFT_DARKGREEN 0x03E0   ///<   0, 125,   0
#define TFT_DARKCYAN 0x03EF    ///<   0, 125, 123
#define TFT_MAROON 0x7800      ///< 123,   0,   0
#define TFT_PURPLE 0x780F      ///< 123,   0, 123
#define TFT_OLIVE 0x7BE0       ///< 123, 125,   0
#define TFT_LIGHTGREY 0xC618   ///< 198, 195, 198
#define TFT_DARKGREY 0x7BEF    ///< 123, 125, 123
#define TFT_BLUE 0x001F        ///<   0,   0, 255
#define TFT_GREEN 0x07E0       ///<   0, 255,   0
#define TFT_CYAN 0x07FF        ///<   0, 255, 255
#define TFT_RED 0xF800         ///< 255,   0,   0
#define TFT_MAGENTA 0xF81F     ///< 255,   0, 255
#define TFT_YELLOW 0xFFE0      ///< 255, 255,   0
#define TFT_WHITE 0xFFFF       ///< 255, 255, 255
#define TFT_ORANGE 0xFD20      ///< 255, 165,   0
#define TFT_GREENYELLOW 0xAFE5 ///< 173, 255,  41
#define TFT_PINK 0xFC18        ///< 255, 130, 198

/**************************************************************************/
/*!
@brief Class to manage hardware interface with GC9106 chipset
*/
/**************************************************************************/

class Adafruit_GC9106Ex : public Adafruit_SPITFT {
public:
  Adafruit_GC9106Ex(int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK,
                   int8_t _RST = -1, int8_t _MISO = -1);
  Adafruit_GC9106Ex(int8_t _CS, int8_t _DC, int8_t _RST = -1);
#if !defined(ESP8266)
  Adafruit_GC9106Ex(SPIClass *spiClass, int8_t dc, int8_t cs = -1,
                   int8_t rst = -1);
#endif // end !ESP8266
  Adafruit_GC9106Ex(tftBusWidth busWidth, int8_t d0, int8_t wr, int8_t dc,
                   int8_t cs = -1, int8_t rst = -1, int8_t rd = -1);

  void begin(uint32_t freq = 0);
  void setRotation(uint8_t r);
  void invertDisplay(bool i);
  void scrollTo(uint16_t y);
  void setScrollMargins(uint16_t top, uint16_t bottom);

  // Transaction API not used by GFX
  void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

protected:
  uint8_t _colstart = 0,   ///< Some displays need this changed to offset
      _rowstart = 0;       ///< Some displays need this changed to offset
};

#endif // _ADAFRUIT_GC9106H_
