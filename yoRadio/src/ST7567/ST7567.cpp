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

#include "ST7567.h"
#include <Adafruit_GFX.h>

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

#define st7567_swap(a, b)                                                     \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation

#if ARDUINO >= 100
#define WIRE_WRITE wire->write ///< Wire write function in recent Arduino lib
#else
#define WIRE_WRITE wire->send ///< Wire write function in older Arduino lib
#endif

#ifdef HAVE_PORTREG
#define ST7567_SELECT *csPort &= ~csPinMask;       ///< Device select
#define ST7567_DESELECT *csPort |= csPinMask;      ///< Device deselect
#define ST7567_MODE_COMMAND *dcPort &= ~dcPinMask; ///< Command mode
#define ST7567_MODE_DATA *dcPort |= dcPinMask;     ///< Data mode
#else
#define ST7567_SELECT digitalWrite(csPin, LOW);       ///< Device select
#define ST7567_DESELECT digitalWrite(csPin, HIGH);    ///< Device deselect
#define ST7567_MODE_COMMAND digitalWrite(dcPin, LOW); ///< Command mode
#define ST7567_MODE_DATA digitalWrite(dcPin, HIGH);   ///< Data mode
#endif

#if (ARDUINO >= 157) && !defined(ARDUINO_STM32_FEATHER)
#define SETWIRECLOCK wire->setClock(wireClk)    ///< Set before I2C transfer
#define RESWIRECLOCK wire->setClock(restoreClk) ///< Restore after I2C xfer
#else // setClock() is not present in older Arduino Wire lib (or WICED)
#define SETWIRECLOCK ///< Dummy stand-in define
#define RESWIRECLOCK ///< keeps compiler happy
#endif

#if defined(SPI_HAS_TRANSACTION)
#define SPI_TRANSACTION_START spi->beginTransaction(spiSettings) ///< Pre-SPI
#define SPI_TRANSACTION_END spi->endTransaction()                ///< Post-SPI
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
    ST7567_SELECT;                                                             \
  } ///< Wire, SPI or bitbang transfer setup
#define TRANSACTION_END                                                        \
  if (wire) {                                                                  \
    RESWIRECLOCK;                                                              \
  } else {                                                                     \
    ST7567_DESELECT;                                                           \
    if (spi) {                                                                 \
      SPI_TRANSACTION_END;                                                     \
    }                                                                          \
  } ///< Wire, SPI or bitbang transfer end


// CONSTRUCTORS, DESTRUCTOR ------------------------------------------------

