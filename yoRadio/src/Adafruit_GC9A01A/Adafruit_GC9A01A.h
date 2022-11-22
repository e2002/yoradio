/*!
 * @file Adafruit_GC9A01A.h
 *
 * Library to provide GC9A01A display driver support in Adafruit_GFX.
 * ADAFRUIT DOES NOT PROVIDE TECHNICAL SUPPORT FOR THESE DISPLAYS NOR
 * THIS CODE. This was adapted from a prior library (Adafruit_ILI9341)
 * for use with an IPS screen from BuyDisplay.com.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * This library depends on <a href="https://github.com/adafruit/Adafruit_GFX">
 * Adafruit_GFX</a> being present on your system. Please make sure you have
 * installed the latest version before using this library.
 *
 * Written by Limor "ladyada" Fried for Adafruit Industries.
 * GC9A01A adaptation by Phil "PaintYourDragon" Burgess.
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */

#pragma once

#include "Adafruit_GFX.h"
#include "Arduino.h"
#include "Print.h"
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <SPI.h>

#define GC9A01A_TFTWIDTH 240  ///< Display width in pixels
#define GC9A01A_TFTHEIGHT 240 ///< Display height in pixels

// NOTE: ILI9341 registers defined (but commented out) are ones that
// *might* be compatible with the GC9A01A, but aren't documented in
// the device datasheet. A couple are defined (with ILI name) and NOT
// commented out because they appeared in the manufacturer example code
// as raw register addresses, no documentation in datasheet, they SEEM
// to do the same thing as their ILI equivalents but this is not 100%
// confirmed so they've not been assigned GC9A01A register defines.

//#define ILI9341_NOP 0x00     ///< No-op register
#define GC9A01A_SWRESET 0x01 ///< Software reset register

//#define GC9A01A 0x04   ///< Read display identification information
//#define GC9A01A 0x09   ///< Read Display Status

#define GC9A01A_SLPIN 0x10  ///< Enter Sleep Mode
#define GC9A01A_SLPOUT 0x11 ///< Sleep Out
#define GC9A01A_PTLON 0x12  ///< Partial Mode ON
#define GC9A01A_NORON 0x13  ///< Normal Display Mode ON

//#define ILI9341_RDMODE 0x0A     ///< Read Display Power Mode
//#define ILI9341_RDMADCTL 0x0B   ///< Read Display MADCTL
//#define ILI9341_RDPIXFMT 0x0C   ///< Read Display Pixel Format
//#define ILI9341_RDIMGFMT 0x0D   ///< Read Display Image Format
//#define ILI9341_RDSELFDIAG 0x0F ///< Read Display Self-Diagnostic Result

#define GC9A01A_INVOFF 0x20   ///< Display Inversion OFF
#define GC9A01A_INVON 0x21    ///< Display Inversion ON
//#define ILI9341_GAMMASET 0x26 ///< Gamma Set
#define GC9A01A_DISPOFF 0x28  ///< Display OFF
#define GC9A01A_DISPON 0x29   ///< Display ON

#define GC9A01A_CASET 0x2A ///< Column Address Set
#define GC9A01A_PASET 0x2B ///< Page Address Set
#define GC9A01A_RAMWR 0x2C ///< Memory Write
//#define ILI9341_RAMRD 0x2E ///< Memory Read

#define GC9A01A_PTLAR 0x30    ///< Partial Area
#define GC9A01A_VSCRDEF 0x33  ///< Vertical Scrolling Definition
#define GC9A01A_TEOFF 0x34    ///< Tearing effect line off
#define GC9A01A_TEON 0x35     ///< Tearing effect line on
#define GC9A01A_MADCTL 0x36   ///< Memory Access Control
#define GC9A01A_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define GC9A01A_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set
// 0x3C = Write memory continue
// 0x44 = Set tear scanline
// 0x45 = Get scanline
// 0x51 = Write display brightness
// 0x53 = Write CTRL display

// 0xA7 = Vcore voltage control

// 0xB0 = RGB interface signal control
//#define ILI9341_FRMCTR1  0xB1 ///< Frame Rate Control (In Normal Mode/Full Colors)
//#define ILI9341_FRMCTR2 0xB2 ///< Frame Rate Control (In Idle Mode/8 colors)
//#define ILI9341_FRMCTR3 0xB3 ///< Frame Rate control (In Partial Mode/Full Colors)
//#define ILI9341_INVCTR 0xB4  ///< Display Inversion Control
// 0xB5 = Blanking porch control
#define GC9A01A1_DFUNCTR 0xB6 ///< Display Function Control
// 0xBA = TE control

