#include "../../options.h"
#if DSP_MODEL==DSP_ILI9225
#include "TFT_22_ILI9225Fix.h"

//#define DEBUG
#ifdef DEBUG
    #define DB_PRINT( ... ) { char dbgbuf[60]; sprintf( dbgbuf,   __VA_ARGS__ ) ; Serial.println( dbgbuf ); }
#else
    #define DB_PRINT(  ... ) ;
#endif

#ifndef ARDUINO_STM32_FEATHER
    #include "pins_arduino.h"
    #ifndef RASPI
        #include "wiring_private.h"
    #endif
#endif
#include <limits.h>
#ifdef __AVR__
    #include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
    #include <pgmspace.h>
#endif

// Many (but maybe not all) non-AVR board installs define macros
// for compatibility with existing PROGMEM-reading AVR code.
// Do our own checks and defines here for good measure...

#ifndef pgm_read_byte
    #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
    #define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
    #define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif

// Pointers are a peculiar case...typically 16-bit on AVR boards,
// 32 bits elsewhere.  Try to accommodate both...

#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
    #define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#else
    #define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#endif

// Control pins

#ifdef USE_FAST_PINIO
    #define SPI_DC_HIGH()           *dcport |=  dcpinmask
    #define SPI_DC_LOW()            *dcport &= ~dcpinmask
    #define SPI_CS_HIGH()           *csport |=  cspinmask
    #define SPI_CS_LOW()            *csport &= ~cspinmask
#else
    #define SPI_DC_HIGH()           digitalWrite(_rs, HIGH)
    #define SPI_DC_LOW()            digitalWrite(_rs, LOW)
    #define SPI_CS_HIGH()           digitalWrite(_cs, HIGH)
    #define SPI_CS_LOW()            digitalWrite(_cs, LOW)
#endif

// Software SPI Macros

#ifdef USE_FAST_PINIO
    #define SSPI_MOSI_HIGH()        *mosiport |=  mosipinmask
    #define SSPI_MOSI_LOW()         *mosiport &= ~mosipinmask
    #define SSPI_SCK_HIGH()         *clkport |=  clkpinmask
    #define SSPI_SCK_LOW()          *clkport &= ~clkpinmask
#else
    #define SSPI_MOSI_HIGH()        digitalWrite(_sdi, HIGH)
    #define SSPI_MOSI_LOW()         digitalWrite(_sdi, LOW)
    #define SSPI_SCK_HIGH()         digitalWrite(_clk, HIGH)
    #define SSPI_SCK_LOW()          digitalWrite(_clk, LOW)
#endif

#define SSPI_BEGIN_TRANSACTION()
#define SSPI_END_TRANSACTION()
#define SSPI_WRITE(v)               _spiWrite(v)
#define SSPI_WRITE16(s)             SSPI_WRITE((s) >> 8); SSPI_WRITE(s)
#define SSPI_WRITE32(l)             SSPI_WRITE((l) >> 24); SSPI_WRITE((l) >> 16); SSPI_WRITE((l) >> 8); SSPI_WRITE(l)
#define SSPI_WRITE_PIXELS(c,l)      for(uint32_t i=0; i<(l); i+=2){ SSPI_WRITE(((uint8_t*)(c))[i+1]); SSPI_WRITE(((uint8_t*)(c))[i]); }

// Hardware SPI Macros
#ifndef ESP32
    #ifdef SPI_CHANNEL
        extern SPIClass SPI_CHANNEL; 
        #define SPI_OBJECT  SPI_CHANNEL
    #else
        #define SPI_OBJECT  SPI
    #endif
#else
    #define SPI_OBJECT  _spi
#endif

#if defined (__AVR__) || defined(TEENSYDUINO) || defined(ARDUINO_ARCH_STM32F1)
    #define HSPI_SET_CLOCK() SPI_OBJECT.setClockDivider(SPI_CLOCK_DIV2);
#elif defined (__arm__)
    #define HSPI_SET_CLOCK() SPI_OBJECT.setClockDivider(11);
#elif defined(ESP8266) || defined(ESP32)
    #define HSPI_SET_CLOCK() SPI_OBJECT.setFrequency(SPI_DEFAULT_FREQ);
#elif defined(RASPI)
    #define HSPI_SET_CLOCK() SPI_OBJECT.setClock(SPI_DEFAULT_FREQ);
#elif defined(ARDUINO_ARCH_STM32F1)
    #define HSPI_SET_CLOCK() SPI_OBJECT.setClock(SPI_DEFAULT_FREQ);
#else
    #define HSPI_SET_CLOCK()
#endif

#ifdef SPI_HAS_TRANSACTION
    #define HSPI_BEGIN_TRANSACTION() SPI_OBJECT.beginTransaction(SPISettings(SPI_DEFAULT_FREQ, MSBFIRST, SPI_MODE0))
    #define HSPI_END_TRANSACTION()   SPI_OBJECT.endTransaction()
#else
    #define HSPI_BEGIN_TRANSACTION() HSPI_SET_CLOCK(); SPI_OBJECT.setBitOrder(MSBFIRST); SPI_OBJECT.setDataMode(SPI_MODE0)
    #define HSPI_END_TRANSACTION()
#endif

#ifdef ESP32
    #define SPI_HAS_WRITE_PIXELS
#endif
#if defined(ESP8266) || defined(ESP32)
    // Optimized SPI (ESP8266 and ESP32)
    #define HSPI_READ()              SPI_OBJECT.transfer(0)
    #define HSPI_WRITE(b)            SPI_OBJECT.write(b)
    #define HSPI_WRITE16(s)          SPI_OBJECT.write16(s)
    #define HSPI_WRITE32(l)          SPI_OBJECT.write32(l)
    #ifdef SPI_HAS_WRITE_PIXELS
        #define SPI_MAX_PIXELS_AT_ONCE  32
        #define HSPI_WRITE_PIXELS(c,l)   SPI_OBJECT.writePixels(c,l)
    #else
        #define HSPI_WRITE_PIXELS(c,l)   for(uint32_t i=0; i<((l)/2); i++){ HSPI_WRITE16(((uint16_t*)(c))[i]); }
    #endif
#elif defined ( __STM32F1__ )
    #define HSPI_WRITE(b)            SPI_OBJECT.write(b)
    #define HSPI_WRITE16(s)          SPI_OBJECT.write16(s)

