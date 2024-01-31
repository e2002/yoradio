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

#ifdef __AVR__
#include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
#include <pgmspace.h>
#else
#define pgm_read_byte(addr)                                                    \
  (*(const unsigned char *)(addr)) ///< PROGMEM workaround for non-AVR
#endif

#if !defined(__ARM_ARCH) && !defined(ENERGIA) && !defined(ESP8266) &&          \
    !defined(ESP32) && !defined(__arc__)
#include <util/delay.h>
#endif

#include "UC1609.h"
#include <Adafruit_GFX.h>
#include "../core/spidog.h"

// SOME DEFINES AND STATIC VARIABLES USED INTERNALLY -----------------------

#if defined(I2C_BUFFER_LENGTH)
#define WIRE_MAX min(256, I2C_BUFFER_LENGTH) ///< Particle or similar Wire lib
#elif defined(BUFFER_LENGTH)
#define WIRE_MAX min(256, BUFFER_LENGTH) ///< AVR or similar Wire lib
#elif defined(SERIAL_BUFFER_SIZE)
#define WIRE_MAX                                                               \
  min(255, SERIAL_BUFFER_SIZE - 1) ///< Newer Wire uses RingBuffer
#else
#define WIRE_MAX 32 ///< Use common Arduino core default
#endif

#define uc1609_swap(a, b)                                                     \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation

#if ARDUINO >= 100
#define WIRE_WRITE wire->write ///< Wire write function in recent Arduino lib
#else
#define WIRE_WRITE wire->send ///< Wire write function in older Arduino lib
#endif

#ifdef HAVE_PORTREG
#define UC1609_SELECT *csPort &= ~csPinMask;       ///< Device select
#define UC1609_DESELECT *csPort |= csPinMask;      ///< Device deselect
#define UC1609_MODE_COMMAND *dcPort &= ~dcPinMask; ///< Command mode
#define UC1609_MODE_DATA *dcPort |= dcPinMask;     ///< Data mode
#else
#define UC1609_SELECT digitalWrite(csPin, LOW);       ///< Device select
#define UC1609_DESELECT digitalWrite(csPin, HIGH);    ///< Device deselect
#define UC1609_MODE_COMMAND digitalWrite(dcPin, LOW); ///< Command mode
#define UC1609_MODE_DATA digitalWrite(dcPin, HIGH);   ///< Data mode
#endif

#if (ARDUINO >= 157) && !defined(ARDUINO_STM32_FEATHER)
#define SETWIRECLOCK wire->setClock(wireClk)    ///< Set before I2C transfer
#define RESWIRECLOCK wire->setClock(restoreClk) ///< Restore after I2C xfer
#else // setClock() is not present in older Arduino Wire lib (or WICED)
#define SETWIRECLOCK ///< Dummy stand-in define
#define RESWIRECLOCK ///< keeps compiler happy
#endif

#if defined(SPI_HAS_TRANSACTION)
#define TAKE_MUTEX() sdog.takeMutex()
#define GIVE_MUTEX() sdog.giveMutex()
#define SPI_TRANSACTION_START TAKE_MUTEX(); spi->beginTransaction(spiSettings) ///< Pre-SPI
#define SPI_TRANSACTION_END spi->endTransaction(); GIVE_MUTEX()                ///< Post-SPI
#else // SPI transactions likewise not present in older Arduino SPI lib
#define SPI_TRANSACTION_START ///< Dummy stand-in define
#define SPI_TRANSACTION_END   ///< keeps compiler happy
#endif

// The definition of 'transaction' is broadened a bit in the context of
// this library -- referring not just to SPI transactions (if supported
// in the version of the SPI library being used), but also chip select
// (if SPI is being used, whether hardware or soft), and also to the
// beginning and end of I2C transfers (the Wire clock may be sped up before
// issuing data to the display, then restored to the default rate afterward
// so other I2C device types still work).  All of these are encapsulated
// in the TRANSACTION_* macros.

// Check first if Wire, then hardware SPI, then soft SPI:
#define TRANSACTION_START                                                      \
  if (wire) {                                                                  \
    SETWIRECLOCK;                                                              \
  } else {                                                                     \
    if (spi) {                                                                 \
      SPI_TRANSACTION_START;                                                   \
    }                                                                          \
    UC1609_SELECT;                                                             \
  } ///< Wire, SPI or bitbang transfer setup
