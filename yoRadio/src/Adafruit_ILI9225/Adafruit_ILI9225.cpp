#include "../core/options.h"
#if DSP_MODEL==DSP_ILI9225

#include "Adafruit_ILI9225.h"

#define SPI_DEFAULT_FREQ 24000000

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit ILI9225 driver with software SPI
    @param    cs    Chip select pin #
    @param    dc    Data/Command pin #
    @param    mosi  SPI MOSI pin #
    @param    sclk  SPI Clock pin #
    @param    rst   Reset pin # (optional, pass -1 if unused)
    @param    miso  SPI MISO pin # (optional, pass -1 if unused)
*/
/**************************************************************************/
Adafruit_ILI9225::Adafruit_ILI9225(int8_t cs, int8_t dc, int8_t mosi,
                                   int8_t sclk, int8_t rst, int8_t miso)
    : Adafruit_SPITFT(ILI9225_TFTWIDTH, ILI9225_TFTHEIGHT, cs, dc, mosi, sclk,
                      rst, miso) {}

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit ILI9225 driver with hardware SPI using the
            default SPI peripheral.
    @param  cs   Chip select pin # (OK to pass -1 if CS tied to GND).
    @param  dc   Data/Command pin # (required).
    @param  rst  Reset pin # (optional, pass -1 if unused).
*/
/**************************************************************************/
Adafruit_ILI9225::Adafruit_ILI9225(int8_t cs, int8_t dc, int8_t rst)
    : Adafruit_SPITFT(ILI9225_TFTWIDTH, ILI9225_TFTHEIGHT, cs, dc, rst) {}

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit ILI9225 driver with hardware SPI using
            a specific SPI peripheral (not necessarily default).
    @param  spiClass  Pointer to SPI peripheral (e.g. &SPI or &SPI1).
    @param  dc        Data/Command pin # (required).
    @param  cs        Chip select pin # (optional, pass -1 if unused and
                      CS is tied to GND).
    @param  rst       Reset pin # (optional, pass -1 if unused).
*/
/**************************************************************************/
Adafruit_ILI9225::Adafruit_ILI9225(SPIClass *spiClass, int8_t dc, int8_t cs,
                                   int8_t rst)
    : Adafruit_SPITFT(ILI9225_TFTWIDTH, ILI9225_TFTHEIGHT, spiClass, cs, dc,
                      rst) {}

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit ILI9225 driver using parallel interface.
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
Adafruit_ILI9225::Adafruit_ILI9225(tftBusWidth busWidth, int8_t d0, int8_t wr,
                                   int8_t dc, int8_t cs, int8_t rst, int8_t rd)
    : Adafruit_SPITFT(ILI9225_TFTWIDTH, ILI9225_TFTHEIGHT, busWidth, d0, wr, dc,
                      cs, rst, rd) {}

