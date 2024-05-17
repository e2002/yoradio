// created by Jean-Marc Zingg to be a standalone ILI9486_SPI library (instead of the GxCTRL_ILI9486_SPI class for the GxTFT library)
// code extracts taken from https://github.com/Bodmer/TFT_HX8357
// spi kludge handling solution found in https://github.com/Bodmer/TFT_eSPI
// code extracts taken from https://github.com/adafruit/Adafruit-GFX-Library
//
// License: GNU GENERAL PUBLIC LICENSE V3, see LICENSE
//
#include "../core/options.h"
#if DSP_MODEL==DSP_ILI9488 || DSP_MODEL==DSP_ILI9486

#include <SPI.h>
#include "ILI9486_SPI.h"

#define SPI_SPEED 40000000UL
//#define SPI_SPEED 20000000 // max reliable speed is 20Mhz for RPi SPI kludge
//#define SPI_SPEED 4000000

#define ILI9486_SPI_CASET 0x2A
#define ILI9486_SPI_PASET 0x2B
#define ILI9486_SPI_RAMWR 0x2C
#define ILI9486_SPI_RAMRD 0x2E
#define ILI9486_SPI_MADCTL 0x36
#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

#define ILI9488_INVOFF  0x20
#define ILI9488_INVON   0x21

//ILI9486_SPI::ILI9486_SPI(int8_t cs, int8_t dc, int8_t rst) : Adafruit_GFX(320, 480), _spi_settings(SPI_SPEED, MSBFIRST, SPI_MODE0)
ILI9486_SPI::ILI9486_SPI(int8_t cs, int8_t dc, int8_t rst) : ILI9486_SPI(&SPI, cs, dc, rst) { }

ILI9486_SPI::ILI9486_SPI(SPIClass *spiClass, int8_t cs, int8_t dc, int8_t rst) : Adafruit_GFX(320, 480), _spi_settings(SPI_SPEED, MSBFIRST, SPI_MODE0)
{
  _spi = spiClass;
  _spi16_mode = true;
  _cs = cs;
  _dc = dc;
  _rst = rst;
  _bgr = MADCTL_BGR;
#if defined(ESP8266)
  if ((_cs >= 0) && (_dc >= 0))
  {
    _cs_pinmask = (uint32_t) digitalPinToBitMask(_cs);
    _dc_pinmask = (uint32_t) digitalPinToBitMask(_dc);
  }
  else
  {
    _cs_pinmask = 0;
    _dc_pinmask = 0;
  }
#endif
  _x_address_set = -1;
  _x_address_set = -1;
  digitalWrite(_cs, HIGH);
  digitalWrite(_dc, HIGH);
  pinMode(_cs, OUTPUT);
  pinMode(_dc, OUTPUT);
  if (rst >= 0)
  {
    digitalWrite(rst, HIGH);
    pinMode(rst, OUTPUT);
  }
}

// *** (overridden) virtual methods ***

#if defined(ESP8266) && true // This is for the RPi display that needs 16 bits

#define CMD_BITS (16-1)

