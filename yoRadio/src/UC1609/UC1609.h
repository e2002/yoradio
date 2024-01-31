/**********************************************************************************
* This is a library for UC1609 Monochrome LCD Display.
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

#define LCDWIDTH 192
#define LCDHEIGHT 64

#define BLACK 1
#define WHITE 0
#define INVERSE 2

#define UC1609_I2C_ADDRESS 0x3C // 0x3E

// UC1909 Write registers
#define UC1609_SYSTEM_RESET 0xE2 /**< System Reset */

#define UC1609_POWER_CONTROL 0x28 /**< Power control Address */
#define UC1609_PC_SET 0x06 /**< PC[2:0] 110, Internal V LCD (7x charge pump) + 10b: 1.4mA */

#define UC1609_ADDRESS_CONTROL 0x88 /**< set RAM address control */
#define UC1609_ADDRESS_SET 0x01 /**< Set AC [2:0] Program registers  for RAM address control */
#define UC1609_SET_COM_END 0xF1 /**< Set CEN[5:0] Program the ending COM electrode */

#define UC1609_SET_PAGEADD 0xB0 /**< Page address Set PA[3:0]  */
#define UC1609_SET_COLADD_LSB 0x00 /**< Column Address Set CA [3:0] */
#define UC1609_SET_COLADD_MSB 0x10 /**< Column Address Set CA [7:4] */

#define UC1609_TEMP_COMP_REG 0x27 /**< Temperature Compensation Register */
#define UC1609_TEMP_COMP_SET 0x00 /**< TC[1:0] = 00b= -0.00%/ C */

#define UC1609_FRAMERATE_REG 0xA0 /**< Frame rate register */
#define UC1609_FRAMERATE_SET 0x01  /**< Set Frame Rate LC [4:3] 01b: 95 fps */

#define UC1609_BIAS_RATIO 0xE8 /**< Bias Ratio. The ratio between V-LCD and V-D . */
#define UC1609_BIAS_RATIO_SET 0x03 /**<  Set BR[1:0] = 11 (set to 9 default, 11b = 9) */

#define UC1609_GN_PM 0x81 /**< Set V BIAS Potentiometer to fine tune V-D and V-LCD  (double-byte command) */
#define UC1609_DEFAULT_GN_PM 0x88 /**< default only used if user does not specify Vbias */

#define UC1609_LCD_CONTROL 0xC0 /**< Rotate map control */
#define UC1609_DISPLAY_ON 0xAE /**< enables display */
#define UC1609_ALL_PIXEL_ON 0xA4 /**< sets on all Pixels on */
#define UC1609_INVERSE_DISPLAY 0xA6 /**< inverts display */
#define UC1609_SCROLL 0x40 /**< scrolls , Set the scroll line number. 0-64 */

// Rotate
#define UC1609_ROTATION_NORMAL 0x04
#define UC1609_ROTATION_FLIP 0x02
#define UC1609_ROTATION_DEFAULT 0x00

/*! The controller object for UC1609 displays */
class UC1609 : public Adafruit_GFX {
public:
  UC1609(uint8_t w, uint8_t h, TwoWire *twi = &Wire,
         int8_t rst_pin = -1, uint32_t clkDuring = 400000UL,
         uint32_t clkAfter = 100000UL);
  UC1609(uint8_t w, uint8_t h, int8_t mosi_pin, int8_t sclk_pin,
         int8_t dc_pin, int8_t rst_pin, int8_t cs_pin);
  UC1609(uint8_t w, uint8_t h, SPIClass *spi_ptr, int8_t dc_pin,
         int8_t rst_pin, int8_t cs_pin, uint32_t bitrate = 8000000UL);
  ~UC1609(void);

  bool begin(uint8_t i2caddr = UC1609_I2C_ADDRESS, bool reset = true, bool periphBegin = true);
  void display(void);
  void clearDisplay(void);
  void invertDisplay(bool i);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void uc1609_command(uint8_t c);
  bool getPixel(int16_t x, int16_t y);
  uint8_t *getBuffer(void);

  void setContrast(uint8_t val);
  void sleep(void);
  void wake(void);

protected:
  inline void SPIwrite(uint8_t d) __attribute__((always_inline));
  void uc1609_command1(uint8_t c);
  void uc1609_commandList(const uint8_t *c, uint8_t n);

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
  uint32_t wireClk;    ///< Wire speed for UC1609 transfers
  uint32_t restoreClk; ///< Wire speed following UC1609 transfers
#endif
  uint8_t _contrast = UC1609_DEFAULT_GN_PM; ///< contrast setting (0x00~0xFF)
#if defined(SPI_HAS_TRANSACTION)
protected:
  // Allow sub-class to change
  SPISettings spiSettings;
#endif
};