#else
    // Standard Byte-by-Byte SPI

    #if defined(__AVR_ATmega4809__)
    static inline uint8_t _avr_spi_read(void) __attribute__((always_inline));
    static inline uint8_t _avr_spi_read(void) {
        uint8_t r = 0;
        SPI0_DATA = r;
        while(!(SPI0_INTFLAGS & _BV(SPI_IF_bp)));
        r = SPI0_DATA;
        return r;
    }
        #define HSPI_WRITE(b)        {SPI0_DATA = (b); while(!(SPI0_INTFLAGS & _BV(SPI_IF_bp)));}
        // #define HSPI_READ()          _avr_spi_read()
    #elif defined (__AVR__) || defined(TEENSYDUINO)
        static inline uint8_t _avr_spi_read(void) __attribute__((always_inline));
        static inline uint8_t _avr_spi_read(void) {
            uint8_t r = 0;
            SPDR = r;
            while(!(SPSR & _BV(SPIF)));
            r = SPDR;
            return r;
        }
        #define HSPI_WRITE(b)        {SPDR = (b); while(!(SPSR & _BV(SPIF)));}
        // #define HSPI_READ()          _avr_spi_read()
    #else
        #define HSPI_WRITE(b)        SPI_OBJECT.transfer((uint8_t)(b))
        // #define HSPI_READ()          HSPI_WRITE(0)
    #endif
    // #define HSPI_WRITE16(s)          HSPI_WRITE((s) >> 8); HSPI_WRITE(s)
    // #define HSPI_WRITE32(l)          HSPI_WRITE((l) >> 24); HSPI_WRITE((l) >> 16); HSPI_WRITE((l) >> 8); HSPI_WRITE(l)
    // #define HSPI_WRITE_PIXELS(c,l)   for(uint32_t i=0; i<(l); i+=2){ HSPI_WRITE(((uint8_t*)(c))[i+1]); HSPI_WRITE(((uint8_t*)(c))[i]); }
#endif

// Final SPI Macros

#if defined (ARDUINO_ARCH_ARC32)
    #define SPI_DEFAULT_FREQ         16000000
#elif defined (__AVR__) || defined(TEENSYDUINO)
    #define SPI_DEFAULT_FREQ         8000000
#elif defined(ESP8266) || defined(ESP32)
    #define SPI_DEFAULT_FREQ         40000000
#elif defined(RASPI)
    #define SPI_DEFAULT_FREQ         80000000
#elif defined(ARDUINO_ARCH_STM32F1)
    #define SPI_DEFAULT_FREQ         18000000
    //#define SPI_DEFAULT_FREQ         36000000
#else
    #define SPI_DEFAULT_FREQ         24000000
#endif

#define SPI_BEGIN()             if(_clk < 0){SPI_OBJECT.begin();}
#define SPI_BEGIN_TRANSACTION() if(_clk < 0){HSPI_BEGIN_TRANSACTION();}
#define SPI_END_TRANSACTION()   if(_clk < 0){HSPI_END_TRANSACTION();}
// #define SPI_WRITE16(s)          if(_clk < 0){HSPI_WRITE16(s);}else{SSPI_WRITE16(s);}
// #define SPI_WRITE32(l)          if(_clk < 0){HSPI_WRITE32(l);}else{SSPI_WRITE32(l);}
// #define SPI_WRITE_PIXELS(c,l)   if(_clk < 0){HSPI_WRITE_PIXELS(c,l);}else{SSPI_WRITE_PIXELS(c,l);}


// Constructor when using software SPI.  All output pins are configurable.
TFT_22_ILI9225::TFT_22_ILI9225(int8_t rst, int8_t rs, int8_t cs, int8_t sdi, int8_t clk, int8_t led) {
    _rst  = rst;
    _rs   = rs;
    _cs   = cs;
    _sdi  = sdi;
    _clk  = clk;
    _led  = led;
    _brightness = 255; // Set to maximum brightness
    hwSPI = false;
    writeFunctionLevel = 0;
    gfxFont = NULL;
}


// Constructor when using software SPI.  All output pins are configurable. Adds backlight brightness 0-255
TFT_22_ILI9225::TFT_22_ILI9225(int8_t rst, int8_t rs, int8_t cs, int8_t sdi, int8_t clk, int8_t led, uint8_t brightness) {
    _rst  = rst;
    _rs   = rs;
    _cs   = cs;
    _sdi  = sdi;
    _clk  = clk;
    _led  = led;
    _brightness = brightness;
    hwSPI = false;
    writeFunctionLevel = 0;
    gfxFont = NULL;
}


// Constructor when using hardware SPI.  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)
TFT_22_ILI9225::TFT_22_ILI9225(int8_t rst, int8_t rs, int8_t cs, int8_t led) {
    _rst  = rst;
    _rs   = rs;
    _cs   = cs;
    _sdi  = _clk = -1;
    _led  = led;
    _brightness = 255; // Set to maximum brightness
    hwSPI = true;
    writeFunctionLevel = 0;
    gfxFont = NULL;
}


// Constructor when using hardware SPI.  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)
// Adds backlight brightness 0-255
TFT_22_ILI9225::TFT_22_ILI9225(int8_t rst, int8_t rs, int8_t cs, int8_t led, uint8_t brightness) {
    _rst  = rst;
    _rs   = rs;
    _cs   = cs;
    _sdi  = _clk = -1;
    _led  = led;
    _brightness = brightness;
    hwSPI = true;
    writeFunctionLevel = 0;
    gfxFont = NULL;
}

#ifdef ESP32
void TFT_22_ILI9225::begin(SPIClass &spi)
#else
void TFT_22_ILI9225::begin()
#endif
{
#ifdef ESP32
    _spi = spi;
#endif
    // Set up reset pin
    if (_rst > 0) {
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, LOW);
    }
    // Set up backlight pin, turn off initially
    if (_led > 0) {
        pinMode(_led, OUTPUT);
        setBacklight(false);
    }

    // Control pins
    pinMode(_rs, OUTPUT);
    digitalWrite(_rs, LOW);
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);

#ifdef USE_FAST_PINIO
    csport    = portOutputRegister(digitalPinToPort(_cs));
    cspinmask = digitalPinToBitMask(_cs);
    dcport    = portOutputRegister(digitalPinToPort(_rs));
    dcpinmask = digitalPinToBitMask(_rs);