void ILI9486_SPI::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  // Range checking
  if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return;
  if (_spi16_mode && _cs_pinmask)
  {
    _spi->beginTransaction(_spi_settings);
    GPOC = _cs_pinmask;
    SPI1U1 = (CMD_BITS << SPILMOSI) | (CMD_BITS << SPILMISO);
    // No need to send x if it has not changed (speeds things up)
    if (_x_address_set != x)
    {
      GPOC = _dc_pinmask;
      SPI1W0 = ILI9486_SPI_CASET << (CMD_BITS + 1 - 8);
      SPI1CMD |= SPIBUSY;
      while (SPI1CMD & SPIBUSY) {}
      GPOS = _dc_pinmask;
      SPI1W0 = x >> 0;
      SPI1CMD |= SPIBUSY;
      while (SPI1CMD & SPIBUSY) {}
      SPI1W0 = x << 8;
      SPI1CMD |= SPIBUSY;
      while (SPI1CMD & SPIBUSY) {}
      SPI1W0 = x >> 0;
      SPI1CMD |= SPIBUSY;
      while (SPI1CMD & SPIBUSY) {}
      SPI1W0 = x << 8;
      SPI1CMD |= SPIBUSY;
      while (SPI1CMD & SPIBUSY) {}
      _x_address_set = x;
    }
    // No need to send y if it has not changed (speeds things up)
    if (_y_address_set != y)
    {
      GPOC = _dc_pinmask;
      SPI1W0 = ILI9486_SPI_PASET << (CMD_BITS + 1 - 8);
      SPI1CMD |= SPIBUSY;
      while (SPI1CMD & SPIBUSY) {}
      GPOS = _dc_pinmask;
      SPI1W0 = y >> 0;
      SPI1CMD |= SPIBUSY;
      while (SPI1CMD & SPIBUSY) {}
      SPI1W0 = y << 8;
      SPI1CMD |= SPIBUSY;
      while (SPI1CMD & SPIBUSY) {}
      SPI1W0 = y >> 0;
      SPI1CMD |= SPIBUSY;
      while (SPI1CMD & SPIBUSY) {}
      SPI1W0 = y << 8;
      SPI1CMD |= SPIBUSY;
      while (SPI1CMD & SPIBUSY) {}
      _y_address_set = y;
    }
    GPOC = _dc_pinmask;
    SPI1W0 = ILI9486_SPI_RAMWR << (CMD_BITS + 1 - 8);
    SPI1CMD |= SPIBUSY;
    while (SPI1CMD & SPIBUSY) {}
    GPOS = _dc_pinmask;
    SPI1W0 = (color >> 8) | (color << 8);
    SPI1CMD |= SPIBUSY;
    while (SPI1CMD & SPIBUSY) {}
    GPOS = _cs_pinmask;
    _spi->endTransaction();
  }
  else
  {
    _startTransaction();
    _setWindow(x, y, 1, 1);
    _writeColor16(color, 1);
    _endTransaction();
  }
}

#else

void ILI9486_SPI::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
  {
    return;
  }
  _startTransaction();
  _setWindow(x, y, 1, 1);
  _writeColor16(color, 1);
  _endTransaction();
}

#endif

void ILI9486_SPI::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  //  if ((x < 0) || (y < 0) || (w < 1) || (h < 1) || (x + w > _width) || (y + h > _height))
  //  {
  //    Serial.print("fillRect("); Serial.print(x); Serial.print(", "); Serial.print(y); Serial.print(", "); Serial.print(w); Serial.print(", "); Serial.print(h); Serial.println(") oops? "); delay(1);
  //  }
  // a correct clipping is the goal. try to achieve this
  if (x < 0) w += x, x = 0;
  if (y < 0) h += y, y = 0;
  if (x + w > _width) w = _width - x;
  if (y + h > _height) h = _height - y;
  if ((w < 1) || (h < 1)) return;
  _startTransaction();
  _setWindow(x, y, w, h);
  _writeColor16(color, uint32_t(w) * uint32_t(h));
  _endTransaction();
}

void ILI9486_SPI::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  fillRect(x, y, 1, h, color);
}

void ILI9486_SPI::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  fillRect(x, y, w, 1, color);
}

void ILI9486_SPI::fillScreen(uint16_t color)
{
  fillRect(0, 0, _width, _height, color);
}

void ILI9486_SPI::setRotation(uint8_t r)
{
  Adafruit_GFX::setRotation(r);
  _startTransaction();
  if (_spi16_mode)
  {
    _writeCommand16(ILI9486_SPI_MADCTL);
    switch (r & 3)
    {
      case 0:
        _writeData16(MADCTL_MX | _bgr);
        break;
      case 1:
        _writeData16(MADCTL_MV | _bgr);
        break;
      case 2:
        _writeData16(MADCTL_MY | _bgr);
        break;
      case 3:
        _writeData16(MADCTL_MX | MADCTL_MY | MADCTL_MV | _bgr);
        break;
    }
  }
  else
  {
    _writeCommand(ILI9486_SPI_MADCTL);
    switch (r & 3)
    {
      case 0:
        _writeData(MADCTL_MX | _bgr);
        break;
      case 1:
        _writeData(MADCTL_MV | _bgr);
        break;
      case 2:
        _writeData(MADCTL_MY | _bgr);
        break;
      case 3:
        _writeData(MADCTL_MX | MADCTL_MY | MADCTL_MV | _bgr);
        break;
    }
  }
  _endTransaction();
}