#define TRANSACTION_END                                                        \
  if (wire) {                                                                  \
    RESWIRECLOCK;                                                              \
  } else {                                                                     \
    UC1609_DESELECT;                                                           \
    if (spi) {                                                                 \
      SPI_TRANSACTION_END;                                                     \
    }                                                                          \
  } ///< Wire, SPI or bitbang transfer end


// CONSTRUCTORS, DESTRUCTOR ------------------------------------------------

/*!
    @brief  Constructor for I2C-interfaced UC1609 displays.
    @param  w
            Display width in pixels
    @param  h
            Display height in pixels
    @param  twi
            Pointer to an existing TwoWire instance (e.g. &Wire, the
            microcontroller's primary I2C bus).
    @param  rst_pin
            Reset pin (using Arduino pin numbering), or -1 if not used
            (some displays might be wired to share the microcontroller's
            reset pin).
    @param  clkDuring
            Speed (in Hz) for Wire transmissions in UC1609 library calls.
            Defaults to 400000 (400 KHz), a known 'safe' value for most
            microcontrollers, and meets the UC1609 datasheet spec.
            Some systems can operate I2C faster (800 KHz for ESP32, 1 MHz
            for many other 32-bit MCUs), and some (perhaps not all)
            UC1609's can work with this -- so it's optionally be specified
            here and is not a default behavior. (Ignored if using pre-1.5.7
            Arduino software, which operates I2C at a fixed 100 KHz.)
    @param  clkAfter
            Speed (in Hz) for Wire transmissions following UC1609 library
            calls. Defaults to 100000 (100 KHz), the default Arduino Wire
            speed. This is done rather than leaving it at the 'during' speed
            because other devices on the I2C bus might not be compatible
            with the faster rate. (Ignored if using pre-1.5.7 Arduino
            software, which operates I2C at a fixed 100 KHz.)
    @note   Call the object's begin() function before use -- buffer
            allocation is performed there!
*/
UC1609::UC1609(uint8_t w, uint8_t h, TwoWire *twi,
               int8_t rst_pin, uint32_t clkDuring,
               uint32_t clkAfter)
    : Adafruit_GFX(w, h), spi(NULL), wire(twi ? twi : &Wire), buffer(NULL),
      mosiPin(-1), clkPin(-1), dcPin(-1), csPin(-1), rstPin(rst_pin)
#if ARDUINO >= 157
      ,
      wireClk(clkDuring), restoreClk(clkAfter)
#endif
{
}

/*!
    @brief  Constructor for SPI UC1609 displays, using software (bitbang)
            SPI.
    @param  w
            Display width in pixels
    @param  h
            Display height in pixels
    @param  mosi_pin
            MOSI (master out, slave in) pin (using Arduino pin numbering).
            This transfers serial data from microcontroller to display.
    @param  sclk_pin
            SCLK (serial clock) pin (using Arduino pin numbering).
            This clocks each bit from MOSI.
    @param  dc_pin
            Data/command pin (using Arduino pin numbering), selects whether
            display is receiving commands (low) or data (high).
    @param  rst_pin
            Reset pin (using Arduino pin numbering), or -1 if not used
            (some displays might be wired to share the microcontroller's
            reset pin).
    @param  cs_pin
            Chip-select pin (using Arduino pin numbering) for sharing the
            bus with other devices. Active low.
    @note   Call the object's begin() function before use -- buffer
            allocation is performed there!
*/
UC1609::UC1609(uint8_t w, uint8_t h, int8_t mosi_pin,
               int8_t sclk_pin, int8_t dc_pin,
               int8_t rst_pin, int8_t cs_pin)
    : Adafruit_GFX(w, h), spi(NULL), wire(NULL), buffer(NULL),
      mosiPin(mosi_pin), clkPin(sclk_pin), dcPin(dc_pin), csPin(cs_pin),
      rstPin(rst_pin) 
{
}