static const uint8_t PROGMEM initcmd[] = {
  ILI9225_POWER_CTRL1, 0x00, 0x00,
  ILI9225_POWER_CTRL2, 0x00, 0x00,
  ILI9225_POWER_CTRL3, 0x00, 0x00,
  ILI9225_POWER_CTRL4, 0x00, 0x00,
  ILI9225_POWER_CTRL5, 0x00, 0x00,
  ILI9225_DELAY, 40,
  ILI9225_POWER_CTRL2, 0x00, 0x18,
  ILI9225_POWER_CTRL3, 0x61, 0x21,
  ILI9225_POWER_CTRL4, 0x00, 0x6F,
  ILI9225_POWER_CTRL5, 0x49, 0x5F,
  ILI9225_POWER_CTRL1, 0x08, 0x00,
  ILI9225_DELAY, 10,
  ILI9225_POWER_CTRL2, 0x10, 0x3B,
  ILI9225_DELAY, 50,
  ILI9225_DRIVER_OUTPUT_CTRL, 0x01, 0x1C,
  ILI9225_LCD_AC_DRIVING_CTRL, 0x01, 0x00,
  ILI9225_ENTRY_MODE, 0x10, 0x38,
  ILI9225_DISP_CTRL1, 0x00, 0x00,
  ILI9225_BLANK_PERIOD_CTRL1, 0x08, 0x08,
  ILI9225_FRAME_CYCLE_CTRL, 0x11, 0x00,
  ILI9225_INTERFACE_CTRL, 0x00, 0x00,
  ILI9225_OSC_CTRL, 0x0D, 0x01,
  ILI9225_VCI_RECYCLING, 0x00, 0x20,
  ILI9225_RAM_ADDR_SET1, 0x00, 0x00,
  ILI9225_RAM_ADDR_SET2, 0x00, 0x00,
  ILI9225_GATE_SCAN_CTRL, 0x00, 0x00,
  ILI9225_VERTICAL_SCROLL_CTRL1, 0x00, 0xDB,
  ILI9225_VERTICAL_SCROLL_CTRL2, 0x00, 0x00,
  ILI9225_VERTICAL_SCROLL_CTRL3, 0x00, 0x00,
  ILI9225_PARTIAL_DRIVING_POS1, 0x00, 0xDB,
  ILI9225_PARTIAL_DRIVING_POS2, 0x00, 0x00,
  ILI9225_HORIZONTAL_WINDOW_ADDR1, 0x00, 0xAF,
  ILI9225_HORIZONTAL_WINDOW_ADDR2, 0x00, 0x00,
  ILI9225_VERTICAL_WINDOW_ADDR1, 0x00, 0xDB,
  ILI9225_VERTICAL_WINDOW_ADDR2, 0x00, 0x00,
  ILI9225_GAMMA_CTRL1, 0x00, 0x00,
  ILI9225_GAMMA_CTRL2, 0x08, 0x08,
  ILI9225_GAMMA_CTRL3, 0x08, 0x0A,
  ILI9225_GAMMA_CTRL4, 0x00, 0x0A,
  ILI9225_GAMMA_CTRL5, 0x0A, 0x08,
  ILI9225_GAMMA_CTRL6, 0x08, 0x08,
  ILI9225_GAMMA_CTRL7, 0x00, 0x00,
  ILI9225_GAMMA_CTRL8, 0x0A, 0x00,
  ILI9225_GAMMA_CTRL9, 0x07, 0x10,
  ILI9225_GAMMA_CTRL10, 0x07, 0x10,
  ILI9225_DISP_CTRL1, 0x00, 0x12,
  ILI9225_DELAY, 50,
  ILI9225_DISP_CTRL1, 0x10, 0x17,
  0x00 // the end
};

/**************************************************************************/
/*!
    @brief   Initialize ILI9225 chip
    Connects to the ILI9225 over SPI and sends initialization procedure commands
    @param    freq  Desired SPI clock frequency
*/
/**************************************************************************/
void Adafruit_ILI9225::begin(uint32_t freq) {

  if (!freq)
    freq = SPI_DEFAULT_FREQ;
  initSPI(freq);
  SPI_CS_HIGH();
  uint8_t cmd;
  const uint8_t *addr = initcmd;
  
  while ((cmd = pgm_read_byte(addr++)) > 0) {
    if (cmd == ILI9225_DELAY) {
      delay(pgm_read_byte(addr++));
    } else {
      uint8_t hi = pgm_read_byte(addr++);
      uint8_t lo = pgm_read_byte(addr++);
      uint8_t data[2] = { hi, lo };
      sendCommand(cmd, data, 2);
    }
  }
  _width = ILI9225_TFTWIDTH;
  _height = ILI9225_TFTHEIGHT;
}

