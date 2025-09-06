#ifndef _ADAFRUIT_ILI9225H_
#define _ADAFRUIT_ILI9225H_

#include "Adafruit_GFX.h"
#include "Arduino.h"
#include "Print.h"
#include <Adafruit_SPITFT.h>
#include <SPI.h>
/******************************/
#define ILI9225_DELAY                   (0xDEu)
#define ILI9225_DRIVER_OUTPUT_CTRL      (0x01u)  // Driver Output Control
#define ILI9225_LCD_AC_DRIVING_CTRL     (0x02u)  // LCD AC Driving Control
#define ILI9225_ENTRY_MODE              (0x03u)  // Entry Mode
#define ILI9225_DISP_CTRL1              (0x07u)  // Display Control 1
#define ILI9225_BLANK_PERIOD_CTRL1      (0x08u)  // Blank Period Control
#define ILI9225_FRAME_CYCLE_CTRL        (0x0Bu)  // Frame Cycle Control
#define ILI9225_INTERFACE_CTRL          (0x0Cu)  // Interface Control
#define ILI9225_OSC_CTRL                (0x0Fu)  // Osc Control
#define ILI9225_POWER_CTRL1             (0x10u)  // Power Control 1
#define ILI9225_POWER_CTRL2             (0x11u)  // Power Control 2
#define ILI9225_POWER_CTRL3             (0x12u)  // Power Control 3
#define ILI9225_POWER_CTRL4             (0x13u)  // Power Control 4
#define ILI9225_POWER_CTRL5             (0x14u)  // Power Control 5
#define ILI9225_VCI_RECYCLING           (0x15u)  // VCI Recycling
#define ILI9225_RAM_ADDR_SET1           (0x20u)  // Horizontal GRAM Address Set
#define ILI9225_RAM_ADDR_SET2           (0x21u)  // Vertical GRAM Address Set
#define ILI9225_GRAM_DATA_REG           (0x22u)  // GRAM Data Register
#define ILI9225_GATE_SCAN_CTRL          (0x30u)  // Gate Scan Control Register
#define ILI9225_VERTICAL_SCROLL_CTRL1   (0x31u)  // Vertical Scroll Control 1 Register
#define ILI9225_VERTICAL_SCROLL_CTRL2   (0x32u)  // Vertical Scroll Control 2 Register
#define ILI9225_VERTICAL_SCROLL_CTRL3   (0x33u)  // Vertical Scroll Control 3 Register
#define ILI9225_PARTIAL_DRIVING_POS1    (0x34u)  // Partial Driving Position 1 Register
#define ILI9225_PARTIAL_DRIVING_POS2    (0x35u)  // Partial Driving Position 2 Register
#define ILI9225_HORIZONTAL_WINDOW_ADDR1 (0x36u)  // Horizontal Address Start Position
#define ILI9225_HORIZONTAL_WINDOW_ADDR2 (0x37u)  // Horizontal Address End Position
#define ILI9225_VERTICAL_WINDOW_ADDR1   (0x38u)  // Vertical Address Start Position
#define ILI9225_VERTICAL_WINDOW_ADDR2   (0x39u)  // Vertical Address End Position
#define ILI9225_GAMMA_CTRL1             (0x50u)  // Gamma Control 1
#define ILI9225_GAMMA_CTRL2             (0x51u)  // Gamma Control 2
#define ILI9225_GAMMA_CTRL3             (0x52u)  // Gamma Control 3
#define ILI9225_GAMMA_CTRL4             (0x53u)  // Gamma Control 4
#define ILI9225_GAMMA_CTRL5             (0x54u)  // Gamma Control 5
#define ILI9225_GAMMA_CTRL6             (0x55u)  // Gamma Control 6
#define ILI9225_GAMMA_CTRL7             (0x56u)  // Gamma Control 7
#define ILI9225_GAMMA_CTRL8             (0x57u)  // Gamma Control 8
#define ILI9225_GAMMA_CTRL9             (0x58u)  // Gamma Control 9
#define ILI9225_GAMMA_CTRL10            (0x59u)  // Gamma Control 10