/*!
    @brief  Constructor for SPI UC1609 displays, using native hardware SPI.
    @param  w
            Display width in pixels
    @param  h
            Display height in pixels
    @param  spi_ptr
            Pointer to an existing SPIClass instance (e.g. &SPI, the
            microcontroller's primary SPI bus).
    @param  dc_pin
            Data/command pin (using Arduino pin numbering), selects whether
            display is receiving commands (low) or data (high).
    @param  rst_pin
            Reset pin (using Arduino pin numbering), or -1 if not used
            (some displays might be wired to share the microcontroller's
            reset pin).
    @param  cs_pin
            Chip-select pin (using Arduino pin numbering) for sharing the
            bus with other devices. Active low.
    @param  bitrate
            SPI clock rate for transfers to this display. Default if
            unspecified is 8000000UL (8 MHz).
    @note   Call the object's begin() function before use -- buffer
            allocation is performed there!
*/
UC1609::UC1609(uint8_t w, uint8_t h, SPIClass *spi_ptr,
               int8_t dc_pin, int8_t rst_pin, int8_t cs_pin,
               uint32_t bitrate)
    : Adafruit_GFX(w, h), spi(spi_ptr ? spi_ptr : &SPI), wire(NULL),
      buffer(NULL), mosiPin(-1), clkPin(-1), dcPin(dc_pin), csPin(cs_pin),
      rstPin(rst_pin) {
#ifdef SPI_HAS_TRANSACTION
  spiSettings = SPISettings(bitrate, MSBFIRST, SPI_MODE0);
#endif
}

/*!
    @brief  Destructor for UC1609 object.
*/
UC1609::~UC1609(void) {
  if (buffer) {
    free(buffer);
    buffer = NULL;
  }
}

// LOW-LEVEL UTILS ---------------------------------------------------------

// Issue single byte out SPI, either soft or hardware as appropriate.
// SPI transaction/selection must be performed in calling function.
/*!
    @brief  Write a single byte to the SPI port.

    @param  d
                        Data byte to be written.

    @return void
*/
inline void UC1609::SPIwrite(uint8_t d) {
  if (spi) {
    (void)spi->transfer(d);
  } else {
    for (uint8_t bit = 0x80; bit; bit >>= 1) {
#ifdef HAVE_PORTREG
      if (d & bit)
        *mosiPort |= mosiPinMask;
      else
        *mosiPort &= ~mosiPinMask;
      *clkPort |= clkPinMask;  // Clock high
      *clkPort &= ~clkPinMask; // Clock low
#else
      digitalWrite(mosiPin, d & bit);
      digitalWrite(clkPin, HIGH);
      digitalWrite(clkPin, LOW);
#endif
    }
  }
}

/*!
    @brief Issue single command to UC1609, using I2C or hard/soft SPI as
   needed. Because command calls are often grouped, SPI transaction and
   selection must be started/ended in calling function for efficiency. This is a
   protected function, not exposed (see ssd1306_command() instead).

        @param c
                   the command character to send to the display.
                   Refer to UC1609 data sheet for commands
    @return None (void).
    @note
*/
void UC1609::uc1609_command1(uint8_t c) {
  if (wire) { // I2C
    wire->beginTransmission(i2caddr & 0xFE); // CD = 0
    WIRE_WRITE(c);
    wire->endTransmission();
  } else { // SPI (hw or soft) -- transaction started in calling function
    UC1609_MODE_COMMAND
    SPIwrite(c);
  }
}

/*!
    @brief Issue list of commands to UC1609, same rules as above re:
   transactions. This is a protected function, not exposed.
        @param c
                   pointer to list of commands

        @param n
                   number of commands in the list

    @return None (void).
    @note
*/
void UC1609::uc1609_commandList(const uint8_t *c, uint8_t n) {
  if (wire) { // I2C
    wire->beginTransmission(i2caddr & 0xFE); // CD = 0
    uint16_t bytesOut = 0;
    while (n--) {
      if (bytesOut >= WIRE_MAX) {
        wire->endTransmission();
        wire->beginTransmission(i2caddr & 0xFE); // CD = 0
        bytesOut = 0;
      }
      WIRE_WRITE(pgm_read_byte(c++));
      bytesOut++;
    }
    wire->endTransmission();
  } else { // SPI -- transaction started in calling function
    UC1609_MODE_COMMAND
    while (n--)
      SPIwrite(pgm_read_byte(c++));
  }
}