#endif

    // Software SPI
    if (_clk >= 0) {
        pinMode(_sdi, OUTPUT);
        digitalWrite(_sdi, LOW);
        pinMode(_clk, OUTPUT);
        digitalWrite(_clk, HIGH);
#ifdef USE_FAST_PINIO
        clkport     = portOutputRegister(digitalPinToPort(_clk));
        clkpinmask  = digitalPinToBitMask(_clk);
        mosiport    = portOutputRegister(digitalPinToPort(_sdi));
        mosipinmask = digitalPinToBitMask(_sdi);
        SSPI_SCK_LOW();
        SSPI_MOSI_LOW();
    } else {
        clkport     = 0;
        clkpinmask  = 0;
        mosiport    = 0;
        mosipinmask = 0;
#endif
    }

    // Hardware SPI
    SPI_BEGIN();

    // Initialization Code
    if (_rst > 0) {
        digitalWrite(_rst, HIGH); // Pull the reset pin high to release the ILI9225C from the reset status
        delay(1); 
        digitalWrite(_rst, LOW); // Pull the reset pin low to reset ILI9225
        delay(10);
        digitalWrite(_rst, HIGH); // Pull the reset pin high to release the ILI9225C from the reset status
        delay(50);
    }

    /* Start Initial Sequence */

    /* Set SS bit and direction output from S528 to S1 */
    startWrite();
    _writeRegister(ILI9225_POWER_CTRL1, 0x0000); // Set SAP,DSTB,STB
    _writeRegister(ILI9225_POWER_CTRL2, 0x0000); // Set APON,PON,AON,VCI1EN,VC
    _writeRegister(ILI9225_POWER_CTRL3, 0x0000); // Set BT,DC1,DC2,DC3
    _writeRegister(ILI9225_POWER_CTRL4, 0x0000); // Set GVDD
    _writeRegister(ILI9225_POWER_CTRL5, 0x0000); // Set VCOMH/VCOML voltage
    endWrite();
    delay(40); 

    // Power-on sequence
    startWrite();
    _writeRegister(ILI9225_POWER_CTRL2, 0x0018); // Set APON,PON,AON,VCI1EN,VC
    _writeRegister(ILI9225_POWER_CTRL3, 0x6121); // Set BT,DC1,DC2,DC3
    _writeRegister(ILI9225_POWER_CTRL4, 0x006F); // Set GVDD   /*007F 0088 */
    _writeRegister(ILI9225_POWER_CTRL5, 0x495F); // Set VCOMH/VCOML voltage
    _writeRegister(ILI9225_POWER_CTRL1, 0x0800); // Set SAP,DSTB,STB
    endWrite();
    delay(10);
    startWrite();
    _writeRegister(ILI9225_POWER_CTRL2, 0x103B); // Set APON,PON,AON,VCI1EN,VC
    endWrite();
    delay(50);

    startWrite();
    _writeRegister(ILI9225_DRIVER_OUTPUT_CTRL, 0x011C); // set the display line number and display direction
    _writeRegister(ILI9225_LCD_AC_DRIVING_CTRL, 0x0100); // set 1 line inversion
    _writeRegister(ILI9225_ENTRY_MODE, 0x1038); // set GRAM write direction and BGR=1.
    _writeRegister(ILI9225_DISP_CTRL1, 0x0000); // Display off
    _writeRegister(ILI9225_BLANK_PERIOD_CTRL1, 0x0808); // set the back porch and front porch
    _writeRegister(ILI9225_FRAME_CYCLE_CTRL, 0x1100); // set the clocks number per line
    _writeRegister(ILI9225_INTERFACE_CTRL, 0x0000); // CPU interface
    _writeRegister(ILI9225_OSC_CTRL, 0x0D01); // Set Osc  /*0e01*/
    _writeRegister(ILI9225_VCI_RECYCLING, 0x0020); // Set VCI recycling
    _writeRegister(ILI9225_RAM_ADDR_SET1, 0x0000); // RAM Address
    _writeRegister(ILI9225_RAM_ADDR_SET2, 0x0000); // RAM Address

    /* Set GRAM area */
    _writeRegister(ILI9225_GATE_SCAN_CTRL, 0x0000); 
    _writeRegister(ILI9225_VERTICAL_SCROLL_CTRL1, 0x00DB); 
    _writeRegister(ILI9225_VERTICAL_SCROLL_CTRL2, 0x0000); 
    _writeRegister(ILI9225_VERTICAL_SCROLL_CTRL3, 0x0000); 
    _writeRegister(ILI9225_PARTIAL_DRIVING_POS1, 0x00DB); 
    _writeRegister(ILI9225_PARTIAL_DRIVING_POS2, 0x0000); 
    _writeRegister(ILI9225_HORIZONTAL_WINDOW_ADDR1, 0x00AF); 
    _writeRegister(ILI9225_HORIZONTAL_WINDOW_ADDR2, 0x0000); 
    _writeRegister(ILI9225_VERTICAL_WINDOW_ADDR1, 0x00DB); 
    _writeRegister(ILI9225_VERTICAL_WINDOW_ADDR2, 0x0000); 

    /* Set GAMMA curve */
    _writeRegister(ILI9225_GAMMA_CTRL1, 0x0000); 
    _writeRegister(ILI9225_GAMMA_CTRL2, 0x0808); 
    _writeRegister(ILI9225_GAMMA_CTRL3, 0x080A); 
    _writeRegister(ILI9225_GAMMA_CTRL4, 0x000A); 
    _writeRegister(ILI9225_GAMMA_CTRL5, 0x0A08); 
    _writeRegister(ILI9225_GAMMA_CTRL6, 0x0808); 
    _writeRegister(ILI9225_GAMMA_CTRL7, 0x0000); 
    _writeRegister(ILI9225_GAMMA_CTRL8, 0x0A00); 
    _writeRegister(ILI9225_GAMMA_CTRL9, 0x0710); 
    _writeRegister(ILI9225_GAMMA_CTRL10, 0x0710); 

    _writeRegister(ILI9225_DISP_CTRL1, 0x0012); 
    endWrite();
    delay(50); 
    startWrite();
    _writeRegister(ILI9225_DISP_CTRL1, 0x1017);
    endWrite();

    // Turn on backlight
    setBacklight(true);
    setOrientation(0);

    // Initialize variables
    setBackgroundColor( COLOR_BLACK );

    clear();
}


void TFT_22_ILI9225::_spiWrite(uint8_t b) {
    if (_clk < 0) {
        HSPI_WRITE(b);
        return;
    }
    // Fast SPI bitbang swiped from LPD8806 library
    for (uint8_t bit = 0x80; bit; bit >>= 1) {
        if ((b) & bit) {
            SSPI_MOSI_HIGH();
        } else {
            SSPI_MOSI_LOW();
        }
        SSPI_SCK_HIGH();
        SSPI_SCK_LOW();
    }
}


