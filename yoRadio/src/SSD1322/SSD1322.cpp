/*!
 * @file Jamis_SSD1322.h
 *
 * This is a partial port of Adafruit's SSD1306 library to the SSD1322 
 * display.
 *
 * These displays use SPI to communicate. SPI requires 4 pins (MOSI, SCK,
 * select, data/command) and optionally a reset pin.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries, with
 * contributions from the open source community.
 *
 * BSD license, all text above, and the splash screen header file,
 * must be included in any redistribution.
 *
 */
#include "../core/options.h"
#if DSP_MODEL==DSP_SSD1322


#define pgm_read_byte(addr) \
  (*(const unsigned char *)(addr)) ///< PROGMEM workaround for non-AVR

#include <Adafruit_GFX.h>
#include "SSD1322.h"
//#include "splash.h"

#define ssd1322_swap(a, b) \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation

#define SSD1322_SELECT       digitalWrite(csPin, LOW);  ///< Device select
#define SSD1322_DESELECT     digitalWrite(csPin, HIGH); ///< Device deselect
#define SSD1322_MODE_COMMAND digitalWrite(dcPin, LOW);  ///< Command mode
#define SSD1322_MODE_DATA    digitalWrite(dcPin, HIGH); ///< Data mode

#if defined(SPI_HAS_TRANSACTION)
  #define SPI_TRANSACTION_START spi->beginTransaction(spiSettings) ///< Pre-SPI
  #define SPI_TRANSACTION_END   spi->endTransaction()              ///< Post-SPI
#else // SPI transactions likewise not present in older Arduino SPI lib
  #define SPI_TRANSACTION_START ///< Dummy stand-in define
  #define SPI_TRANSACTION_END   ///< keeps compiler happy
#endif

#define TRANSACTION_START   \
  if(spi) {                \
    SPI_TRANSACTION_START; \
  }                        \
  SSD1322_SELECT;
#define TRANSACTION_END     \
  SSD1322_DESELECT;        \
  if(spi) {                \
    SPI_TRANSACTION_END;   \
  }

/*!
    @brief  Constructor for SPI SSD1306 displays, using native hardware SPI.
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
    @return Jamis_SSD1322 object.
    @note   Call the object's begin() function before use -- buffer
            allocation is performed there!
*/
Jamis_SSD1322::Jamis_SSD1322(int16_t w, int16_t h, SPIClass *spi,
  int8_t dc_pin, int8_t rst_pin, int8_t cs_pin, uint32_t bitrate) : 
  Adafruit_GFX(w, h), spi(spi ? spi : &SPI), buffer(NULL),
  mosiPin(-1), clkPin(-1), dcPin(dc_pin), csPin(cs_pin), rstPin(rst_pin) {
#ifdef SPI_HAS_TRANSACTION
  spiSettings = SPISettings(bitrate, MSBFIRST, SPI_MODE3);
#endif
}

/*!
    @brief  Destructor for Jamis_SSD1322 object.
*/
Jamis_SSD1322::~Jamis_SSD1322(void) {
  if(buffer) {
    free(buffer);
    buffer = NULL;
  }
}

// LOW-LEVEL UTILS ---------------------------------------------------------

// Issue single byte out SPI, either soft or hardware as appropriate.
// SPI transaction/selection must be performed in calling function.
inline void Jamis_SSD1322::SPIwrite(uint8_t d) {
  spi->transfer(d);
}

// Issue single command to SSD1322, using hard SPI.
// Because command calls are often grouped, SPI transaction and selection
// must be started/ended in calling function for efficiency.
// This is a private function, not exposed (see ssd1322_command() instead).
void Jamis_SSD1322::ssd1322_command1(uint8_t c) {
  SSD1322_MODE_COMMAND
  SPIwrite(c);
}

// Issue single byte of data out SPI.
void Jamis_SSD1322::ssd1322_data1(uint8_t c) {
  SSD1322_MODE_DATA
  SPIwrite(c);
}