// A public version of uc1609_command1(), for existing user code that
// might rely on that function. This encapsulates the command transfer
// in a transaction start/end, similar to old library's handling of it.
/*!
    @brief  Issue a single low-level command directly to the UC1609
            display, bypassing the library.
    @param  c
            Command to issue (0x00 to 0xFF, see datasheet).
    @return None (void).
*/
void UC1609::uc1609_command(uint8_t c) {
  TRANSACTION_START
  uc1609_command1(c);
  TRANSACTION_END
}

// ALLOCATE & INIT DISPLAY -------------------------------------------------

/*!
    @brief  Allocate RAM for image buffer, initialize peripherals and pins.
    @param  addr
            I2C address of corresponding UC1609 display.
            SPI displays (hardware or software) do not use addresses, but
            this argument is still required (pass 0 or any value really,
            it will simply be ignored). Default if unspecified is 0.
    @param  reset
            If true, and if the reset pin passed to the constructor is
            valid, a hard reset will be performed before initializing the
            display. If using multiple UC1609 displays on the same bus, and
            if they all share the same reset pin, you should only pass true
            on the first display being initialized, false on all others,
            else the already-initialized displays would be reset. Default if
            unspecified is true.
    @return true on successful allocation/init, false otherwise.
            Well-behaved code should check the return value before
            proceeding.
    @note   MUST call this function before any drawing or updates!
*/
bool UC1609::begin(uint8_t addr, bool reset, bool periphBegin) {

  if ((!buffer) && !(buffer = (uint8_t *)malloc(WIDTH * ((HEIGHT + 7) / 8))))
    return false;

// clear display and set up a bounding box for screen updates
  clearDisplay();

  // Setup pin directions
  if (wire) { // Using I2C
    // If I2C address is unspecified, use default (0x3C)
    i2caddr = addr ? addr : UC1609_I2C_ADDRESS;
    // TwoWire begin() function might be already performed by the calling
    // function if it has unusual circumstances (e.g. TWI variants that
    // can accept different SDA/SCL pins, or if two UC1609 instances
    // with different addresses -- only a single begin() is needed).
    if (periphBegin)
      wire->begin();
  } else { // Using one of the SPI modes, either soft or hardware
    pinMode(dcPin, OUTPUT); // Set data/command pin as output
    pinMode(csPin, OUTPUT); // Same for chip select
#ifdef HAVE_PORTREG
    dcPort = (PortReg *)portOutputRegister(digitalPinToPort(dcPin));
    dcPinMask = digitalPinToBitMask(dcPin);
    csPort = (PortReg *)portOutputRegister(digitalPinToPort(csPin));
    csPinMask = digitalPinToBitMask(csPin);
#endif
    UC1609_DESELECT
    if (spi) { // Hardware SPI
      // SPI peripheral begin same as wire check above.
      if (periphBegin)
        spi->begin();
    } else {                    // Soft SPI
      pinMode(mosiPin, OUTPUT); // MOSI and SCLK outputs
      pinMode(clkPin, OUTPUT);
#ifdef HAVE_PORTREG
      mosiPort = (PortReg *)portOutputRegister(digitalPinToPort(mosiPin));
      mosiPinMask = digitalPinToBitMask(mosiPin);
      clkPort = (PortReg *)portOutputRegister(digitalPinToPort(clkPin));
      clkPinMask = digitalPinToBitMask(clkPin);
      *clkPort &= ~clkPinMask; // Clock low
#else
      digitalWrite(clkPin, LOW); // Clock low
#endif
    }
  }

  // Reset UC1609 if requested and reset pin specified in constructor
  if (reset && (rstPin >= 0)) {
    pinMode(rstPin, OUTPUT);
    digitalWrite(rstPin, HIGH);
    delay(1);                   // VDD goes high at start, pause for 1 ms
    digitalWrite(rstPin, LOW);  // Bring reset low
    delay(1);                  // Wait 1 ms
    digitalWrite(rstPin, HIGH); // Bring out of reset
    delay(10);                  // Wait 10 ms
  }

  TRANSACTION_START

  // Init sequence
  static const uint8_t PROGMEM init1[] = {UC1609_SET_COM_END,
                                          LCDHEIGHT - 1,
                                          UC1609_TEMP_COMP_REG | UC1609_TEMP_COMP_SET,
                                          UC1609_LCD_CONTROL | UC1609_ROTATION_DEFAULT,
                                          UC1609_SCROLL | 0,
                                          UC1609_POWER_CONTROL | UC1609_PC_SET,
                                          UC1609_BIAS_RATIO | UC1609_BIAS_RATIO_SET,
                                          UC1609_GN_PM,
                                          UC1609_DEFAULT_GN_PM,
                                          UC1609_ADDRESS_CONTROL | UC1609_ADDRESS_SET,
                                          UC1609_FRAMERATE_REG | UC1609_FRAMERATE_SET,
                                          UC1609_LCD_CONTROL | UC1609_ROTATION_NORMAL,
                                          UC1609_INVERSE_DISPLAY | 0,
                                          UC1609_ALL_PIXEL_ON | 0};
  uc1609_commandList(init1, sizeof(init1));

  // display on
  uc1609_command1(UC1609_DISPLAY_ON | 1);
  TRANSACTION_END

  return true; // Success
}