// 0xC1 = Power criterion control
#define GC9A01A1_VREG1A 0xC3 ///< Vreg1a voltage control
#define GC9A01A1_VREG1B 0xC4 ///< Vreg1b voltage control
//#define ILI9341_PWCTR1 0xC0 ///< Power Control 1
//#define ILI9341_PWCTR2 0xC1 ///< Power Control 2
//#define ILI9341_PWCTR3 0xC2 ///< Power Control 3
//#define ILI9341_PWCTR4 0xC3 ///< Power Control 4
//#define ILI9341_PWCTR5 0xC4 ///< Power Control 5
//#define ILI9341_VMCTR1 0xC5 ///< VCOM Control 1
//#define ILI9341_VMCTR2 0xC7 ///< VCOM Control 2
#define GC9A01A1_VREG2A 0xC9 ///< Vreg2a voltage control

#define GC9A01A_RDID1 0xDA ///< Read ID 1
#define GC9A01A_RDID2 0xDB ///< Read ID 2
#define GC9A01A_RDID3 0xDC ///< Read ID 3
//#define ILI9341_RDID4 0xDD ///< Read ID 4

#define ILI9341_GMCTRP1 0xE0 ///< Positive Gamma Correction
#define ILI9341_GMCTRN1 0xE1 ///< Negative Gamma Correction
#define ILI9341_FRAMERATE 0xE8 ///< Frame rate control
// 0xE9 = SPI 2data control
// 0xEC = Charge pump frequent control
#define GC9A01A_INREGEN2 0xEF ///< Inter register enable 2
#define GC9A01A_GAMMA1 0xF0 ///< Set gamma 1
#define GC9A01A_GAMMA2 0xF1 ///< Set gamma 2
#define GC9A01A_GAMMA3 0xF2 ///< Set gamma 3
#define GC9A01A_GAMMA4 0xF3 ///< Set gamma 4
//#define ILI9341_PWCTR6     0xFC
// 0xF6 = Interface control
#define GC9A01A_INREGEN1 0xFE ///< Inter register enable 1

// Color definitions
#define GC9A01A_BLACK 0x0000       ///<   0,   0,   0
#define GC9A01A_NAVY 0x000F        ///<   0,   0, 123
#define GC9A01A_DARKGREEN 0x03E0   ///<   0, 125,   0
#define GC9A01A_DARKCYAN 0x03EF    ///<   0, 125, 123
#define GC9A01A_MAROON 0x7800      ///< 123,   0,   0
#define GC9A01A_PURPLE 0x780F      ///< 123,   0, 123
#define GC9A01A_OLIVE 0x7BE0       ///< 123, 125,   0
#define GC9A01A_LIGHTGREY 0xC618   ///< 198, 195, 198
#define GC9A01A_DARKGREY 0x7BEF    ///< 123, 125, 123
#define GC9A01A_BLUE 0x001F        ///<   0,   0, 255
#define GC9A01A_GREEN 0x07E0       ///<   0, 255,   0
#define GC9A01A_CYAN 0x07FF        ///<   0, 255, 255
#define GC9A01A_RED 0xF800         ///< 255,   0,   0
#define GC9A01A_MAGENTA 0xF81F     ///< 255,   0, 255
#define GC9A01A_YELLOW 0xFFE0      ///< 255, 255,   0
#define GC9A01A_WHITE 0xFFFF       ///< 255, 255, 255
#define GC9A01A_ORANGE 0xFD20      ///< 255, 165,   0
#define GC9A01A_GREENYELLOW 0xAFE5 ///< 173, 255,  41
#define GC9A01A_PINK 0xFC18        ///< 255, 130, 198

/*!
@brief Class to manage hardware interface with GC9A01A chipset.
*/
class Adafruit_GC9A01A : public Adafruit_SPITFT {
public:
  Adafruit_GC9A01A(int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK,
                   int8_t _RST = -1, int8_t _MISO = -1);
  Adafruit_GC9A01A(int8_t _CS, int8_t _DC, int8_t _RST = -1);
#if !defined(ESP8266)
  Adafruit_GC9A01A(SPIClass *spiClass, int8_t dc, int8_t cs = -1,
                   int8_t rst = -1);
#endif // end !ESP8266
  Adafruit_GC9A01A(tftBusWidth busWidth, int8_t d0, int8_t wr, int8_t dc,
                   int8_t cs = -1, int8_t rst = -1, int8_t rd = -1);

  void begin(uint32_t freq = 0);
  void setRotation(uint8_t r);
  void invertDisplay(bool i);

  void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void enableDisplay(boolean enable);
  void enableSleep(boolean enable);
};
