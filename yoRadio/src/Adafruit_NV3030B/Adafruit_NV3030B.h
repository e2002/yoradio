/*!
 * @file Adafruit_NV3030B.h
 *
 * Library to provide NV3030B display driver support in Adafruit_GFX.
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

#define NV3030B_TFTWIDTH 240  ///< Display width in pixels
#define NV3030B_TFTHEIGHT 320 ///< Display height in pixels

#define NV3030B_NOP 0x00     ///< No-op register

#define NV3030B_RDDID 0x04   ///< Read display identification information
#define NV3030B_RDDST 0x09   ///< Read Display Status

#define NV3030B_SLPIN 0x10  ///< Enter Sleep Mode
#define NV3030B_SLPOUT 0x11 ///< Sleep Out
#define NV3030B_PTLON 0x12  ///< Partial Mode ON
#define NV3030B_NORON 0x13  ///< Normal Display Mode ON and Partial Mode OFF

#define NV3030B_RDMODE 0x0A     ///< Read Display Power Mode
#define NV3030B_RDMADCTL 0x0B   ///< Read Display MADCTL
#define NV3030B_RDPIXFMT 0x0C   ///< Read Display Pixel Format
#define NV3030B_RDIMGMODE 0x0D  ///< Read Display Image Mode
// 0x0E = Read display Signal mode
#define NV3030B_RDSELFDIAG 0x0F ///< Read Display Self-Diagnostic Result

#define NV3030B_INVOFF 0x20   ///< Display Inversion OFF
#define NV3030B_INVON 0x21    ///< Display Inversion ON
#define NV3030B_DISPOFF 0x28  ///< Display OFF
#define NV3030B_DISPON 0x29   ///< Display ON

#define NV3030B_CASET 0x2A ///< Column Address Set
#define NV3030B_PASET 0x2B ///< Page Address Set
#define NV3030B_RAMWR 0x2C ///< Memory Write

#define NV3030B_PTLAR 0x30    ///< Partial Area
#define NV3030B_VSCRDEF 0x33  ///< Vertical Scrolling Definition
#define NV3030B_TEOFF 0x34    ///< Tearing effect line off
#define NV3030B_TEON 0x35     ///< Tearing effect line on
#define NV3030B_MADCTL 0x36   ///< Memory Access Control
#define NV3030B_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define NV3030B_IDLEOFF 0x38  ///< Idle mode off
#define NV3030B_IDLEON 0x39   ///< Idle mode on and other mode off

#define NV3030B_PIXFMT 0x3A   ///< Interface Pixel Format
// 0x3C = Write memory continue
// 0x44 = Set tear scanline
// 0x45 = Get tear scanline
// 0x53 = Write display brightness
// 0x54 = Read display brightness

// 0xD3 = read idd3
#define NV3030B_RDID1 0xDA ///< Read ID 1
#define NV3030B_RDID2 0xDB ///< Read ID 2
#define NV3030B_RDID3 0xDC ///< Read ID 3

// Color definitions
#define NV3030B_BLACK 0x0000       ///<   0,   0,   0
#define NV3030B_NAVY 0x000F        ///<   0,   0, 123
#define NV3030B_DARKGREEN 0x03E0   ///<   0, 125,   0
#define NV3030B_DARKCYAN 0x03EF    ///<   0, 125, 123
#define NV3030B_MAROON 0x7800      ///< 123,   0,   0
#define NV3030B_PURPLE 0x780F      ///< 123,   0, 123
#define NV3030B_OLIVE 0x7BE0       ///< 123, 125,   0
#define NV3030B_LIGHTGREY 0xC618   ///< 198, 195, 198
#define NV3030B_DARKGREY 0x7BEF    ///< 123, 125, 123
#define NV3030B_BLUE 0x001F        ///<   0,   0, 255
#define NV3030B_GREEN 0x07E0       ///<   0, 255,   0
#define NV3030B_CYAN 0x07FF        ///<   0, 255, 255
#define NV3030B_RED 0xF800         ///< 255,   0,   0
#define NV3030B_MAGENTA 0xF81F     ///< 255,   0, 255
#define NV3030B_YELLOW 0xFFE0      ///< 255, 255,   0
#define NV3030B_WHITE 0xFFFF       ///< 255, 255, 255
#define NV3030B_ORANGE 0xFD20      ///< 255, 165,   0
#define NV3030B_GREENYELLOW 0xAFE5 ///< 173, 255,  41
#define NV3030B_PINK 0xFC18        ///< 255, 130, 198

/*!
@brief Class to manage hardware interface with NV3030B chipset.
*/
class Adafruit_NV3030B : public Adafruit_SPITFT {
public:
  Adafruit_NV3030B(int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK,
                   int8_t _RST = -1, int8_t _MISO = -1);
  Adafruit_NV3030B(int8_t _CS, int8_t _DC, int8_t _RST = -1);
#if !defined(ESP8266)
  Adafruit_NV3030B(SPIClass *spiClass, int8_t dc, int8_t cs = -1,
                   int8_t rst = -1);
#endif // end !ESP8266
  Adafruit_NV3030B(tftBusWidth busWidth, int8_t d0, int8_t wr, int8_t dc,
                   int8_t cs = -1, int8_t rst = -1, int8_t rd = -1);

  void begin(uint32_t freq = 0);
  void setRotation(uint8_t r);
  void invertDisplay(bool i);

  void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void enableDisplay(boolean enable);
  void enableSleep(boolean enable);
};
