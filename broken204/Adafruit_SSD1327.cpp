/*********************************************************************
This is a library for our grayscale OLEDs based on SSD1327 drivers

  Pick one up today in the adafruit shop!
  ------> https://www.adafruit.com/products/4741

These displays use I2C or SPI to communicate

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen below must be included in any
redistribution
*********************************************************************/

#include "Adafruit_SSD1327.h"
#include "splash.h"

// CONSTRUCTORS, DESTRUCTOR ------------------------------------------------

/*!
    @brief  Constructor for I2C-interfaced SSD1327 displays.
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
            Speed (in Hz) for Wire transmissions in SSD1327 library calls.
            Defaults to 400000 (400 KHz), a known 'safe' value for most
            microcontrollers, and meets the SSD1327 datasheet spec.
            Some systems can operate I2C faster (800 KHz for ESP32, 1 MHz
            for many other 32-bit MCUs), and some (perhaps not all)
            SSD1327's can work with this -- so it's optionally be specified
            here and is not a default behavior. (Ignored if using pre-1.5.7
            Arduino software, which operates I2C at a fixed 100 KHz.)
    @param  clkAfter
            Speed (in Hz) for Wire transmissions following SSD1327 library
            calls. Defaults to 100000 (100 KHz), the default Arduino Wire
            speed. This is done rather than leaving it at the 'during' speed
            because other devices on the I2C bus might not be compatible
            with the faster rate. (Ignored if using pre-1.5.7 Arduino
            software, which operates I2C at a fixed 100 KHz.)
    @note   Call the object's begin() function before use -- buffer
            allocation is performed there!
*/
Adafruit_SSD1327::Adafruit_SSD1327(uint16_t w, uint16_t h, TwoWire *twi,
                                   int8_t rst_pin, uint32_t clkDuring,
                                   uint32_t clkAfter)
    : Adafruit_GrayOLED(4, w, h, twi, rst_pin, clkDuring, clkAfter) {}

/*!
    @brief  Constructor for SPI SSD1327 displays, using software (bitbang)
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
Adafruit_SSD1327::Adafruit_SSD1327(uint16_t w, uint16_t h, int8_t mosi_pin,
                                   int8_t sclk_pin, int8_t dc_pin,
                                   int8_t rst_pin, int8_t cs_pin)
    : Adafruit_GrayOLED(4, w, h, mosi_pin, sclk_pin, dc_pin, rst_pin, cs_pin) {}

/*!
    @brief  Constructor for SPI SSD1327 displays, using native hardware SPI.
    @param  w
            Display width in pixels
    @param  h
            Display height in pixels
    @param  spi
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
Adafruit_SSD1327::Adafruit_SSD1327(uint16_t w, uint16_t h, SPIClass *spi,
                                   int8_t dc_pin, int8_t rst_pin, int8_t cs_pin,
                                   uint32_t bitrate)
    : Adafruit_GrayOLED(4, w, h, spi, dc_pin, rst_pin, cs_pin, bitrate) {}

/*!
    @brief  Destructor for Adafruit_SSD1327 object.
*/
Adafruit_SSD1327::~Adafruit_SSD1327(void) {}

// ALLOCATE & INIT DISPLAY -------------------------------------------------

/*!
    @brief  Allocate RAM for image buffer, initialize peripherals and pins.
    @param  addr
            I2C address of corresponding SSD1327 display.
            SPI displays (hardware or software) do not use addresses, but
            this argument is still required (pass 0 or any value really,
            it will simply be ignored). Default if unspecified is 0.
    @param  reset
            If true, and if the reset pin passed to the constructor is
            valid, a hard reset will be performed before initializing the
            display. If using multiple SSD1327 displays on the same bus, and
            if they all share the same reset pin, you should only pass true
            on the first display being initialized, false on all others,
            else the already-initialized displays would be reset. Default if
            unspecified is true.
    @return true on successful allocation/init, false otherwise.
            Well-behaved code should check the return value before
            proceeding.
    @note   MUST call this function before any drawing or updates!
*/
bool Adafruit_SSD1327::begin(uint8_t addr, bool reset) {

  if (!Adafruit_GrayOLED::_init(addr, reset)) {
    return false;
  }

  /*
  drawBitmap((WIDTH - splash2_width) / 2, (HEIGHT - splash2_height) / 2,
             splash2_data, splash2_width, splash2_height, 1);
             */

  // Init sequence, make sure its under 32 bytes, or split into multiples!
  static const uint8_t init_128x128[] = {
      // Init sequence for 128x32 OLED module
      SSD1327_DISPLAYOFF, // 0xAE
      SSD1327_SETCONTRAST,
      0x80,             // 0x81, 0x80
      SSD1327_SEGREMAP, // 0xA0 0x53
      0x51, // remap memory, odd even columns, com flip and column swap
      SSD1327_SETSTARTLINE,
      0x00, // 0xA1, 0x00
      SSD1327_SETDISPLAYOFFSET,
      0x00, // 0xA2, 0x00
      SSD1327_DISPLAYALLOFF, SSD1327_SETMULTIPLEX,
      0x7F, // 0xA8, 0x7F (1/64)
      SSD1327_PHASELEN,
      0x11, // 0xB1, 0x11
      /*
      SSD1327_GRAYTABLE,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
      0x07, 0x08, 0x10, 0x18, 0x20, 0x2f, 0x38, 0x3f,
      */
      SSD1327_DCLK,
      0x00, // 0xb3, 0x00 (100hz)
      SSD1327_REGULATOR,
      0x01, // 0xAB, 0x01
      SSD1327_PRECHARGE2,
      0x04, // 0xB6, 0x04
      SSD1327_SETVCOM,
      0x0F, // 0xBE, 0x0F
      SSD1327_PRECHARGE,
      0x08, // 0xBC, 0x08
      SSD1327_FUNCSELB,
      0x62, // 0xD5, 0x62
      SSD1327_CMDLOCK,
      0x12, // 0xFD, 0x12
      SSD1327_NORMALDISPLAY, SSD1327_DISPLAYON};

  page_offset = 0;
  if (!oled_commandList(init_128x128, sizeof(init_128x128))) {
    return false;
  }

  delay(100);                      // 100ms delay recommended
  oled_command(SSD1327_DISPLAYON); // 0xaf
  setContrast(0x2F);

  // memset(buffer, 0x81, _bpp * WIDTH * ((HEIGHT + 7) / 8));

  return true; // Success
}

