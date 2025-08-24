// created by Jean-Marc Zingg to be a standalone ILI9486_SPI library (instead of the GxCTRL_ILI9486_SPI class for the GxTFT library)
// code extracts taken from https://github.com/Bodmer/TFT_HX8357
// spi kludge handling solution found in https://github.com/Bodmer/TFT_eSPI
// code extracts taken from https://github.com/adafruit/Adafruit-GFX-Library
//
// License: GNU GENERAL PUBLIC LICENSE V3, see LICENSE
//

#ifndef _ILI9486_SPI_H_
#define _ILI9486_SPI_H_

#include <Arduino.h>
#include <SPI.h>

#include "GFXcanvas16T.h"

#include <Adafruit_GFX.h>

class ILI9486_SPI : public Adafruit_GFX
{
  public:
    ILI9486_SPI(int8_t cs, int8_t dc, int8_t rst);
    ILI9486_SPI(SPIClass *spiClass, int8_t cs, int8_t dc, int8_t rst);
    // (overridden) virtual methods
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
    void writePixels(uint16_t *colors, uint32_t len);
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    virtual void fillScreen(uint16_t color);
    virtual void setRotation(uint8_t r);
    virtual void invertDisplay(bool i);
    // other public methods
    void setSpiKludge(bool rpi_spi16_mode = true); // call with false before init to disable
    void init(void);
    void setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {setWindow(x, y, w, h);}
    void pushColors(const uint16_t* data, uint16_t n); // fast one
    void setBackLight(bool lit);
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    void drawRGBBitmap(int16_t x, int16_t y, uint16_t *pcolors, int16_t w, int16_t h);
    void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h);
    virtual void _startTransaction();
    virtual void _endTransaction();
    void _writeCommand(uint8_t cmd);
    void sendCommand(uint8_t cmd);
    void startWrite(void){}
    void endWrite(void){}
  private:
    void _setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void _writeCommand16(uint16_t cmd);
    void _writeData(uint8_t data);
    void _writeData16(uint16_t data);
    void _writeData16(uint16_t data, uint32_t n);
    void _writeData16(const uint16_t* data, uint32_t n);
    // note: only use for pixel data, RGB888 on ILI9488, ILI9486 native SPI
    void _writeColor16(uint16_t data, uint32_t n);
    void _writeColor16(const uint16_t* data, uint32_t n);
  private:
    bool _spi16_mode;
    SPISettings _spi_settings;
    SPIClass *_spi;
    int8_t _cs, _dc, _rst;
    int8_t _bgr;
    int32_t  _x_address_set, _y_address_set;
    uint32_t _cs_pinmask, _dc_pinmask;
};

#endif
