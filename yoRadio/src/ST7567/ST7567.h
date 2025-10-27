/**********************************************************************************
* This is a library for ST7567 Monochrome LCD Display.
* These displays use I2C or SPI to communicate
*
* This is a free library WITH NO WARRANTY, use it at your own risk!
***********************************************************************************
* This library depends on Adafruit GFX library at
*   https://github.com/adafruit/Adafruit-GFX-Library
*   being present on your system. Please make sure you have installed the latest
*   version before using this library.
***********************************************************************************/

#if defined(ARDUINO_STM32_FEATHER)
typedef class HardwareSPI SPIClass;
#endif

#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>

#if defined(__AVR__)
typedef volatile uint8_t PortReg;
typedef uint8_t PortMask;
#define HAVE_PORTREG
#elif defined(__SAM3X8E__)
typedef volatile RwReg PortReg;
typedef uint32_t PortMask;
#define HAVE_PORTREG
#elif (defined(__arm__) || defined(ARDUINO_FEATHER52)) &&                      \
    !defined(ARDUINO_ARCH_MBED) && !defined(ARDUINO_ARCH_RP2040)
typedef volatile uint32_t PortReg;
typedef uint32_t PortMask;
#define HAVE_PORTREG
#endif

#define LCDWIDTH 128
#define LCDHEIGHT 64

#define BLACK 1
#define WHITE 0
#define INVERSE 2

#define ST7567_I2C_ADDRESS 0x3F

#define CMD_DISPLAY_OFF   0xAE
#define CMD_DISPLAY_ON    0xAF

#define CMD_SET_DISP_START_LINE  0x40
#define CMD_SET_PAGE  0xB0

#define CMD_SET_COLUMN_UPPER  0x10
#define CMD_SET_COLUMN_LOWER  0x00

#define CMD_SEG_DIRECTION_NORMAL 0xA0
#define CMD_SEG_DIRECTION_REVERSE 0xA1

#define CMD_SET_DISP_NORMAL 0xA6
#define CMD_SET_DISP_REVERSE 0xA7

#define CMD_SET_ALLPTS_NORMAL 0xA4
#define CMD_SET_ALLPTS_ON  0xA5
#define CMD_SET_BIAS_9 0xA2
#define CMD_SET_BIAS_7 0xA3

#define CMD_RMW  0xE0 // Read-modify-Write (column address increment: Read:+0, Write:+1)
#define CMD_RMW_CLEAR 0xEE // END Read-modify-Write
#define CMD_INTERNAL_RESET  0xE2
#define CMD_SET_COM_NORMAL  0xC0
#define CMD_SET_COM_REVERSE  0xC8
#define CMD_SET_POWER_CONTROL  0x28
#define CMD_SET_RESISTOR_RATIO  0x20 // regulation ratio RR[2:0]
#define CMD_SET_VOLUME_FIRST  0x81
#define CMD_SET_VOLUME_SECOND  0 // EV[5:0]
#define CMD_NOP  0xE3

/*! The controller object for ST7567 displays */
class ST7567 : public Adafruit_GFX {
public:
  ST7567(uint8_t w, uint8_t h, TwoWire *twi = &Wire,
         int8_t rst_pin = -1, uint32_t clkDuring = 400000UL,
         uint32_t clkAfter = 100000UL);
  ST7567(uint8_t w, uint8_t h, int8_t mosi_pin, int8_t sclk_pin,
         int8_t dc_pin, int8_t rst_pin, int8_t cs_pin);
  ST7567(uint8_t w, uint8_t h, SPIClass *spi_ptr, int8_t dc_pin,
         int8_t rst_pin, int8_t cs_pin, uint32_t bitrate = 4000000UL);
  ~ST7567(void);

  bool begin(uint8_t i2caddr = ST7567_I2C_ADDRESS, bool reset = true, bool periphBegin = true);
  void display(void);
  void clearDisplay(void);
  void invertDisplay(bool i);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void st7567_command(uint8_t c);
  bool getPixel(int16_t x, int16_t y);
  uint8_t *getBuffer(void);

  void setContrast(uint8_t val);
  uint8_t getContrast(void);

  void sleep(void);
  void wake(void);

protected:
  inline void SPIwrite(uint8_t d) __attribute__((always_inline));
  void st7567_command1(uint8_t c);
  void st7567_commandList(const uint8_t *c, uint8_t n);

  void updateBoundingBox(uint8_t xmin, uint8_t ymin, uint8_t xmax,
                         uint8_t ymax);
  uint8_t xUpdateMin, xUpdateMax, yUpdateMin, yUpdateMax;

  SPIClass *spi;   ///< Initialized during construction when using SPI. See
                   ///< SPI.cpp, SPI.h
  TwoWire *wire;   ///< Initialized during construction when using I2C. See
                   ///< Wire.cpp, Wire.h
  uint8_t *buffer; ///< Buffer data used for display buffer. Allocated when
                   ///< begin method is called.

  int8_t i2caddr;  ///< I2C address initialized when begin method is called.
  int8_t mosiPin;  ///< (Master Out Slave In) set when using SPI set during
                   ///< construction.
  int8_t clkPin;   ///< (Clock Pin) set when using SPI set during construction.
  int8_t dcPin;    ///< (Data Pin) set when using SPI set during construction.
  int8_t
      csPin; ///< (Chip Select Pin) set when using SPI set during construction.
  int8_t rstPin; ///< Display reset pin assignment. Set during construction.

#ifdef HAVE_PORTREG
  PortReg *mosiPort, *clkPort, *dcPort, *csPort;
  PortMask mosiPinMask, clkPinMask, dcPinMask, csPinMask;
#endif
#if ARDUINO >= 157
  uint32_t wireClk;    ///< Wire speed for ST7567 transfers
  uint32_t restoreClk; ///< Wire speed following ST7567 transfers
#endif
  uint8_t _contrast = 0x30; ///< contrast setting (0x00~0x3F)
#if defined(SPI_HAS_TRANSACTION)
protected:
  // Allow sub-class to change
  SPISettings spiSettings;
#endif
};