void ILI9486_SPI::invertDisplay(bool i)
{
  /*_bgr = i ? MADCTL_BGR : 0x00;
  setRotation(rotation);*/
  sendCommand(i ? ILI9488_INVON : ILI9488_INVOFF);
}

// *** other public methods ***

void ILI9486_SPI::setSpiKludge(bool rpi_spi16_mode)
{
  _spi16_mode = rpi_spi16_mode;
}

void ILI9486_SPI::init(void)
{
  digitalWrite(_cs, HIGH);
#if defined(TFT_SCK) && defined(TFT_MOSI) && defined(TFT_MISO) && defined(TFT_RST)
  _spi->begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_RST);
#else
  _spi->begin();
#endif
  _spi->beginTransaction( { SPI_SPEED, MSBFIRST, SPI_MODE0 } );
  _spi->endTransaction();
  if (_rst >= 0)
  {
    digitalWrite(_rst, LOW);
    delay(20);
    digitalWrite(_rst, HIGH);
  }
  delay(200);
  _startTransaction();
  if (_spi16_mode)
  {
    _writeCommand16(0x3A);
    _writeData16(0x55);  // use 16 bits per pixel color
    _writeCommand16(0x36);
    _writeData16(0x48);  // MX, BGR == rotation 0
    // PGAMCTRL(Positive Gamma Control)
    _writeCommand16(0xE0);
    _writeData16(0x0F);
    _writeData16(0x1F);
    _writeData16(0x1C);
    _writeData16(0x0C);
    _writeData16(0x0F);
    _writeData16(0x08);
    _writeData16(0x48);
    _writeData16(0x98);
    _writeData16(0x37);
    _writeData16(0x0A);
    _writeData16(0x13);
    _writeData16(0x04);
    _writeData16(0x11);
    _writeData16(0x0D);
    _writeData16(0x00);
    // NGAMCTRL(Negative Gamma Control)
    _writeCommand16(0xE1);
    _writeData16(0x0F);
    _writeData16(0x32);
    _writeData16(0x2E);
    _writeData16(0x0B);
    _writeData16(0x0D);
    _writeData16(0x05);
    _writeData16(0x47);
    _writeData16(0x75);
    _writeData16(0x37);
    _writeData16(0x06);
    _writeData16(0x10);
    _writeData16(0x03);
    _writeData16(0x24);
    _writeData16(0x20);
    _writeData16(0x00);
    // Digital Gamma Control 1
    _writeCommand16(0xE2);
    _writeData16(0x0F);
    _writeData16(0x32);
    _writeData16(0x2E);
    _writeData16(0x0B);
    _writeData16(0x0D);
    _writeData16(0x05);
    _writeData16(0x47);
    _writeData16(0x75);
    _writeData16(0x37);
    _writeData16(0x06);
    _writeData16(0x10);
    _writeData16(0x03);
    _writeData16(0x24);
    _writeData16(0x20);
    _writeData16(0x00);
    _writeCommand16(0x11);  // Sleep OUT
    delay(150);   // wait some time
    _writeCommand16(0x29);  // Display ON
  }
  else
  {
    _writeCommand(0x3A);
    _writeData(0x66);  // 18 bit colour for native SPI
    _writeCommand(0x36);
    _writeData(0x48);  // MX, BGR == rotation 0
    // PGAMCTRL(Positive Gamma Control)
#if DSP_MODEL==DSP_ILI9486 && false
    _writeCommand(0xE0);
    _writeData(0x0F);
    _writeData(0x1F);
    _writeData(0x1C);
    _writeData(0x0C);
    _writeData(0x0F);
    _writeData(0x08);
    _writeData(0x48);
    _writeData(0x98);
    _writeData(0x37);
    _writeData(0x0A);
    _writeData(0x13);
    _writeData(0x04);
    _writeData(0x11);
    _writeData(0x0D);
    _writeData(0x00);
    // NGAMCTRL(Negative Gamma Control)
    _writeCommand(0xE1);
    _writeData(0x0F);
    _writeData(0x32);
    _writeData(0x2E);
    _writeData(0x0B);
    _writeData(0x0D);
    _writeData(0x05);
    _writeData(0x47);
    _writeData(0x75);
    _writeData(0x37);
    _writeData(0x06);
    _writeData(0x10);
    _writeData(0x03);
    _writeData(0x24);
    _writeData(0x20);
    _writeData(0x00);
    // Digital Gamma Control 1
    _writeCommand(0xE2);
    _writeData(0x0F);
    _writeData(0x32);
    _writeData(0x2E);
    _writeData(0x0B);
    _writeData(0x0D);
    _writeData(0x05);
    _writeData(0x47);
    _writeData(0x75);
    _writeData(0x37);
    _writeData(0x06);
    _writeData(0x10);
    _writeData(0x03);
    _writeData(0x24);
    _writeData(0x20);
    _writeData(0x00);
#endif
    _writeCommand(0x11);  // Sleep OUT
    delay(150);   // wait some time
    _writeCommand(0x29);  // Display ON
  }
  _endTransaction();
}