#define ILI9225C_INVOFF  0x20
#define ILI9225C_INVON   0x21
/********************************************/
#define ILI9225_TFTWIDTH 176  ///< ILI9225 max TFT width
#define ILI9225_TFTHEIGHT 220 ///< ILI9225 max TFT height
// Color definitions
#define ILI9225_BLACK 0x0000       ///<   0,   0,   0
#define ILI9225_NAVY 0x000F        ///<   0,   0, 123
#define ILI9225_DARKGREEN 0x03E0   ///<   0, 125,   0
#define ILI9225_DARKCYAN 0x03EF    ///<   0, 125, 123
#define ILI9225_MAROON 0x7800      ///< 123,   0,   0
#define ILI9225_PURPLE 0x780F      ///< 123,   0, 123
#define ILI9225_OLIVE 0x7BE0       ///< 123, 125,   0
#define ILI9225_LIGHTGREY 0xC618   ///< 198, 195, 198
#define ILI9225_DARKGREY 0x7BEF    ///< 123, 125, 123
#define ILI9225_BLUE 0x001F        ///<   0,   0, 255
#define ILI9225_GREEN 0x07E0       ///<   0, 255,   0
#define ILI9225_CYAN 0x07FF        ///<   0, 255, 255
#define ILI9225_RED 0xF800         ///< 255,   0,   0
#define ILI9225_MAGENTA 0xF81F     ///< 255,   0, 255
#define ILI9225_YELLOW 0xFFE0      ///< 255, 255,   0
#define ILI9225_WHITE 0xFFFF       ///< 255, 255, 255
#define ILI9225_ORANGE 0xFD20      ///< 255, 165,   0
#define ILI9225_GREENYELLOW 0xAFE5 ///< 173, 255,  41
#define ILI9225_PINK 0xFC18        ///< 255, 130, 198

enum autoIncMode_t { R2L_BottomUp, BottomUp_R2L, L2R_BottomUp, BottomUp_L2R, R2L_TopDown, TopDown_R2L, L2R_TopDown, TopDown_L2R };

/**************************************************************************/
/*!
@brief Class to manage hardware interface with ILI9225 chipset (also seems to
work with ILI9340)
*/
/**************************************************************************/

class Adafruit_ILI9225 : public Adafruit_SPITFT {
public:
  Adafruit_ILI9225(int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK, int8_t _RST = -1, int8_t _MISO = -1);
  Adafruit_ILI9225(int8_t _CS, int8_t _DC, int8_t _RST = -1);
  Adafruit_ILI9225(SPIClass *spiClass, int8_t dc, int8_t cs = -1, int8_t rst = -1);
  Adafruit_ILI9225(tftBusWidth busWidth, int8_t d0, int8_t wr, int8_t dc, int8_t cs = -1, int8_t rst = -1, int8_t rd = -1);
  void begin(uint32_t freq = 0);
  void setRotation(uint8_t r);
  void invertDisplay(bool i);
  void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void setDisplay(bool flag);
private:
  // Corresponding modes if orientation changed:
  const autoIncMode_t modeTab [3][8] = {
  //          { R2L_BottomUp, BottomUp_R2L, L2R_BottomUp, BottomUp_L2R, R2L_TopDown,  TopDown_R2L,  L2R_TopDown,  TopDown_L2R }//
  /* 90° */   { BottomUp_L2R, L2R_BottomUp, TopDown_L2R,  L2R_TopDown,  BottomUp_R2L, R2L_BottomUp, TopDown_R2L,  R2L_TopDown },   
  /*180° */   { L2R_TopDown , TopDown_L2R,  R2L_TopDown,  TopDown_R2L,  L2R_BottomUp, BottomUp_L2R, R2L_BottomUp, BottomUp_R2L}, 
  /*270° */   { TopDown_R2L , R2L_TopDown,  BottomUp_R2L, R2L_BottomUp, TopDown_L2R,  L2R_TopDown,  BottomUp_L2R, L2R_BottomUp}
  };
  uint16_t _entry, _invertmode;
  void _writeRegister(uint16_t reg, uint16_t data) { writeCommand(reg); SPI_WRITE16(data); }
  void _orientCoordinates(uint16_t &x1, uint16_t &y1);
  void _swap(uint16_t &a, uint16_t &b);
};

#endif // _ADAFRUIT_ILI9225H