// DRAWING FUNCTIONS -------------------------------------------------------

/*!
  @brief Update the bounding box for partial updates
  @param xmin left
  @param ymin top
  @param xmax right
  @param ymax bottom
 */
void UC1609::updateBoundingBox(uint8_t xmin, uint8_t ymin,
                               uint8_t xmax, uint8_t ymax) {
  xUpdateMin = min(xUpdateMin, xmin);
  xUpdateMax = max(xUpdateMax, xmax);
  yUpdateMin = min(yUpdateMin, ymin);
  yUpdateMax = max(yUpdateMax, ymax);
}

/*!
    @brief  Set/clear/invert a single pixel. This is also invoked by the
            Adafruit_GFX library in generating many higher-level graphics
            primitives.
    @param  x
            Column of display -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @param  color
            Pixel color, one of: BLACK, WHITE or INVERSE.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void UC1609::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x >= 0) && (x < width()) && (y >= 0) && (y < height())) {
    // Pixel is in-bounds. Rotate coordinates if needed.
    switch (getRotation()) {
    case 1:
      uc1609_swap(x, y);
      x = WIDTH - x - 1;
      break;
    case 2:
      x = WIDTH - x - 1;
      y = HEIGHT - y - 1;
      break;
    case 3:
      uc1609_swap(x, y);
      y = HEIGHT - y - 1;
      break;
    }

    updateBoundingBox(x, y, x, y);

    switch (color) {
    case BLACK:
      buffer[x + (y / 8) * WIDTH] |= (1 << (y & 7));
      break;
    case WHITE:
      buffer[x + (y / 8) * WIDTH] &= ~(1 << (y & 7));
      break;
    case INVERSE:
      buffer[x + (y / 8) * WIDTH] ^= (1 << (y & 7));
      break;
    }
  }
}

/*!
    @brief  Clear contents of display buffer (set all pixels to off).
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void UC1609::clearDisplay(void) {
  memset(buffer, 0, WIDTH * ((HEIGHT + 7) / 8));
  updateBoundingBox(0, 0, WIDTH - 1, HEIGHT - 1);
}

/*!
    @brief  Return color of a single pixel in display buffer.
    @param  x
            Column of display -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @return true if pixel is set (usually UC1609_BLACK, unless display invert
   mode is enabled), false if clear (UC1609_WHITE).
    @note   Reads from buffer contents; may not reflect current contents of
            screen if display() has not been called.
*/
bool UC1609::getPixel(int16_t x, int16_t y) {
  if ((x >= 0) && (x < width()) && (y >= 0) && (y < height())) {
    // Pixel is in-bounds. Rotate coordinates if needed.
    switch (getRotation()) {
    case 1:
      uc1609_swap(x, y);
      x = WIDTH - x - 1;
      break;
    case 2:
      x = WIDTH - x - 1;
      y = HEIGHT - y - 1;
      break;
    case 3:
      uc1609_swap(x, y);
      y = HEIGHT - y - 1;
      break;
    }
    return (buffer[x + (y / 8) * WIDTH] & (1 << (y & 7)));
  }
  return false; // Pixel out of bounds
}