void ILI9486_SPI::setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  _startTransaction();
  _setWindow(x, y, w, h);
  _endTransaction();
}

void ILI9486_SPI::pushColors(const uint16_t* data, uint16_t n)
{
  _startTransaction();
  _writeColor16(data, n);
  _endTransaction();
}

void ILI9486_SPI::setBackLight(bool lit)
{
  _startTransaction();
  if (_spi16_mode) _writeCommand16(lit ? 0x29 : 0x28);  // Display ON / Display OFF
  else _writeCommand(lit ? 0x29 : 0x28);  // Display ON / Display OFF
  _endTransaction();
}

uint16_t ILI9486_SPI::color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// code extract taken from Adafruit_SPITFT::drawRGBBitmap https://github.com/adafruit/Adafruit-GFX-Library
void ILI9486_SPI::drawRGBBitmap(int16_t x, int16_t y, uint16_t *pcolors, int16_t w, int16_t h)
{
  if (!pcolors) return; // canvas might be empty
  int16_t x2, y2; // Lower-right coord
  if (( x             >= _width ) ||     // Off-edge right
      ( y             >= _height) ||      // " top
      ((x2 = (x + w - 1)) <  0      ) ||  // " left
      ((y2 = (y + h - 1)) <  0)     ) return; // " bottom

  int16_t bx1 = 0, by1 = 0; // Clipped top-left within bitmap
  int16_t saveW = w;        // Save original bitmap width value
  if (x < 0) // Clip left
  {
    w  +=  x;
    bx1 = -x;
    x   =  0;
  }
  if (y < 0) // Clip top
  {
    h  +=  y;
    by1 = -y;
    y   =  0;
  }
  if (x2 >= _width ) w = _width  - x; // Clip right
  if (y2 >= _height) h = _height - y; // Clip bottom

  pcolors += by1 * saveW + bx1; // Offset bitmap ptr to clipped top-left
  _startTransaction();
  _setWindow(x, y, w, h); // Clipped area
  while (h--) // For each (clipped) scanline...
  {
    _writeColor16(pcolors, w); // Push one (clipped) row
    pcolors += saveW; // Advance pointer by one full (unclipped) line
  }
  _endTransaction();
}

// code extract taken from Adafruit_SPITFT::drawRGBBitmap https://github.com/adafruit/Adafruit-GFX-Library
void ILI9486_SPI::drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h)
{
  const uint16_t* pcolors = bitmap;
  int16_t x2, y2; // Lower-right coord
  if (( x             >= _width ) ||     // Off-edge right
      ( y             >= _height) ||      // " top
      ((x2 = (x + w - 1)) <  0      ) ||  // " left
      ((y2 = (y + h - 1)) <  0)     ) return; // " bottom

  int16_t bx1 = 0, by1 = 0; // Clipped top-left within bitmap
  int16_t saveW = w;        // Save original bitmap width value
  if (x < 0) // Clip left
  {
    w  +=  x;
    bx1 = -x;
    x   =  0;
  }
  if (y < 0) // Clip top
  {
    h  +=  y;
    by1 = -y;
    y   =  0;
  }
  if (x2 >= _width ) w = _width  - x; // Clip right
  if (y2 >= _height) h = _height - y; // Clip bottom

  pcolors += by1 * saveW + bx1; // Offset bitmap ptr to clipped top-left
  _startTransaction();
  _setWindow(x, y, w, h); // Clipped area
  while (h--) // For each (clipped) scanline...
  {
    for (int16_t i = 0; i < w; i++) // Push one (clipped) row
    {
      _writeColor16(pgm_read_word(pcolors + i), 1);
    }
    pcolors += saveW; // Advance pointer by one full (unclipped) line
  }
  _endTransaction();
}