void TFT_22_ILI9225::_spiWrite16(uint16_t s) {
    // Attempt to use HSPI_WRITE16 if available
#ifdef HSPI_WRITE16
    if (_clk < 0) {
        HSPI_WRITE16(s);
        return;
    }
#endif
    // Fallback to SSPI_WRITE16 if HSPI_WRITE16 not available
    SSPI_WRITE16(s);
}


void TFT_22_ILI9225::_spiWriteCommand(uint8_t c) {
    SPI_DC_LOW();
    SPI_CS_LOW();
    _spiWrite(c);
    SPI_CS_HIGH();
}


void TFT_22_ILI9225::_spiWriteData(uint8_t c) {
    SPI_DC_HIGH();
    SPI_CS_LOW();
    _spiWrite(c);
    SPI_CS_HIGH();
}


void TFT_22_ILI9225::_orientCoordinates(uint16_t &x1, uint16_t &y1) {

    switch (_orientation) {
        case 0:  // ok
            break;
        case 1: // ok
            y1 = _maxY - y1 - 1;
            _swap(x1, y1);
            break;
        case 2: // ok
            x1 = _maxX - x1 - 1;
            y1 = _maxY - y1 - 1;
            break;
        case 3: // ok
            x1 = _maxX - x1 - 1;
            _swap(x1, y1);
            break;
    }
}


void TFT_22_ILI9225::_setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    _setWindow( x0, y0, x1, y1, TopDown_L2R ); // default for drawing characters
}


void TFT_22_ILI9225::_setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, autoIncMode_t mode) {
    DB_PRINT( "setWindows( x0=%d, y0=%d, x1=%d, y1=%d, mode=%d", x0,y0,x1,y1,mode );
    
    // clip to TFT-Dimensions
    x0 = min( x0, (uint16_t) (_maxX-1) );
    x1 = min( x1, (uint16_t) (_maxX-1) );
    y0 = min( y0, (uint16_t) (_maxY-1) );
    y1 = min( y1, (uint16_t) (_maxY-1) );
    _orientCoordinates(x0, y0);
    _orientCoordinates(x1, y1);

    if (x1<x0) _swap(x0, x1);
    if (y1<y0) _swap(y0, y1);
    
    startWrite();
    // autoincrement mode
    if ( _orientation > 0 ) mode = modeTab[_orientation-1][mode];
    _writeRegister(ILI9225_ENTRY_MODE, 0x1000 | ( mode<<3) );
    _writeRegister(ILI9225_HORIZONTAL_WINDOW_ADDR1,x1);
    _writeRegister(ILI9225_HORIZONTAL_WINDOW_ADDR2,x0);

    _writeRegister(ILI9225_VERTICAL_WINDOW_ADDR1,y1);
    _writeRegister(ILI9225_VERTICAL_WINDOW_ADDR2,y0);
    DB_PRINT( "gedreht: x0=%d, y0=%d, x1=%d, y1=%d, mode=%d", x0,y0,x1,y1,mode );
    // starting position within window and increment/decrement direction
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
    _writeCommand16( ILI9225_GRAM_DATA_REG );

    //_writeRegister(ILI9225_RAM_ADDR_SET1,x0);
    //_writeRegister(ILI9225_RAM_ADDR_SET2,y0);

    //_writeCommand(0x00, 0x22);

    endWrite();
}


void TFT_22_ILI9225::_resetWindow() {
    _writeRegister(ILI9225_HORIZONTAL_WINDOW_ADDR1, 0x00AF); 
    _writeRegister(ILI9225_HORIZONTAL_WINDOW_ADDR2, 0x0000); 
    _writeRegister(ILI9225_VERTICAL_WINDOW_ADDR1, 0x00DB); 
    _writeRegister(ILI9225_VERTICAL_WINDOW_ADDR2, 0x0000); 

}


void TFT_22_ILI9225::clear() {
    uint8_t old = _orientation;
    setOrientation(0);
    fillRectangle(0, 0, _maxX - 1, _maxY - 1, COLOR_BLACK);
    setOrientation(old);
    delay(10);
}


void TFT_22_ILI9225::invert(boolean flag) {
    startWrite();
    _writeCommand16(flag ? ILI9225C_INVON : ILI9225C_INVOFF);
    //_writeCommand(0x00, flag ? ILI9225C_INVON : ILI9225C_INVOFF);
    endWrite();
}


void TFT_22_ILI9225::setBacklight(boolean flag) {
    blState = flag;
#ifndef ESP32
    if (_led) analogWrite(_led, blState ? _brightness : 0);
#endif
}


void TFT_22_ILI9225::setBacklightBrightness(uint8_t brightness) {
    _brightness = brightness;
    setBacklight(blState);
}


void TFT_22_ILI9225::setDisplay(boolean flag) {
    if (flag) {
        startWrite();
        _writeRegister(0x00ff, 0x0000);
        _writeRegister(ILI9225_POWER_CTRL1, 0x0000);
        endWrite();
        delay(50);
        startWrite();
        _writeRegister(ILI9225_DISP_CTRL1, 0x1017);
        endWrite();
        delay(200);
    } else {
        startWrite();
        _writeRegister(0x00ff, 0x0000);
        _writeRegister(ILI9225_DISP_CTRL1, 0x0000);
        endWrite();
        delay(50);
        startWrite();
        _writeRegister(ILI9225_POWER_CTRL1, 0x0003);
        endWrite();
        delay(200);
    }
}


void TFT_22_ILI9225::setOrientation(uint8_t orientation) {

    _orientation = orientation % 4;

    switch (_orientation) {
    case 0:
        _maxX = ILI9225_LCD_WIDTH;
        _maxY = ILI9225_LCD_HEIGHT;
        break;
    case 1:
        _maxX = ILI9225_LCD_HEIGHT;
        _maxY = ILI9225_LCD_WIDTH;
        break;
    case 2:
        _maxX = ILI9225_LCD_WIDTH;
        _maxY = ILI9225_LCD_HEIGHT;
        break;
    case 3:
        _maxX = ILI9225_LCD_HEIGHT;
        _maxY = ILI9225_LCD_WIDTH;
        break;
    }
}


uint8_t TFT_22_ILI9225::getOrientation() {
    return _orientation;
}