/**************************************************************************/
/*!
    @brief   Set origin of (0,0) and orientation of TFT display
    @param   m  The index for rotation, from 0-3 inclusive
*/
/**************************************************************************/
void Adafruit_ILI9225::setRotation(uint8_t m) {
  rotation = m % 4; // can't be higher than 3
  switch (rotation) {
  case 0:    
    _width = ILI9225_TFTWIDTH;
    _height = ILI9225_TFTHEIGHT;
    _entry = 0x1030;
    break;
  case 1:
    _width = ILI9225_TFTHEIGHT;
    _height = ILI9225_TFTWIDTH;
    _entry = 0x1028;
    break;
  case 2:
    _width = ILI9225_TFTWIDTH;
    _height = ILI9225_TFTHEIGHT;
    _entry = 0x1000;
    break;
  case 3:
    _width = ILI9225_TFTHEIGHT;
    _height = ILI9225_TFTWIDTH;
    _entry = 0x1038;
    break;
  }
}

void Adafruit_ILI9225::_swap(uint16_t &a, uint16_t &b) {
  uint16_t w = a;
  a = b;
  b = w;
}

void Adafruit_ILI9225::_orientCoordinates(uint16_t &x1, uint16_t &y1) {
  switch (rotation) {
    case 0:
        break;
    case 1:
        y1 = _height - y1 - 1;
        _swap(x1, y1);
        break;
    case 2:
        x1 = _width - x1 - 1;
        y1 = _height - y1 - 1;
        break;
    case 3:
        x1 = _width - x1 - 1;
        _swap(x1, y1);
        break;
  }
}

void Adafruit_ILI9225::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  uint16_t x0=x, x1=x + w - 1, y0=y, y1=y + h - 1;
  x0 = min( x0, (uint16_t) (_width-1) );
  x1 = min( x1, (uint16_t) (_width-1) );
  y0 = min( y0, (uint16_t) (_height-1) );
  y1 = min( y1, (uint16_t) (_height-1) );
  _orientCoordinates(x0, y0);
  _orientCoordinates(x1, y1);

  if (x1<x0) _swap(x0, x1);
  if (y1<y0) _swap(y0, y1);

  autoIncMode_t mode = L2R_TopDown;
  if ( rotation > 0 ) mode = modeTab[rotation-1][mode];
  _writeRegister(ILI9225_ENTRY_MODE, 0x1000 | ( mode<<3) );
  _writeRegister(ILI9225_HORIZONTAL_WINDOW_ADDR1,x1);
  _writeRegister(ILI9225_HORIZONTAL_WINDOW_ADDR2,x0);
  _writeRegister(ILI9225_VERTICAL_WINDOW_ADDR1,y1);
  _writeRegister(ILI9225_VERTICAL_WINDOW_ADDR2,y0);
  switch ( mode>>1 ) {
    case 0:
      _writeRegister(ILI9225_RAM_ADDR_SET1,x1);
      _writeRegister(ILI9225_RAM_ADDR_SET2,y1);
      break;
    case 1:
      _writeRegister(ILI9225_RAM_ADDR_SET1,x0);
      _writeRegister(ILI9225_RAM_ADDR_SET2,y1);
      break;
    case 2:
      _writeRegister(ILI9225_RAM_ADDR_SET1,x1);
      _writeRegister(ILI9225_RAM_ADDR_SET2,y0);
      break;
    case 3:
      _writeRegister(ILI9225_RAM_ADDR_SET1,x0);
      _writeRegister(ILI9225_RAM_ADDR_SET2,y0);
      break;
  }
  writeCommand(ILI9225_GRAM_DATA_REG);
}

void Adafruit_ILI9225::invertDisplay(bool invert) {
  _invertmode = invert?0x1013:0x1017;
  startWrite();
  _writeRegister(ILI9225_DISP_CTRL1, _invertmode);
  endWrite();
}

void Adafruit_ILI9225::setDisplay(bool flag) {
  startWrite();
  if (flag) {
    _writeRegister(ILI9225_POWER_CTRL1, 0x0000);
    delay(50);
    _writeRegister(ILI9225_DISP_CTRL1, _invertmode);
  } else {
    _writeRegister(ILI9225_DISP_CTRL1, 0x0000);
    delay(50);
    _writeRegister(ILI9225_POWER_CTRL1, 0x0003);
  }
  endWrite();
}

#endif