// A public version of ssd1322_command1(), for existing user code that
// might rely on that function. This encapsulates the command transfer
// in a transaction start/end, similar to old library's handling of it.
/*!
    @brief  Issue a single low-level command directly to the SSD1306
            display, bypassing the library.
    @param  c
            Command to issue (0x00 to 0xFF, see datasheet).
    @return None (void).
*/
void Jamis_SSD1322::ssd1322_command(uint8_t c) {
  TRANSACTION_START
  ssd1322_command1(c);
  TRANSACTION_END
}

// ALLOCATE & INIT DISPLAY -------------------------------------------------

/*!
    @brief  Allocate RAM for image buffer, initialize peripherals and pins.
    @param  reset
            If true, and if the reset pin passed to the constructor is
            valid, a hard reset will be performed before initializing the
            display. If using multiple SSD1322 displays on the same bus, and
            if they all share the same reset pin, you should only pass true
            on the first display being initialized, false on all others,
            else the already-initialized displays would be reset. Default if
            unspecified is true.
    @param  periphBegin
            If true, and if a hardware peripheral is being used (I2C or SPI,
            but not software SPI), call that peripheral's begin() function,
            else (false) it has already been done in one's sketch code.
            Cases where false might be used include multiple displays or
            other devices sharing a common bus, or situations on some
            platforms where a nonstandard begin() function is available
            (e.g. a TwoWire interface on non-default pins, as can be done
            on the ESP8266 and perhaps others).
    @return true on successful allocation/init, false otherwise.
            Well-behaved code should check the return value before
            proceeding.
    @note   MUST call this function before any drawing or updates!
*/
boolean Jamis_SSD1322::begin(boolean reset, boolean periphBegin) {
  // Note: The SSD1322 has 4 bit grayscale color.
  if((!buffer) && !(buffer = (uint8_t *)malloc( WIDTH * ((HEIGHT) / 2) )))
    return false;

  clearDisplay();


  // Setup pin directions
  pinMode(dcPin, OUTPUT); // Set data/command pin as output
  pinMode(csPin, OUTPUT); // Same for chip select
  SSD1322_DESELECT

  // SPI peripheral begin same as wire check above.
  if(periphBegin) spi->begin();

  // Reset SSD1322 if requested and reset pin specified in constructor
  if(reset && (rstPin >= 0)) {
    pinMode(     rstPin, OUTPUT);
    digitalWrite(rstPin, HIGH);
    delay(1);                   // VDD goes high at start, pause for 1 ms
    digitalWrite(rstPin, LOW);  // Bring reset low
    delay(10);                  // Wait 10 ms
    digitalWrite(rstPin, HIGH); // Bring out of reset
  }

  TRANSACTION_START

  ssd1322_command1(0xFD); // Set Command Lock (MCU protection status)
  ssd1322_data1(0x12); // 0x12 = Unlock Basic Commands; 0x16 = lock
  
  ssd1322_command1(0xA4); // Set Display Mode = OFF
  
  ssd1322_command1(0xB3); // Set Front Clock Divider / Oscillator Frequency
  ssd1322_data1(0x91); // 0x91 = 80FPS; 0xD0 = default / 1100b 
  
  ssd1322_command1(0xCA); // Set MUX Ratio
  ssd1322_data1(0x3F); // 0x3F = 63d = 64MUX (1/64 duty cycle)
  
  ssd1322_command1(0xA2); // Set Display Offset
  ssd1322_data1(0x00); // 0x00 = (default)
  
  ssd1322_command1(0xA1); // Set Display Start Line
  ssd1322_data1(0x00); // 0x00 = register 00h
  
  ssd1322_command1(0xA0); // Set Re-map and Dual COM Line mode
  ssd1322_data1(0x14); // 0x14 = Default except Enable Nibble Re-map, Scan from COM[N-1] to COM0, where N is the Multiplex ratio
  ssd1322_data1(0x11); // 0x11 = Enable Dual COM mode (MUX <= 63)
  
  ssd1322_command1(0xB5); // Set GPIO
  ssd1322_data1(0x00); // 0x00 = {GPIO0, GPIO1 = HiZ (Input Disabled)}
  
  ssd1322_command1(0xAB); // Function Selection
  ssd1322_data1(0x01); // 0x01 = Enable internal VDD regulator (default)
  
  ssd1322_command1(0xB4); // Display Enhancement A
  ssd1322_data1(0xA0); // 0xA0 = Enable external VSL; 0xA2 = internal VSL
  ssd1322_data1(0xB5); // 0xB5 = Normal (default); 0xFD = 11111101b = Enhanced low GS display quality
  
  ssd1322_command1(0xC1); // Set Contrast Current
  ssd1322_data1(0x7F); // 0x7F = (default)
  
  ssd1322_command1(0xC7); // Master Contrast Current Control
  ssd1322_data1(0x0F); // 0x0F = (default)
  
  ssd1322_command1(0xB9); // Select Default Linear Gray Scale table
  
  // ssd1322_command1(0xB8); // Select Custom Gray Scale table (GS0 = 0)
  // ssd1322_command1(0x00); // GS1
  // ssd1322_command1(0x08); // GS2
  // ssd1322_command1(0x10); // GS3
  // ssd1322_command1(0x18); // GS4
  // ssd1322_command1(0x20); // GS5
  // ssd1322_command1(0x28); // GS6
  // ssd1322_command1(0x30); // GS7
  // ssd1322_command1(0x38); // GS8
  // ssd1322_command1(0x40); // GS9
  // ssd1322_command1(0x48); // GS10
  // ssd1322_command1(0x50); // GS11
  // ssd1322_command1(0x58); // GS12
  // ssd1322_command1(0x60); // GS13
  // ssd1322_command1(0x68); // GS14
  // ssd1322_command1(0x70); // GS15
  // ssd1322_command1(0x00); // Enable Custom Gray Scale table
  
  ssd1322_command1(0xB1); // Set Phase Length
  ssd1322_data1(0xE2); // 0xE2 = Phase 1 period (reset phase length) = 5 DCLKs,
                       //        Phase 2 period (first pre-charge phase length) = 14 DCLKs
  
  ssd1322_command1(0xD1); // Display Enhancement B
  ssd1322_data1(0xA2); // 0xA2 = Normal (default); 0x82 = reserved
  ssd1322_data1(0x20); // 0x20 = as-is
  
  ssd1322_command1(0xBB); // Set Pre-charge voltage
  ssd1322_data1(0x1F); // 0x17 = default; 0x1F = 0.60*Vcc (spec example)
  
  ssd1322_command1(0xB6); // Set Second Precharge Period
  ssd1322_data1(0x08); // 0x08 = 8 dclks (default)
  
  ssd1322_command1(0xBE); // Set VCOMH
  ssd1322_data1(0x07); // 0x04 = 0.80*Vcc (default); 0x07 = 0.86*Vcc (spec example)
  
  ssd1322_command1(0xA6); // Set Display Mode = Normal Display
  
  ssd1322_command1(0xA9); // Exit Partial Display
  
  ssd1322_command1(0xAF); // Set Sleep mode OFF (Display ON)
  
  TRANSACTION_END

  return true; // Success
}