void TFT_22_ILI9225::drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    startWrite();
    drawLine(x1, y1, x1, y2, color);
    drawLine(x1, y1, x2, y1, color);
    drawLine(x1, y2, x2, y2, color);
    drawLine(x2, y1, x2, y2, color);
    endWrite();
}


void TFT_22_ILI9225::fillRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {

    _setWindow(x1, y1, x2, y2);

    startWrite();
    for (uint16_t t=(y2 - y1 + 1) * (x2 - x1 + 1); t > 0; t--)
        _writeData16(color);
    endWrite();
    _resetWindow();
}


void TFT_22_ILI9225::drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    startWrite();

    drawPixel(x0, y0 + r, color);
    drawPixel(x0, y0-  r, color);
    drawPixel(x0 + r, y0, color);
    drawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
    endWrite();
}


void TFT_22_ILI9225::fillCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint16_t color) {

    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    startWrite();
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawLine(x0 + x, y0 + y, x0 - x, y0 + y, color); // bottom
        drawLine(x0 + x, y0 - y, x0 - x, y0 - y, color); // top
        drawLine(x0 + y, y0 - x, x0 + y, y0 + x, color); // right
        drawLine(x0 - y, y0 - x, x0 - y, y0 + x, color); // left
    }
    endWrite();
    fillRectangle(x0-x, y0-y, x0+x, y0+y, color);
}


void TFT_22_ILI9225::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {

    // Classic Bresenham algorithm
    int16_t steep = abs((int16_t)(y2 - y1)) > abs((int16_t)(x2 - x1));

    int16_t dx, dy;

    if (steep) {
        _swap(x1, y1);
        _swap(x2, y2);
    }

    if (x1 > x2) {
        _swap(x1, x2);
        _swap(y1, y2);
    }

    dx = x2 - x1;
    dy = abs((int16_t)(y2 - y1));

    int16_t err = dx / 2;
    int16_t ystep;

    if (y1 < y2) ystep = 1;
    else ystep = -1;

    startWrite();
    for (; x1<=x2; x1++) {
        if (steep) drawPixel(y1, x1, color);
        else       drawPixel(x1, y1, color);

        err -= dy;
        if (err < 0) {
            y1 += ystep;
            err += dx;
        }
    }
    endWrite();
}


void TFT_22_ILI9225::drawPixel(uint16_t x1, uint16_t y1, uint16_t color) {

    if((x1 >= _maxX) || (y1 >= _maxY)) return;

    // _setWindow(x1, y1, x1+1, y1+1);
    // _orientCoordinates(x1, y1);
    // startWrite();
    // //_writeData(color >> 8, color);
    // _writeData16(color);
    // endWrite();

    _orientCoordinates(x1, y1);
    startWrite();
    _writeRegister(ILI9225_RAM_ADDR_SET1,x1);
    _writeRegister(ILI9225_RAM_ADDR_SET2,y1);
    _writeRegister(ILI9225_GRAM_DATA_REG,color);
    
    endWrite();
}


uint16_t TFT_22_ILI9225::maxX() {
    return _maxX;
}


uint16_t TFT_22_ILI9225::maxY() {
    return _maxY;
}


uint16_t TFT_22_ILI9225::setColor(uint8_t red8, uint8_t green8, uint8_t blue8) {
    // rgb16 = red5 green6 blue5
    return (red8 >> 3) << 11 | (green8 >> 2) << 5 | (blue8 >> 3);
}


void TFT_22_ILI9225::splitColor(uint16_t rgb, uint8_t &red, uint8_t &green, uint8_t &blue) {
    // rgb16 = red5 green6 blue5
    red   = (rgb & 0b1111100000000000) >> 11 << 3;
    green = (rgb & 0b0000011111100000) >>  5 << 2;
    blue  = (rgb & 0b0000000000011111)       << 3;
}


void TFT_22_ILI9225::_swap(uint16_t &a, uint16_t &b) {
    uint16_t w = a;
    a = b;
    b = w;
}


// Utilities

void TFT_22_ILI9225::_writeCommand16(uint16_t command) {
    SPI_DC_LOW();
    SPI_CS_LOW();
    if ( _clk < 0 ) {
# ifdef HSPI_WRITE16
        HSPI_WRITE16(command);
#else 
        HSPI_WRITE(command >> 8);
        HSPI_WRITE(0x00ff & command);
#endif
    } else {
        // Fast SPI bitbang swiped from LPD8806 library
        for (uint16_t bit = 0x8000; bit; bit >>= 1) {
            if ((command) & bit) {
                SSPI_MOSI_HIGH();
            } else {
                SSPI_MOSI_LOW();
            }
            SSPI_SCK_HIGH();
            SSPI_SCK_LOW();
        }
    }
    SPI_CS_HIGH();
}


void TFT_22_ILI9225::_writeData16(uint16_t data) {
    SPI_DC_HIGH();
    SPI_CS_LOW();
    if (_clk < 0) {
# ifdef HSPI_WRITE16
        HSPI_WRITE16(data);
#else 
        HSPI_WRITE(data >> 8);
        HSPI_WRITE(0x00ff & data);
#endif
    } else {
        // Fast SPI bitbang swiped from LPD8806 library
        for (uint16_t bit = 0x8000; bit; bit >>= 1) {
            if((data) & bit) {
                SSPI_MOSI_HIGH();
            } else {
                SSPI_MOSI_LOW();
            }
            SSPI_SCK_HIGH();
            SSPI_SCK_LOW();
        }
    }
    SPI_CS_HIGH();
}


/*
void TFT_22_ILI9225::_writeData(uint8_t HI, uint8_t LO) {
    _spiWriteData(HI);
    _spiWriteData(LO);
}

void TFT_22_ILI9225::_writeCommand(uint8_t HI, uint8_t LO) {
    _spiWriteCommand(HI);
    _spiWriteCommand(LO);
}
*/


void TFT_22_ILI9225::_writeRegister(uint16_t reg, uint16_t data) {
    _writeCommand16(reg);
    _writeData16(data);
}


void TFT_22_ILI9225::drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color) {
    startWrite();
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x3, y3, color);
    drawLine(x3, y3, x1, y1, color);
    endWrite();
}