// *** private methods

void ILI9486_SPI::_setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint16_t xe = x + w - 1;
  uint16_t ye = y + h - 1;
  if (_spi16_mode)
  {
    _writeCommand16(ILI9486_SPI_CASET);
    uint16_t columns[] = {uint16_t(x >> 8), uint16_t(x & 0xFF), uint16_t(xe >> 8), uint16_t(xe & 0xFF)};
    _writeData16(columns, 4);
    _writeCommand16(ILI9486_SPI_PASET);
    uint16_t rows[] = {uint16_t(y >> 8), uint16_t(y & 0xFF), uint16_t(ye >> 8), uint16_t(ye & 0xFF)};
    _writeData16(rows, 4);
    _writeCommand16(ILI9486_SPI_RAMWR);
  }
  else
  {
    _writeCommand(ILI9486_SPI_CASET);
    _writeData16(x);
    _writeData16(xe);
    _writeCommand(ILI9486_SPI_PASET);
    _writeData16(y);
    _writeData16(ye);
    _writeCommand(ILI9486_SPI_RAMWR);
  }
  _x_address_set = -1;
  _x_address_set = -1;
}

void ILI9486_SPI::_startTransaction()
{
  _spi->beginTransaction(_spi_settings);
  if (_cs >= 0) digitalWrite(_cs, LOW);
}

void ILI9486_SPI::_endTransaction()
{
  if (_cs >= 0) digitalWrite(_cs, HIGH);
  _spi->endTransaction();
}

void ILI9486_SPI::_writeCommand(uint8_t cmd)
{
  digitalWrite(_dc, LOW);
  _spi->transfer(cmd);
  digitalWrite(_dc, HIGH);
}

void ILI9486_SPI::sendCommand(uint8_t cmd)
{
  _startTransaction();
  if (_cs >= 0)
    digitalWrite(_cs, LOW);

  digitalWrite(_dc, LOW);          // Command mode
  _spi->transfer(cmd); // Send the command byte

  digitalWrite(_dc, HIGH);


  if (_cs >= 0)
    digitalWrite(_cs, HIGH);
  _endTransaction();
  
  
/*  digitalWrite(_dc, LOW);
  _spi->transfer(cmd);
  digitalWrite(_dc, HIGH);*/
}
void ILI9486_SPI::_writeCommand16(uint16_t cmd)
{
  digitalWrite(_dc, LOW);
  _writeData16(cmd);
  digitalWrite(_dc, HIGH);
}

void ILI9486_SPI::_writeData(uint8_t data)
{
  digitalWrite(_dc, HIGH);
  _spi->transfer(data);
}

void ILI9486_SPI::_writeData16(uint16_t data)
{
#if (defined (ESP8266) || defined(ESP32))
  _spi->write16(data);
#else
  _spi->transfer(data >> 8);
  _spi->transfer(data);
#endif
}

void ILI9486_SPI::_writeData16(uint16_t data, uint32_t n)
{
#if (defined (ESP8266) || defined(ESP32)) && true // fastest
  uint16_t swapped = ((data << 8) | (data >> 8));
  _spi->writePattern((uint8_t*)&swapped, 2, n);
#elif (defined (ESP8266) || defined(ESP32))
  while (n-- > 0)
  {
    _spi->write16(data);
  }
#else // wdt on ESP8266
  while (n-- > 0)
  {
    _spi->transfer(data >> 8);
    _spi->transfer(data);
  }
#endif
}