// DRAWING FUNCTIONS -------------------------------------------------------

/*!
    @brief  Set/clear/invert a single pixel. This is also invoked by the
            Adafruit_GFX library in generating many higher-level graphics
            primitives.
    @param  x
            Column of display -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @param  color
            Pixel color, one of: BLACK, WHITE or INVERT.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void Jamis_SSD1322::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if((x >= 0) && (x < width()) && (y >= 0) && (y < height())) {
    // Pixel is in-bounds. Rotate coordinates if needed.
    switch(getRotation()) {
     case 1:
      ssd1322_swap(x, y);
      x = WIDTH - x - 1;
      break;
     case 2:
      x = WIDTH  - x - 1;
      y = HEIGHT - y - 1;
      break;
     case 3:
      ssd1322_swap(x, y);
      y = HEIGHT - y - 1;
      break;
    }
    buffer[(x >> 1) + (y)*WIDTH/2] &= (x % 2) ? 0xF0 : 0x0F;
    buffer[(x >> 1) + (y)*WIDTH/2] |=  (color << (!(x & 1) * 4) );
  }
}


/*!
    @brief  Clear contents of display buffer (set all pixels to off).
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void Jamis_SSD1322::clearDisplay(void) {
  memset(buffer, 0, WIDTH * ((HEIGHT) / 2));
}

/*!
    @brief  Draw a horizontal line. This is also invoked by the Adafruit_GFX
            library in generating many higher-level graphics primitives.
    @param  x
            Leftmost column -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @param  w
            Width of line, in pixels.
    @param  color
            Line color, one of: BLACK, WHITE or INVERT.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void Jamis_SSD1322::drawFastHLine(
  int16_t x, int16_t y, int16_t w, uint16_t color) {
  boolean bSwap = false;
  switch(rotation) {
   case 1:
    // 90 degree rotation, swap x & y for rotation, then invert x
    bSwap = true;
    ssd1322_swap(x, y);
    x = WIDTH - x - 1;
    break;
   case 2:
    // 180 degree rotation, invert x and y, then shift y around for height.
    x  = WIDTH  - x - 1;
    y  = HEIGHT - y - 1;
    x -= (w-1);
    break;
   case 3:
    // 270 degree rotation, swap x & y for rotation,
    // then invert y and adjust y for w (not to become h)
    bSwap = true;
    ssd1322_swap(x, y);
    y  = HEIGHT - y - 1;
    y -= (w-1);
    break;
  }

  if(bSwap) drawFastVLineInternal(x, y, w, color);
  else      drawFastHLineInternal(x, y, w, color);
}

void Jamis_SSD1322::drawFastHLineInternal(
  int16_t x, int16_t y, int16_t w, uint16_t color) {

  if((y >= 0) && (y < HEIGHT)) { // Y coord in bounds?
    if(x < 0) { // Clip left
      w += x;
      x  = 0;
    }
    if((x + w) > WIDTH) { // Clip right
      w = (WIDTH - x);
    }
    if(w > 0) { // Proceed only if width is positive
      // NOTE: This is _not_ fast. But with 4bit packing, I just want this done.
      uint16_t yOffset = (y)*WIDTH/2;
      uint8_t b1 = (x % 2) ? 0xF0 : 0x0F;
      while(w--) {
        buffer[((x + w) >> 1) + yOffset] &= b1;
        buffer[((x + w) >> 1) + yOffset] |=  (color << (!((x + w) & 1) * 4) );
      }
    }
  }
}

/*!
    @brief  Draw a vertical line. This is also invoked by the Adafruit_GFX
            library in generating many higher-level graphics primitives.
    @param  x
            Column of display -- 0 at left to (screen width -1) at right.
    @param  y
            Topmost row -- 0 at top to (screen height - 1) at bottom.
    @param  h
            Height of line, in pixels.
    @param  color
            Line color, one of: BLACK, WHITE or INVERT.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void Jamis_SSD1322::drawFastVLine(
  int16_t x, int16_t y, int16_t h, uint16_t color) {
  boolean bSwap = false;
  switch(rotation) {
   case 1:
    // 90 degree rotation, swap x & y for rotation,
    // then invert x and adjust x for h (now to become w)
    bSwap = true;
    ssd1322_swap(x, y);
    x  = WIDTH - x - 1;
    x -= (h-1);
    break;
   case 2:
    // 180 degree rotation, invert x and y, then shift y around for height.
    x = WIDTH  - x - 1;
    y = HEIGHT - y - 1;
    y -= (h-1);
    break;
   case 3:
    // 270 degree rotation, swap x & y for rotation, then invert y
    bSwap = true;
    ssd1322_swap(x, y);
    y = HEIGHT - y - 1;
    break;
  }

  if(bSwap) drawFastHLineInternal(x, y, h, color);
  else      drawFastVLineInternal(x, y, h, color);
}

void Jamis_SSD1322::drawFastVLineInternal(
  int16_t x, int16_t __y, int16_t __h, uint16_t color) {

  if((x >= 0) && (x < WIDTH)) { // X coord in bounds?
    if(__y < 0) { // Clip top
      __h += __y;
      __y = 0;
    }
    if((__y + __h) > HEIGHT) { // Clip bottom
      __h = (HEIGHT - __y);
    }
    if(__h > 0) { // Proceed only if height is now positive

      //buffer[((x + w) >> 1) + yOffset] |=  (color << (!(w & 1) * 4) );
      uint16_t xOffset = (x >> 1);
      uint16_t mask = (color << (!(x & 1) * 4) );
      uint8_t b1 = (x % 2) ? 0xF0 : 0x0F;
      while(__h--) {
        //Serial.printf("xOffset + (__y+__h)*WIDTH/2=%d, WIDTH=%d\n", xOffset + (__y+__h)*WIDTH/2, WIDTH);
        buffer[xOffset + (__y+__h)*WIDTH/2] &= b1;
        buffer[xOffset + (__y+__h)*WIDTH/2] |= mask;
      }

      
    } // endif positive height
  } // endif x in bounds
}

/*!
    @brief  Return color of a single pixel in display buffer.
    @param  x
            Column of display -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @return true if pixel is set (usually WHITE, unless display invert mode
            is enabled), false if clear (BLACK).
    @note   Reads from buffer contents; may not reflect current contents of
            screen if display() has not been called.
*/
boolean Jamis_SSD1322::getPixel(int16_t x, int16_t y) {
  if((x >= 0) && (x < width()) && (y >= 0) && (y < height())) {
    // Pixel is in-bounds. Rotate coordinates if needed.
    switch(getRotation()) {
     case 1:
      ssd1322_swap(x, y);
      x = WIDTH - x - 1;
      break;
     case 2:
      x = WIDTH  - x - 1;
      y = HEIGHT - y - 1;
      break;
     case 3:
      ssd1322_swap(x, y);
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
uint8_t *Jamis_SSD1322::getBuffer(void) {
  return buffer;
}

// REFRESH DISPLAY ---------------------------------------------------------

/*!
    @brief  Push data currently in RAM to SSD1306 display.
    @return None (void).
    @note   Drawing operations are not visible until this function is
            called. Call after each graphics command, or after a whole set
            of graphics commands, as best needed by one's own application.
*/
void Jamis_SSD1322::display(void) {
  TRANSACTION_START

  // Set column address: Set_Column_Address(0x00, MAXCOLS-1);
  ssd1322_command1(0x15);
  // Each Column address holds 4 horizontal pixels worth of data
  const uint16_t Col0Off = 0x70;
  const uint16_t ColDiv  =    4;
  ssd1322_data1( (Col0Off+0x00)/ColDiv );
  ssd1322_data1( (Col0Off+WIDTH-1)/ColDiv );

  // Set row address: Set_Row_Address(0x00, MAXROWS-1);
  ssd1322_command1(0x75);
  ssd1322_data1(0x00);
  ssd1322_data1(HEIGHT-1);
  
  // Enable writing into MCU RAM: Set_Write_RAM();
  ssd1322_command1(0x5C);

  uint16_t count = WIDTH * ((HEIGHT) / 2);
  //Serial.printlnf("%i", count);
  uint8_t *ptr   = buffer;
  SSD1322_MODE_DATA
  while(count--) SPIwrite(*ptr++);
  TRANSACTION_END
}

#endif //if DSP_MODEL==DSP_SSD1322