void TFT_22_ILI9225::fillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color) {

    uint16_t a, b, y, last;

    // Sort coordinates by Y order (y3 >= y2 >= y1)
    if (y1 > y2) {
        _swap(y1, y2); _swap(x1, x2);
    }
    if (y2 > y3) {
        _swap(y3, y2); _swap(x3, x2);
    }
    if (y1 > y2) {
        _swap(y1, y2); _swap(x1, x2);
    }

    startWrite();
    if (y1 == y3) { // Handle awkward all-on-same-line case as its own thing
        a = b = x1;
        if (x2 < a)      a = x2;
        else if (x2 > b) b = x2;
        if (x3 < a)      a = x3;
        else if (x3 > b) b = x3;
            drawLine(a, y1, b, y1, color);
        return;
    }

    int16_t dx11 = x2 - x1,
            dy11 = y2 - y1,
            dx12 = x3 - x1,
            dy12 = y3 - y1,
            dx22 = x3 - x2,
            dy22 = y3 - y2;
    int32_t sa   = 0,
            sb   = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y2=y3 (flat-bottomed triangle), the scanline y2
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y2 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y1=y2
    // (flat-topped triangle).
    if (y2 == y3) last = y2;   // Include y2 scanline
    else          last = y2 - 1; // Skip it

    for (y = y1; y <= last; y++) {
        a   = x1 + sa / dy11;
        b   = x1 + sb / dy12;
        sa += dx11;
        sb += dx12;
        // longhand:
        // a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        // b = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
        if (a > b) _swap(a,b);
        drawLine(a, y, b, y, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y2=y3.
    sa = dx22 * (y - y2);
    sb = dx12 * (y - y1);
    for (; y<=y3; y++) {
        a   = x2 + sa / dy22;
        b   = x1 + sb / dy12;
        sa += dx22;
        sb += dx12;
        // longhand:
        // a = x2 + (x3 - x2) * (y - y2) / (y3 - y2);
        // b = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
        if (a > b) _swap(a,b);
            drawLine(a, y, b, y, color);
    }
    endWrite();
}


void TFT_22_ILI9225::setBackgroundColor(uint16_t color) {
    _bgColor = color;
}


void TFT_22_ILI9225::setFont(uint8_t* font, bool monoSp) {

    cfont.font     = font;
    cfont.width    = readFontByte(0);
    cfont.height   = readFontByte(1);
    cfont.offset   = readFontByte(2);
    cfont.numchars = readFontByte(3);
    cfont.nbrows   = cfont.height / 8;
    cfont.monoSp   = monoSp;

    if (cfont.height % 8) cfont.nbrows++;  // Set number of bytes used by height of font in multiples of 8
}


_currentFont TFT_22_ILI9225::getFont() {
    return cfont;
}


uint16_t TFT_22_ILI9225::drawText(uint16_t x, uint16_t y, STRING s, uint16_t color) {

    uint16_t currx = x;

    // Print every character in string
#ifdef USE_STRING_CLASS
    for (uint8_t k = 0; k < s.length(); k++) {
        currx += drawChar(currx, y, s.charAt(k), color) + 1;
    }
#else
    for (uint8_t k = 0; k < strlen(s); k++) {
        currx += drawChar(currx, y, s[k], color) + 1;
    }
#endif
    return currx;
}


uint16_t TFT_22_ILI9225::getTextWidth( STRING s ) {

    uint16_t width = 0;
    // Count every character in string ( +1 for spacing )
#ifdef USE_STRING_CLASS
    for (uint8_t k = 0; k < s.length(); k++) {
        width += getCharWidth(s.charAt(k) ) + 1;
    }
#else
    for (uint8_t k = 0; k < strlen(s); k++) {
        width += getCharWidth(s[k]) + 1;
    }
#endif
    return width;
}


uint16_t TFT_22_ILI9225::drawChar(uint16_t x, uint16_t y, uint16_t ch, uint16_t color) {

    uint8_t charData, charWidth;
    uint8_t h, i, j;
    uint16_t charOffset;
    bool fastMode;

    charOffset = (cfont.width * cfont.nbrows) + 1;  // bytes used by each character
    charOffset = (charOffset * (ch - cfont.offset)) + FONT_HEADER_SIZE;  // char offset (add 4 for font header)
    if ( cfont.monoSp ) charWidth = cfont.width;      // monospaced: get char width from font
    else                charWidth  = readFontByte(charOffset);  // get chracter width from 1st byte
    charOffset++;  // increment pointer to first character data byte

    startWrite();
    
    // use autoincrement/decrement feature, if character fits completely on screen
    fastMode = ( (x+charWidth+1) < _maxX && (y+cfont.height-1) < _maxY );
    
    if ( fastMode ) _setWindow( x,y,x+charWidth+1, y+cfont.height-1 );  // set character Window

    for (i = 0; i <= charWidth; i++) {  // each font "column" (+1 blank column for spacing)
        h = 0;  // keep track of char height
        for (j = 0; j < cfont.nbrows; j++)     {  // each column byte
            if (i == charWidth) charData = (uint8_t)0x0; // Insert blank column
            else                charData = readFontByte(charOffset);
            charOffset++;
            
            // Process every row in font character
            for (uint8_t k = 0; k < 8; k++) {
                if (h >= cfont.height ) break;  // No need to process excess bits
                if (fastMode ) _writeData16( bitRead(charData, k)?color:_bgColor );
                else drawPixel( x + i, y + (j * 8) + k, bitRead(charData, k)?color:_bgColor );
                h++;
            }
        }
    }
    endWrite();
    _resetWindow();
    return charWidth;
}


uint16_t TFT_22_ILI9225::getCharWidth(uint16_t ch) {
    uint16_t charOffset;
    charOffset = (cfont.width * cfont.nbrows) + 1;  // bytes used by each character
    charOffset = (charOffset * (ch - cfont.offset)) + FONT_HEADER_SIZE;  // char offset (add 4 for font header)

    return readFontByte(charOffset);  // get font width from 1st byte
}


// Draw a 1-bit image (bitmap) at the specified (x,y) position from the
// provided bitmap buffer (must be PROGMEM memory) using the specified
// foreground color (unset bits are transparent).
void TFT_22_ILI9225::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
    _drawBitmap( x,  y, bitmap,  w,  h, color,  0, true, true, false );
}

// Draw a 1-bit image (bitmap) at the specified (x,y) position from the
// provided bitmap buffer (must be PROGMEM memory) using the specified
// foreground (for set bits) and background (for clear bits) colors.
void TFT_22_ILI9225::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {
    _drawBitmap( x,  y, bitmap,  w,  h, color,  bg, false, true, false );
}


// drawBitmap() variant for RAM-resident (not PROGMEM) bitmaps.
void TFT_22_ILI9225::drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
    _drawBitmap( x,  y, bitmap,  w,  h, color,  0, true, false, false );
}