void ILI9486_SPI::_writeData16(const uint16_t* data, uint32_t n)
{
#if (defined (ESP8266) || defined(ESP32) || (TEENSYDUINO == 147)) && true // fastest
  static const uint16_t swap_buffer_size = 64; // optimal for ESP8266 SPI
  static const uint32_t max_chunk = swap_buffer_size / 2; // uint16_t's
  uint8_t swap_buffer[swap_buffer_size];
  const uint8_t* p1 = reinterpret_cast<const uint8_t*> (data);
  const uint8_t* p2 = p1 + 1;
  while (n > 0)
  {
    uint32_t chunk = min(max_chunk, n);
    n -= chunk;
    uint8_t* p3 = swap_buffer;
    uint8_t* p4 = p3 + 1;
    uint16_t ncopy = chunk;
    while (ncopy-- > 0)
    {
      *p3 = *p2; p3 += 2; p2 += 2;
      *p4 = *p1; p4 += 2; p1 += 2;
    }
#if (defined (ESP8266) || defined(ESP32))
    _spi->transferBytes(swap_buffer, 0, 2 * chunk);
#elif defined(ARDUINO_ARCH_SAM) // same speed
    _spi->transfer(SS, swap_buffer, 2 * chunk);
#else
    _spi->transfer(swap_buffer, 0, 2 * chunk);
#endif
  }
#else
  while (n-- > 0)
  {
    uint16_t color = (*data++);
#if (defined (ESP8266) || defined(ESP32)) && false // faster
    _spi->write16(color);
#else
    _spi->transfer(color >> 8);
    _spi->transfer(color);
#endif
  }
#endif
}

void ILI9486_SPI::_writeColor16(uint16_t data, uint32_t n)
{
  if (_spi16_mode) return _writeData16(data, n);
#if (defined (ESP8266) || defined(ESP32))
  uint8_t rgb888[] = {uint8_t((data & 0xF800) >> 8), uint8_t((data & 0x07E0) >> 3), uint8_t((data & 0x001F) << 3)};
  _spi->writePattern(rgb888, 3, n);
#else // wdt on ESP8266
  while (n-- > 0)
  {
    _spi->transfer(uint8_t((data & 0xF800) >> 8));
    _spi->transfer(uint8_t((data & 0x07E0) >> 3));
    _spi->transfer(uint8_t((data & 0x001F) << 3));
  }
#endif
}

#if (defined (ESP8266) || defined(ESP32))
#define SPI_WRITE_BYTES(data, n) _spi->transferBytes(data, 0, n)
#elif defined(ARDUINO_ARCH_SAM)
#define SPI_WRITE_BYTES(data, n) _spi->transfer(SS, data, n)
#elif (TEENSYDUINO == 147)
#define SPI_WRITE_BYTES(data, n) _spi->transfer(data, 0, n)
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
#define SPI_WRITE_BYTES(data, n) _spi->write(data, n)
#else
// valid for all other platforms? else comment out next line
#define SPI_WRITE_BYTES(data, n) _spi->transfer(data, n)
#endif


void ILI9486_SPI::_writeColor16(const uint16_t* data, uint32_t n)
{
  if (_spi16_mode) return _writeData16(data, n);
#if defined(SPI_WRITE_BYTES)
  static const uint16_t rgb888_buffer_size = 60; // 64 optimal for ESP8266 SPI
  static const uint32_t max_chunk = rgb888_buffer_size / 3; // rgb888
  uint8_t rgb888_buffer[rgb888_buffer_size];
  while (n > 0)
  {
    uint32_t chunk = min(max_chunk, n);
    n -= chunk;
    uint8_t* p = rgb888_buffer;
    uint16_t ncopy = chunk;
    while (ncopy-- > 0)
    {
      *p++ = uint8_t((*data & 0xF800) >> 8);
      *p++ = uint8_t((*data & 0x07E0) >> 3);
      *p++ = uint8_t((*data & 0x001F) << 3);
      data++;
    }
    SPI_WRITE_BYTES(rgb888_buffer, 3 * chunk);
  }
#else
  while (n-- > 0)
  {
    _spi->transfer(uint8_t((*data & 0xF800) >> 8));
    _spi->transfer(uint8_t((*data & 0x07E0) >> 3));
    _spi->transfer(uint8_t((*data & 0x001F) << 3));
  }
#endif
}

#endif