/*!
    @brief  Get base address of display buffer for direct reading or writing.
    @return Pointer to an unsigned 8-bit array, column-major, columns padded
            to full byte boundary if needed.
*/
uint8_t *UC1609::getBuffer(void) { return buffer; }

// REFRESH DISPLAY ---------------------------------------------------------

/*!
    @brief  Push data currently in RAM to UC1609 display.
    @return None (void).
    @note   Drawing operations are not visible until this function is
            called. Call after each graphics command, or after a whole set
            of graphics commands, as best needed by one's own application.
*/
void UC1609::display(void) {
    TRANSACTION_START
#if defined(ESP8266)
  // ESP8266 needs a periodic yield() call to avoid watchdog reset.
  // With the limited size of UC1609 displays, and the fast bitrate
  // being used (1 MHz or more), I think one yield() immediately before
  // a screen write and one immediately after should cover it.  But if
  // not, if this becomes a problem, yields() might be added in the
  // 32-byte transfer condition below.
  yield();
#endif

  uint8_t *ptr = buffer;
  uint8_t bytes_per_page = WIDTH;

  uint8_t first_page = yUpdateMin / 8;
  uint8_t last_page = (yUpdateMax + 7) / 8;
  uint8_t page_start = min(bytes_per_page, xUpdateMin);
  uint8_t page_end = max((uint8_t)0, xUpdateMax);

  for(uint16_t p = first_page; p < last_page ; p++) {
    uint8_t bytes_remaining = bytes_per_page;
    ptr = buffer + (uint16_t)p * (uint16_t)bytes_per_page;
    // fast forward to dirty rectangle beginning
    bytes_remaining -= page_start;
    ptr += page_start;
    // cut off end of dirty rectangle
    bytes_remaining -= (WIDTH - 1) - page_end;

    uint8_t cmd[] = {uint8_t(UC1609_SET_PAGEADD | p),
                     uint8_t(UC1609_SET_COLADD_MSB | (page_start >> 4)),
                     uint8_t(UC1609_SET_COLADD_LSB | (page_start & 0x0F))};
    uc1609_commandList(cmd, sizeof(cmd));

    if (wire) { // I2C
      wire->beginTransmission(i2caddr | 0x01); // CD = 1
      uint16_t bytesOut = 0;
      while (bytes_remaining--) {
        if (bytesOut >= WIRE_MAX) {
          wire->endTransmission();
          wire->beginTransmission(i2caddr | 0x01); // CD = 1
          bytesOut = 0;
        }
      WIRE_WRITE(*ptr++);
      bytesOut++;
      }
    wire->endTransmission();
    } else { // SPI
    UC1609_MODE_DATA
    while (bytes_remaining--)
      SPIwrite(*ptr++);
    }
  }
  TRANSACTION_END
#if defined(ESP8266)
  yield();
#endif

  // reset bounding box
  xUpdateMin = WIDTH - 1;
  xUpdateMax = 0;
  yUpdateMin = HEIGHT - 1;
  yUpdateMax = 0;
}

// OTHER HARDWARE SETTINGS -------------------------------------------------

/*!
  @brief Invert the entire display
  @param i True to invert the display, false to keep it uninverted
 */
void UC1609::invertDisplay(bool i) {
  TRANSACTION_START
  uc1609_command1(UC1609_INVERSE_DISPLAY | i);
  TRANSACTION_END
}

/*!
  @brief  Set the contrast level
  @param val Contrast value
 */
void UC1609::setContrast(uint8_t val) {
  _contrast = map(val, 0, 100, 0, 255); // Input range: 0~100
  TRANSACTION_START
  uc1609_command1(UC1609_GN_PM);
  uc1609_command1(_contrast);
  TRANSACTION_END
}

/*!
    @brief  Put the display driver into a low power mode instead of just turning
   off all pixels
*/
void UC1609::sleep(void) { 
  TRANSACTION_START
  uc1609_command1(UC1609_DISPLAY_ON | 0);
  TRANSACTION_END
   }

/*!
    @brief  Wake the display driver from the low power mode
*/
void UC1609::wake(void) { 
  TRANSACTION_START
  uc1609_command1(UC1609_DISPLAY_ON | 1);
  TRANSACTION_END
   }