// drawBitmap() variant w/background for RAM-resident (not PROGMEM) bitmaps.
void TFT_22_ILI9225::drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {
    _drawBitmap( x,  y, bitmap,  w,  h, color,  bg, false, false, false );
}


//Draw XBitMap Files (*.xbm), exported from GIMP,
//Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
//C Array can be directly used with this function
void TFT_22_ILI9225::drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
    _drawBitmap( x,  y, bitmap,  w,  h, color,  0, true, true, true );
}


void TFT_22_ILI9225::drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {
    _drawBitmap( x,  y, bitmap,  w,  h, color,  bg, false, true, true );
}


// internal function for drawing bitmaps with/without transparent bg, or from ram or progmem
void TFT_22_ILI9225::_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg, bool transparent, bool progmem,bool Xbit) {
    bool noAutoInc = false;     // Flag set when transparent pixel was 'written'
    int16_t i, j, byteWidth = (w + 7) / 8;
    int16_t wx0, wy0, wx1, wy1, wh;  // Window-position and size
	// int16_t ww;
    uint8_t byte, maskBit;
	  byte = 0;
    maskBit = Xbit? 0x01:0x80;
    // adjust window hight/width to displaydimensions
    DB_PRINT( "DrawBitmap.. maxX=%d, maxY=%d", _maxX,_maxY );
    wx0 = x < 0 ? 0 : x;
    wy0 = y < 0 ? 0 : y;
    wx1 = (x + w > _maxX ?_maxX : x + w ) - 1;
    wy1 = (y + h > _maxY ?_maxY : y + h ) - 1;
    wh  = wy1 - wy0 + 1;
    // ww  = wx1 - wx0 + 1;
    _setWindow(wx0, wy0, wx1, wy1, L2R_TopDown);
    startWrite();
    for (j = y>=0?0:-y; j < (y>=0?0:-y)+wh; j++) {
        for (i = 0; i < w; i++ ) {
            if (i & 7) { 
                if ( Xbit ) byte >>=1; else byte <<= 1; 
            }
            else {
                if ( progmem ) byte   = pgm_read_byte(bitmap + j * byteWidth + i / 8);
                else           byte   = bitmap[j * byteWidth + i / 8];
            }
            if ( x+i >= wx0 && x+i <= wx1 ) {
                // write only if pixel is within window
                if (byte & maskBit) {
                    if (noAutoInc) {
                        //there was a transparent area, set pixelkoordinates again
                        drawPixel(x + i, y + j, color);
                        noAutoInc = false;
                    }
                    else  { 
                        _writeData16(color);
                    }
                }
                else  {
                    if (transparent) noAutoInc = true; // no autoincrement in transparent area!
                    else _writeData16( bg);
                }
            }
        }
    }
    endWrite();
    _resetWindow();
}

//High speed color bitmap
void TFT_22_ILI9225::drawBitmap(uint16_t x1, uint16_t y1, const uint16_t** bitmap, int16_t w, int16_t h) {
    startWrite();
    _setWindow(x1, y1, x1+w-1, y1+h-1, L2R_TopDown);
    SPI_DC_HIGH();
    SPI_CS_LOW();
    for (uint16_t y = 0; y < h; y++) {
#ifdef HSPI_WRITE_PIXELS
        if (_clk < 0) {
            HSPI_WRITE_PIXELS(bitmap[y], w * sizeof(uint16_t));
            continue;
        }
#endif
        for (uint16_t x = 0; x < w; x++) {
            _spiWrite16(bitmap[y][x]);
        }
    }
    SPI_CS_HIGH();
    endWrite();
    _resetWindow();
}


//High speed color bitmap
void TFT_22_ILI9225::drawBitmap(uint16_t x1, uint16_t y1, uint16_t** bitmap, int16_t w, int16_t h) {
    startWrite();
    _setWindow(x1, y1, x1+w-1, y1+h-1, L2R_TopDown);
    SPI_DC_HIGH();
    SPI_CS_LOW();
    for (uint16_t y = 0; y < h; y++) {
#ifdef HSPI_WRITE_PIXELS
        if (_clk < 0) {
            HSPI_WRITE_PIXELS(bitmap[y], w * sizeof(uint16_t));
            continue;
        }
#endif
        for (uint16_t x = 0; x < w; x++) {
            _spiWrite16(bitmap[y][x]);
        }
    }
    SPI_CS_HIGH();
    endWrite();
    _resetWindow();
}


//1-D array High speed color bitmap
void TFT_22_ILI9225::drawBitmap(uint16_t x1, uint16_t y1, const uint16_t* bitmap, int16_t w, int16_t h) {
    startWrite();
    _setWindow(x1, y1, x1+w-1, y1+h-1, L2R_TopDown);
    SPI_DC_HIGH();
    SPI_CS_LOW();
#ifdef HSPI_WRITE_PIXELS
    if (_clk < 0) {
        HSPI_WRITE_PIXELS(bitmap, w * h * sizeof(uint16_t));
    } else
#endif
    for (uint16_t i = 0; i < h * w; ++i) {
        _spiWrite16(bitmap[i]);
    }
    SPI_CS_HIGH();
    endWrite();
    _resetWindow();
}


//1-D array High speed color bitmap
void TFT_22_ILI9225::drawBitmap(uint16_t x1, uint16_t y1, uint16_t* bitmap, int16_t w, int16_t h) {
    startWrite();
    _setWindow(x1, y1, x1+w-1, y1+h-1, L2R_TopDown);
    SPI_DC_HIGH();
    SPI_CS_LOW();
#ifdef HSPI_WRITE_PIXELS
    if (_clk < 0) {
        HSPI_WRITE_PIXELS(bitmap, w * h * sizeof(uint16_t));
    } else
#endif
    for (uint16_t i = 0; i < h * w; ++i) {
        _spiWrite16(bitmap[i]);
    }
    SPI_CS_HIGH();
    endWrite();
    _resetWindow();
}


void TFT_22_ILI9225::startWrite(void) {
    if (writeFunctionLevel++ == 0) {
        SPI_BEGIN_TRANSACTION();
        SPI_CS_LOW();
    }
}


void TFT_22_ILI9225::endWrite(void) {
    if (--writeFunctionLevel == 0) {
        SPI_CS_HIGH();
        SPI_END_TRANSACTION();
    }
}


// TEXT- AND CHARACTER-HANDLING FUNCTIONS ----------------------------------

