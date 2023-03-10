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

#ifndef _Jamis_SSD1322_H_
#define _Jamis_SSD1322_H_

#if defined(PARTICLE)
#include "Particle.h"
#define SPI_HAS_TRANSACTION
#define SPISettings __SPISettings
#define BUFFER_LENGTH 32
#else
#include <Wire.h>
#include <SPI.h>
#endif

#include <Adafruit_GFX.h>

#define BLACK                          0 ///< Draw 'off' pixels
#define WHITE                          0xf ///< Draw 'on' pixels
#define INVERSE                        2 ///< Invert pixels

#define SSD1322_DISPLAYOFF 0xAE
#define SSD1322_DISPLAYON 0xAF

/*! 
    @brief  Class that stores state and functions for interacting with
            SSD1322 OLED displays.
*/
class Jamis_SSD1322 : public Adafruit_GFX {
  public:
    Jamis_SSD1322(int16_t w, int16_t h, SPIClass *spi, int8_t dc_pin, 
      int8_t rst_pin, int8_t cs_pin, uint32_t bitrate=8000000UL);

    ~Jamis_SSD1322(void);

    boolean      begin(boolean reset=true, boolean periphBegin=true);
    void         display(void);
    void         clearDisplay(void);
    void         drawPixel(int16_t x, int16_t y, uint16_t color);
    virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void         ssd1322_command(uint8_t c);
    boolean      getPixel(int16_t x, int16_t y);
    uint8_t      *getBuffer(void);
    void         oled_command(uint8_t c) { ssd1322_command1(c); }
  private:
    inline void  SPIwrite(uint8_t d) __attribute__((always_inline));
    void         drawFastHLineInternal(int16_t x, int16_t y, int16_t w,
                 uint16_t color);
    void         drawFastVLineInternal(int16_t x, int16_t y, int16_t h,
                 uint16_t color);
    void         ssd1322_command1(uint8_t c);
    void         ssd1322_data1(uint8_t c);
    void         ssd1322_commandList(const uint8_t *c, uint8_t n);
    
    SPIClass    *spi;
    uint8_t     *buffer;
    int8_t       mosiPin    ,  clkPin    ,  dcPin    ,  csPin, rstPin;
#if defined(SPI_HAS_TRANSACTION)
    SPISettings  spiSettings;
#endif
};

#endif // _Jamis_SSD1322_H_