/*!
    @brief  Do the actual writing of the internal frame buffer to display RAM
*/
void Adafruit_SSD1327::display(void) {
  // ESP8266 needs a periodic yield() call to avoid watchdog reset.
  // With the limited size of SSD1327 displays, and the fast bitrate
  // being used (1 MHz or more), I think one yield() immediately before
  // a screen write and one immediately after should cover it.  But if
  // not, if this becomes a problem, yields() might be added in the
  // 32-byte transfer condition below.
  yield();

  uint16_t count = WIDTH * ((HEIGHT + 7) / 8);
  (void)count;
  uint8_t *ptr = buffer;
  uint8_t dc_byte = 0x40;
  uint8_t rows = HEIGHT;

  uint8_t bytes_per_row = WIDTH / 2; // See fig 10-1 (64 bytes, 128 pixels)
  uint8_t maxbuff = 128;

  /*
  Serial.print("Window: (");
  Serial.print(window_x1);
  Serial.print(", ");
  Serial.print(window_y1);
  Serial.print(") -> (");
  Serial.print(window_x2);
  Serial.print(", ");
  Serial.print(window_y2);
  Serial.println(")");
  */

  int16_t row_start =
      min((int16_t)(bytes_per_row - 1), (int16_t)(window_x1 / 2));
  int16_t row_end = max((int16_t)0, (int16_t)(window_x2 / 2));

  int16_t first_row = min((int16_t)(rows - 1), (int16_t)window_y1);
  int16_t last_row = max((int16_t)0, (int16_t)window_y2);

  /*
  Serial.print("Rows: ");
  Serial.print(first_row);
  Serial.print(" -> ");
  Serial.println(last_row);

  Serial.print("Row start/end: ");
  Serial.print(row_start);
  Serial.print(" -> ");
  Serial.println(row_end);
  */

  if (i2c_dev) { // I2C
    // Set high speed clk
    i2c_dev->setSpeed(i2c_preclk);
    maxbuff = i2c_dev->maxBufferSize() - 1;
  }

  uint8_t cmd[] = {SSD1327_SETROW,    (uint8_t)first_row, (uint8_t)last_row,
                   SSD1327_SETCOLUMN, (uint8_t)row_start, (uint8_t)row_end};
  oled_commandList(cmd, sizeof(cmd));

  for (uint8_t row = first_row; row <= last_row; row++) {
    uint8_t bytes_remaining = row_end - row_start + 1;
    ptr = buffer + (uint16_t)row * (uint16_t)bytes_per_row;
    // fast forward to dirty rectangle beginning
    ptr += row_start;

    while (bytes_remaining) {
      uint8_t to_write = min(bytes_remaining, maxbuff);
      if (i2c_dev) {
        i2c_dev->write(ptr, to_write, true, &dc_byte, 1);
      } else {
        digitalWrite(dcPin, HIGH);
        spi_dev->write(ptr, to_write);
      }
      ptr += to_write;
      bytes_remaining -= to_write;
      yield();
    }
  }

  if (i2c_dev) { // I2C
    // Set low speed clk
    i2c_dev->setSpeed(i2c_postclk);
  }

  // reset dirty window
  window_x1 = 1024;
  window_y1 = 1024;
  window_x2 = -1;
  window_y2 = -1;
}

/*!
    @brief  Enable or disable display invert mode (white-on-black vs
            black-on-white). Handy for testing!
    @param  i
            If true, switch to invert mode (black-on-white), else normal
            mode (white-on-black).
*/
void Adafruit_SSD1327::invertDisplay(bool i) {
  oled_command(i ? SSD1327_INVERTDISPLAY : SSD1327_NORMALDISPLAY);
}