void TFT_22_ILI9225::setGFXFont(const GFXfont *f) {
    gfxFont = (GFXfont *)f;
}


// Draw a string
void TFT_22_ILI9225::drawGFXText(int16_t x, int16_t y, STRING s, uint16_t color) {

    int16_t currx = x;

    if(gfxFont) {
        // Print every character in string
#ifdef USE_STRING_CLASS
        for (uint8_t k = 0; k < s.length(); k++) {
            currx += drawGFXChar(currx, y, s.charAt(k), color) + 1;
        }
#else
        for (uint8_t k = 0; k < strlen(s); k++) {
            currx += drawGFXChar(currx, y, s[k], color) + 1;
        }
#endif
    }
}


// Draw a character
uint16_t TFT_22_ILI9225::drawGFXChar(int16_t x, int16_t y, unsigned char c, uint16_t color) {

    c -= (uint8_t)pgm_read_byte(&gfxFont->first);
    GFXglyph *glyph  = &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
    uint8_t  *bitmap = (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);

    uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
    uint8_t  w  = pgm_read_byte(&glyph->width),
             h  = pgm_read_byte(&glyph->height),
             xa = pgm_read_byte(&glyph->xAdvance);
    int8_t   xo = pgm_read_byte(&glyph->xOffset),
             yo = pgm_read_byte(&glyph->yOffset);
    uint8_t  xx, yy, bits = 0, bit = 0;
    // Add character clipping here one day

    startWrite();
    _setWindow( x-1,y+2,x+w+xo+1, y+yo-2 );  // set character Window
    for(yy=0; yy<h; yy++) {
        for(xx=0; xx<w; xx++) {
            if(!(bit++ & 7)) {
                bits = pgm_read_byte(&bitmap[bo++]);
            }
            if(bits & 0x80) {
                drawPixel(x+xo+xx, y+yo+yy, color);
            }
            bits <<= 1;
        }
    }
    endWrite();
    _resetWindow();
    return (uint16_t)xa;
}


void TFT_22_ILI9225::getGFXCharExtent(uint8_t c, int16_t *gw, int16_t *gh, int16_t *xa) {
    uint8_t first = pgm_read_byte(&gfxFont->first),
            last  = pgm_read_byte(&gfxFont->last);
    // Char present in this font?
    if((c >= first) && (c <= last)) {
        GFXglyph *glyph = &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c - first]);
        *gw = pgm_read_byte(&glyph->width);
        *gh = pgm_read_byte(&glyph->height);
        *xa = pgm_read_byte(&glyph->xAdvance);
        // int8_t  xo = pgm_read_byte(&glyph->xOffset),
        //         yo = pgm_read_byte(&glyph->yOffset);
    }
}

void TFT_22_ILI9225::getGFXTextExtent(STRING str, int16_t x, int16_t y, int16_t *w, int16_t *h) {
    *w  = *h = 0;
#ifdef USE_STRING_CLASS
    for (uint8_t k = 0; k < str.length(); k++) {
        uint8_t c = str.charAt(k);
#else
    for (uint8_t k = 0; k < strlen(str); k++) {
        uint8_t c = str[k];
#endif
        int16_t gw, gh, xa;
        getGFXCharExtent(c, &gw, &gh, &xa);
        if(gh > *h) {
            *h = gh;
        }
        *w += xa;
    }
}

GFXcanvas16::GFXcanvas16(uint16_t w, uint16_t h) {
  uint32_t bytes = w * h * 2;
  if ((buffer = (uint16_t *)malloc(bytes))) {
    memset(buffer, 0, bytes);
    _width = WIDTH = w;
    _height = HEIGHT = h;
  }
}

GFXcanvas16::~GFXcanvas16(void) {
  if (buffer)
    free(buffer);
}

void GFXcanvas16::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (buffer) {
    if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
      return;
    buffer[x + y * WIDTH] = color;
  }
}

uint16_t GFXcanvas16::getPixel(int16_t x, int16_t y) const {
  return getRawPixel(x, y);
}

uint16_t GFXcanvas16::getRawPixel(int16_t x, int16_t y) const {
  if ((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT))
    return 0;
  if (buffer) {
    return buffer[x + y * WIDTH];
  }
  return 0;
}

void GFXcanvas16::fillScreen(uint16_t color) {
  if (buffer) {
    uint8_t hi = color >> 8, lo = color & 0xFF;
    if (hi == lo) {
      memset(buffer, lo, WIDTH * HEIGHT * 2);
    } else {
      uint32_t i, pixels = WIDTH * HEIGHT;
      for (i = 0; i < pixels; i++)
        buffer[i] = color;
    }
  }
}

void GFXcanvas16::byteSwap(void) {
  if (buffer) {
    uint32_t i, pixels = WIDTH * HEIGHT;
    for (i = 0; i < pixels; i++)
      buffer[i] = __builtin_bswap16(buffer[i]);
  }
}

void GFXcanvas16::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if (h < 0) {
    h *= -1;
    y -= h - 1;
    if (y < 0) {
      h += y;
      y = 0;
    }
  }
  if ((x < 0) || (x >= width()) || (y >= height()) || ((y + h - 1) < 0)) {
    return;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (y + h > height()) {
    h = height() - y;
  }
  drawFastRawVLine(x, y, h, color);
}

void GFXcanvas16::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  if (w < 0) {
    w *= -1;
    x -= w - 1;
    if (x < 0) {
      w += x;
      x = 0;
    }
  }
  if ((y < 0) || (y >= height()) || (x >= width()) || ((x + w - 1) < 0)) {
    return;
  }
  if (x < 0) { 
    w += x;
    x = 0;
  }
  if (x + w >= width()) {
    w = width() - x;
  }
  drawFastRawHLine(x, y, w, color);
}

void GFXcanvas16::drawFastRawVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  uint16_t *buffer_ptr = buffer + y * WIDTH + x;
  for (int16_t i = 0; i < h; i++) {
    (*buffer_ptr) = color;
    buffer_ptr += WIDTH;
  }
}

void GFXcanvas16::drawFastRawHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  uint32_t buffer_index = y * WIDTH + x;
  for (uint32_t i = buffer_index; i < buffer_index + w; i++) {
    buffer[i] = color;
  }
}

void GFXcanvas16::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  for (int16_t i = x; i < x + w; i++) {
    drawFastVLine(i, y, h, color);
  }
}

#endif // DSP_MODEL==DSP_ILI9225