/*!
    @brief  Constructor for I2C-interfaced ST7567 displays.
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
            Speed (in Hz) for Wire transmissions in ST7567 library calls.
            Defaults to 400000 (400 KHz), a known 'safe' value for most
            microcontrollers, and meets the ST7567 datasheet spec.
            Some systems can operate I2C faster (800 KHz for ESP32, 1 MHz
            for many other 32-bit MCUs), and some (perhaps not all)
            ST7567's can work with this -- so it's optionally be specified
            here and is not a default behavior. (Ignored if using pre-1.5.7
            Arduino software, which operates I2C at a fixed 100 KHz.)
    @param  clkAfter
            Speed (in Hz) for Wire transmissions following ST7567 library
            calls. Defaults to 100000 (100 KHz), the default Arduino Wire
            speed. This is done rather than leaving it at the 'during' speed
            because other devices on the I2C bus might not be compatible
            with the faster rate. (Ignored if using pre-1.5.7 Arduino
            software, which operates I2C at a fixed 100 KHz.)
    @note   Call the object's begin() function before use -- buffer
            allocation is performed there!
*/
ST7567::ST7567(uint8_t w, uint8_t h, TwoWire *twi,
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
    @brief  Constructor for SPI ST7567 displays, using software (bitbang)
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
ST7567::ST7567(uint8_t w, uint8_t h, int8_t mosi_pin,
               int8_t sclk_pin, int8_t dc_pin,
               int8_t rst_pin, int8_t cs_pin)
    : Adafruit_GFX(w, h), spi(NULL), wire(NULL), buffer(NULL),
      mosiPin(mosi_pin), clkPin(sclk_pin), dcPin(dc_pin), csPin(cs_pin),
      rstPin(rst_pin) 
{
}

/*!
    @brief  Constructor for SPI ST7567 displays, using native hardware SPI.
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
            unspecified is 4000000UL (4 MHz).
    @note   Call the object's begin() function before use -- buffer
            allocation is performed there!
*/
ST7567::ST7567(uint8_t w, uint8_t h, SPIClass *spi_ptr,
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
    @brief  Destructor for ST7567 object.
*/
ST7567::~ST7567(void) {
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
inline void ST7567::SPIwrite(uint8_t d) {
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
    @brief Issue single command to ST7567, using I2C or hard/soft SPI as
   needed. Because command calls are often grouped, SPI transaction and
   selection must be started/ended in calling function for efficiency. This is a
   protected function, not exposed (see ssd1306_command() instead).

        @param c
                   the command character to send to the display.
                   Refer to st7567 data sheet for commands
    @return None (void).
    @note
*/
void ST7567::st7567_command1(uint8_t c) {
  if (wire) { // I2C
    wire->beginTransmission(i2caddr);
    WIRE_WRITE((uint8_t)0x00); // Co = 0, D/C = 0
    WIRE_WRITE(c);
    wire->endTransmission();
  } else { // SPI (hw or soft) -- transaction started in calling function
    ST7567_MODE_COMMAND
    SPIwrite(c);
  }
}

/*!
    @brief Issue list of commands to ST7567, same rules as above re:
   transactions. This is a protected function, not exposed.
        @param c
                   pointer to list of commands

        @param n
                   number of commands in the list

    @return None (void).
    @note
*/
void ST7567::st7567_commandList(const uint8_t *c, uint8_t n) {
  if (wire) { // I2C
    wire->beginTransmission(i2caddr);
    WIRE_WRITE((uint8_t)0x00); // Co = 0, D/C = 0
    uint16_t bytesOut = 1;
    while (n--) {
      if (bytesOut >= WIRE_MAX) {
        wire->endTransmission();
        wire->beginTransmission(i2caddr);
        WIRE_WRITE((uint8_t)0x00); // Co = 0, D/C = 0
        bytesOut = 1;
      }
      WIRE_WRITE(pgm_read_byte(c++));
      bytesOut++;
    }
    wire->endTransmission();
  } else { // SPI -- transaction started in calling function
    ST7567_MODE_COMMAND
    while (n--)
      SPIwrite(pgm_read_byte(c++));
  }
}

// A public version of st7567_command1(), for existing user code that
// might rely on that function. This encapsulates the command transfer
// in a transaction start/end, similar to old library's handling of it.
/*!
    @brief  Issue a single low-level command directly to the ST7567
            display, bypassing the library.
    @param  c
            Command to issue (0x00 to 0xFF, see datasheet).
    @return None (void).
*/
void ST7567::st7567_command(uint8_t c) {
  TRANSACTION_START
  st7567_command1(c);
  TRANSACTION_END
}

// ALLOCATE & INIT DISPLAY -------------------------------------------------

/*!
    @brief  Allocate RAM for image buffer, initialize peripherals and pins.
    @param  addr
            I2C address of corresponding ST7567 display.
            SPI displays (hardware or software) do not use addresses, but
            this argument is still required (pass 0 or any value really,
            it will simply be ignored). Default if unspecified is 0.
    @param  reset
            If true, and if the reset pin passed to the constructor is
            valid, a hard reset will be performed before initializing the
            display. If using multiple ST7567 displays on the same bus, and
            if they all share the same reset pin, you should only pass true
            on the first display being initialized, false on all others,
            else the already-initialized displays would be reset. Default if
            unspecified is true.
    @return true on successful allocation/init, false otherwise.
            Well-behaved code should check the return value before
            proceeding.
    @note   MUST call this function before any drawing or updates!
*/
bool ST7567::begin(uint8_t addr, bool reset, bool periphBegin) {

  if ((!buffer) && !(buffer = (uint8_t *)malloc(WIDTH * ((HEIGHT + 7) / 8))))
    return false;

  clearDisplay();

  // Setup pin directions
  if (wire) { // Using I2C
    // If I2C address is unspecified, use default (0x3F)
    i2caddr = addr ? addr : ST7567_I2C_ADDRESS;
    // TwoWire begin() function might be already performed by the calling
    // function if it has unusual circumstances (e.g. TWI variants that
    // can accept different SDA/SCL pins, or if two ST7567 instances
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
    ST7567_DESELECT
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

  // Reset ST7567 if requested and reset pin specified in constructor
  if (reset && (rstPin >= 0)) {
    pinMode(rstPin, OUTPUT);
    digitalWrite(rstPin, HIGH);
    delay(1);                   // VDD goes high at start, pause for 1 ms
    digitalWrite(rstPin, LOW);  // Bring reset low
    delay(10);                  // Wait 10 ms
    digitalWrite(rstPin, HIGH); // Bring out of reset
  }

  TRANSACTION_START

  // Init sequence
  static const uint8_t PROGMEM init1[] = {CMD_INTERNAL_RESET,
                                          CMD_DISPLAY_OFF,
                                          CMD_SET_DISP_START_LINE,
                                          CMD_SEG_DIRECTION_NORMAL,
                                          CMD_SET_COM_REVERSE,
                                          CMD_SET_DISP_NORMAL,
                                          CMD_SET_BIAS_9};
  st7567_commandList(init1, sizeof(init1));

  st7567_command1(CMD_SET_POWER_CONTROL | 0x4);
  delay(50);
  st7567_command1(CMD_SET_POWER_CONTROL | 0x6);
  delay(50);
  st7567_command1(CMD_SET_POWER_CONTROL | 0x7);
  delay(50);
  st7567_command1(CMD_SET_RESISTOR_RATIO | 0x5);
  st7567_command1(CMD_DISPLAY_ON);
  st7567_command1(CMD_SET_VOLUME_FIRST);
  st7567_command1(CMD_SET_VOLUME_SECOND | _contrast); // set default contrast value

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
void ST7567::updateBoundingBox(uint8_t xmin, uint8_t ymin,
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
void ST7567::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x >= 0) && (x < width()) && (y >= 0) && (y < height())) {
    // Pixel is in-bounds. Rotate coordinates if needed.
    switch (getRotation()) {
    case 1:
      st7567_swap(x, y);
      x = WIDTH - x - 1;
      break;
    case 2:
      x = WIDTH - x - 1;
      y = HEIGHT - y - 1;
      break;
    case 3:
      st7567_swap(x, y);
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
void ST7567::clearDisplay(void) {
  memset(buffer, 0, WIDTH * ((HEIGHT + 7) / 8));
  updateBoundingBox(0, 0, LCDWIDTH - 1, LCDHEIGHT - 1);
}

/*!
    @brief  Return color of a single pixel in display buffer.
    @param  x
            Column of display -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @return true if pixel is set (usually SSD1306_WHITE, unless display invert
   mode is enabled), false if clear (SSD1306_BLACK).
    @note   Reads from buffer contents; may not reflect current contents of
            screen if display() has not been called.
*/
bool ST7567::getPixel(int16_t x, int16_t y) {
  if ((x >= 0) && (x < width()) && (y >= 0) && (y < height())) {
    // Pixel is in-bounds. Rotate coordinates if needed.
    switch (getRotation()) {
    case 1:
      st7567_swap(x, y);
      x = WIDTH - x - 1;
      break;
    case 2:
      x = WIDTH - x - 1;
      y = HEIGHT - y - 1;
      break;
    case 3:
      st7567_swap(x, y);
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
uint8_t *ST7567::getBuffer(void) { return buffer; }

// REFRESH DISPLAY ---------------------------------------------------------

/*!
    @brief  Push data currently in RAM to ST7567 display.
    @return None (void).
    @note   Drawing operations are not visible until this function is
            called. Call after each graphics command, or after a whole set
            of graphics commands, as best needed by one's own application.
*/
void ST7567::display(void) {
    TRANSACTION_START
#if defined(ESP8266)
  // ESP8266 needs a periodic yield() call to avoid watchdog reset.
  // With the limited size of ST7567 displays, and the fast bitrate
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

    uint8_t cmd[] = {uint8_t(CMD_SET_PAGE | p),
                     uint8_t(CMD_SET_COLUMN_UPPER | (page_start >> 4)),
                     uint8_t(CMD_SET_COLUMN_LOWER | (page_start & 0xF))};
    st7567_commandList(cmd, sizeof(cmd));

    if (wire) { // I2C
      wire->beginTransmission(i2caddr);
      WIRE_WRITE((uint8_t)0x40);
      uint16_t bytesOut = 1;
      while (bytes_remaining--) {
        if (bytesOut >= WIRE_MAX) {
          wire->endTransmission(false);
          wire->beginTransmission(i2caddr);
          WIRE_WRITE((uint8_t)0x40);
          bytesOut = 1;
        }
      WIRE_WRITE(*ptr++);
      bytesOut++;
      }
    wire->endTransmission();
    } else { // SPI
    ST7567_MODE_DATA
    while (bytes_remaining--)
      SPIwrite(*ptr++);
    }
  }
  TRANSACTION_END
#if defined(ESP8266)
  yield();
#endif

  // reset bounding box
  xUpdateMin = LCDWIDTH - 1;
  xUpdateMax = 0;
  yUpdateMin = LCDHEIGHT - 1;
  yUpdateMax = 0;
}

// OTHER HARDWARE SETTINGS -------------------------------------------------

/*!
  @brief Invert the entire display
  @param i True to invert the display, false to keep it uninverted
 */
void ST7567::invertDisplay(bool i) {
  TRANSACTION_START
  st7567_command1(i ? CMD_SET_DISP_REVERSE : CMD_SET_DISP_NORMAL);
  TRANSACTION_END
}

/*!
  @brief  Set the contrast level
  @param val Contrast value
 */
void ST7567::setContrast(uint8_t val) {
  if (val > 0x3f) {
    val = 0x3f;
  }
  _contrast = val;
  TRANSACTION_START
  st7567_command1(CMD_SET_VOLUME_FIRST);
  st7567_command1(CMD_SET_VOLUME_SECOND | val);
  TRANSACTION_END
}

/*!
  @brief  Get the contrast level
  @return Contrast value
 */
uint8_t ST7567::getContrast() { return _contrast; }

/*!
    @brief  Put the display driver into a low power mode instead of just turning
   off all pixels
*/
void ST7567::sleep(void) { 
  TRANSACTION_START
  st7567_command1(CMD_DISPLAY_OFF);
  st7567_command1(CMD_SET_ALLPTS_ON);
  TRANSACTION_END
   }

/*!
    @brief  Wake the display driver from the low power mode
*/
void ST7567::wake(void) { 
  TRANSACTION_START
  st7567_command1(CMD_SET_ALLPTS_NORMAL);
  st7567_command1(CMD_DISPLAY_ON);
  TRANSACTION_END
   }
