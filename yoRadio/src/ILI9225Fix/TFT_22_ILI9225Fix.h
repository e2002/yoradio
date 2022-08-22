#ifndef TFT_22_ILI9225FIX_h
#define TFT_22_ILI9225FIX_h

#ifdef __STM32F1__
#define ARDUINO_STM32_FEATHER
#define  PROGMEM
// if 'SPI_CHANNEL' is not defined, 'SPI' is used, only valid for STM32F1
//#define SPI_CHANNEL SPI_2
#endif

#define USE_STRING_CLASS

#ifdef USE_STRING_CLASS
    #define STRING String
#else
    #define STRING const char *
#endif

#if ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif
#include <SPI.h>

//#include "gfxfont.h"
typedef struct { // Data stored PER GLYPH
	uint16_t bitmapOffset;     // Pointer into GFXfont->bitmap
	uint8_t  width, height;    // Bitmap dimensions in pixels
	uint8_t  xAdvance;         // Distance to advance cursor (x axis)
	int8_t   xOffset, yOffset; // Dist from cursor pos to UL corner
} GFXglyph;

typedef struct { // Data stored for FONT AS A WHOLE:
	uint8_t  *bitmap;      // Glyph bitmaps, concatenated
	GFXglyph *glyph;       // Glyph array
	uint8_t   first, last; // ASCII extents
	uint8_t   yAdvance;    // Newline distance (y axis)
} GFXfont;

#if defined(ARDUINO_STM32_FEATHER) || defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || defined(STM32F1)
typedef volatile uint32_t RwReg;
#endif
#if defined(ARDUINO_FEATHER52)
typedef volatile uint32_t RwReg;
#endif

/* ILI9225 screen size */
#define ILI9225_LCD_WIDTH  176
#define ILI9225_LCD_HEIGHT 220

/* ILI9225 LCD Registers */
#define ILI9225_DRIVER_OUTPUT_CTRL      (0x01u)  // Driver Output Control
#define ILI9225_LCD_AC_DRIVING_CTRL     (0x02u)  // LCD AC Driving Control
#define ILI9225_ENTRY_MODE              (0x03u)  // Entry Mode
#define ILI9225_DISP_CTRL1              (0x07u)  // Display Control 1
#define ILI9225_BLANK_PERIOD_CTRL1      (0x08u)  // Blank Period Control
#define ILI9225_FRAME_CYCLE_CTRL        (0x0Bu)  // Frame Cycle Control
#define ILI9225_INTERFACE_CTRL          (0x0Cu)  // Interface Control
#define ILI9225_OSC_CTRL                (0x0Fu)  // Osc Control
#define ILI9225_POWER_CTRL1             (0x10u)  // Power Control 1
#define ILI9225_POWER_CTRL2             (0x11u)  // Power Control 2
#define ILI9225_POWER_CTRL3             (0x12u)  // Power Control 3
#define ILI9225_POWER_CTRL4             (0x13u)  // Power Control 4
#define ILI9225_POWER_CTRL5             (0x14u)  // Power Control 5
#define ILI9225_VCI_RECYCLING           (0x15u)  // VCI Recycling
#define ILI9225_RAM_ADDR_SET1           (0x20u)  // Horizontal GRAM Address Set
#define ILI9225_RAM_ADDR_SET2           (0x21u)  // Vertical GRAM Address Set
#define ILI9225_GRAM_DATA_REG           (0x22u)  // GRAM Data Register
#define ILI9225_GATE_SCAN_CTRL          (0x30u)  // Gate Scan Control Register
#define ILI9225_VERTICAL_SCROLL_CTRL1   (0x31u)  // Vertical Scroll Control 1 Register
#define ILI9225_VERTICAL_SCROLL_CTRL2   (0x32u)  // Vertical Scroll Control 2 Register
#define ILI9225_VERTICAL_SCROLL_CTRL3   (0x33u)  // Vertical Scroll Control 3 Register
#define ILI9225_PARTIAL_DRIVING_POS1    (0x34u)  // Partial Driving Position 1 Register
#define ILI9225_PARTIAL_DRIVING_POS2    (0x35u)  // Partial Driving Position 2 Register
#define ILI9225_HORIZONTAL_WINDOW_ADDR1 (0x36u)  // Horizontal Address Start Position
#define ILI9225_HORIZONTAL_WINDOW_ADDR2 (0x37u)  // Horizontal Address End Position
#define ILI9225_VERTICAL_WINDOW_ADDR1   (0x38u)  // Vertical Address Start Position
#define ILI9225_VERTICAL_WINDOW_ADDR2   (0x39u)  // Vertical Address End Position
#define ILI9225_GAMMA_CTRL1             (0x50u)  // Gamma Control 1
#define ILI9225_GAMMA_CTRL2             (0x51u)  // Gamma Control 2
#define ILI9225_GAMMA_CTRL3             (0x52u)  // Gamma Control 3
#define ILI9225_GAMMA_CTRL4             (0x53u)  // Gamma Control 4
#define ILI9225_GAMMA_CTRL5             (0x54u)  // Gamma Control 5
#define ILI9225_GAMMA_CTRL6             (0x55u)  // Gamma Control 6
#define ILI9225_GAMMA_CTRL7             (0x56u)  // Gamma Control 7
#define ILI9225_GAMMA_CTRL8             (0x57u)  // Gamma Control 8
#define ILI9225_GAMMA_CTRL9             (0x58u)  // Gamma Control 9
#define ILI9225_GAMMA_CTRL10            (0x59u)  // Gamma Control 10

#define ILI9225C_INVOFF  0x20
#define ILI9225C_INVON   0x21

// autoincrement modes (register ILI9225_ENTRY_MODE, bit 5..3 )
enum autoIncMode_t { R2L_BottomUp, BottomUp_R2L, L2R_BottomUp, BottomUp_L2R, R2L_TopDown, TopDown_R2L, L2R_TopDown, TopDown_L2R };

/* RGB 16-bit color table definition (RG565) */
#define COLOR_BLACK          0x0000      /*   0,   0,   0 */
#define COLOR_WHITE          0xFFFF      /* 255, 255, 255 */
#define COLOR_BLUE           0x001F      /*   0,   0, 255 */
#define COLOR_GREEN          0x07E0      /*   0, 255,   0 */
#define COLOR_RED            0xF800      /* 255,   0,   0 */
#define COLOR_NAVY           0x000F      /*   0,   0, 128 */
#define COLOR_DARKBLUE       0x0011      /*   0,   0, 139 */
#define COLOR_DARKGREEN      0x03E0      /*   0, 128,   0 */
#define COLOR_DARKCYAN       0x03EF      /*   0, 128, 128 */
#define COLOR_CYAN           0x07FF      /*   0, 255, 255 */
#define COLOR_TURQUOISE      0x471A      /*  64, 224, 208 */
#define COLOR_INDIGO         0x4810      /*  75,   0, 130 */
#define COLOR_DARKRED        0x8000      /* 128,   0,   0 */
#define COLOR_OLIVE          0x7BE0      /* 128, 128,   0 */
#define COLOR_GRAY           0x8410      /* 128, 128, 128 */
#define COLOR_GREY           0x8410      /* 128, 128, 128 */
#define COLOR_SKYBLUE        0x867D      /* 135, 206, 235 */
#define COLOR_BLUEVIOLET     0x895C      /* 138,  43, 226 */
#define COLOR_LIGHTGREEN     0x9772      /* 144, 238, 144 */
#define COLOR_DARKVIOLET     0x901A      /* 148,   0, 211 */
#define COLOR_YELLOWGREEN    0x9E66      /* 154, 205,  50 */
#define COLOR_BROWN          0xA145      /* 165,  42,  42 */
#define COLOR_DARKGRAY       0x7BEF      /* 128, 128, 128 */
#define COLOR_DARKGREY       0x7BEF      /* 128, 128, 128 */
#define COLOR_SIENNA         0xA285      /* 160,  82,  45 */
#define COLOR_LIGHTBLUE      0xAEDC      /* 172, 216, 230 */
#define COLOR_GREENYELLOW    0xAFE5      /* 173, 255,  47 */
#define COLOR_SILVER         0xC618      /* 192, 192, 192 */
#define COLOR_LIGHTGRAY      0xC618      /* 192, 192, 192 */
#define COLOR_LIGHTGREY      0xC618      /* 192, 192, 192 */
#define COLOR_LIGHTCYAN      0xE7FF      /* 224, 255, 255 */
#define COLOR_VIOLET         0xEC1D      /* 238, 130, 238 */
#define COLOR_AZUR           0xF7FF      /* 240, 255, 255 */
#define COLOR_BEIGE          0xF7BB      /* 245, 245, 220 */
#define COLOR_MAGENTA        0xF81F      /* 255,   0, 255 */
#define COLOR_TOMATO         0xFB08      /* 255,  99,  71 */
#define COLOR_GOLD           0xFEA0      /* 255, 215,   0 */
#define COLOR_ORANGE         0xFD20      /* 255, 165,   0 */
#define COLOR_SNOW           0xFFDF      /* 255, 250, 250 */
#define COLOR_YELLOW         0xFFE0      /* 255, 255,   0 */


/* Font defines */
#define FONT_HEADER_SIZE 4 // 1: pixel width of 1 font character, 2: pixel height, 
#define readFontByte(x) pgm_read_byte(&cfont.font[x])  

/*extern uint8_t Terminal6x8[];
extern uint8_t Terminal11x16[];
extern uint8_t Terminal12x16[];
extern uint8_t Trebuchet_MS16x21[];*/

struct _currentFont
{
    uint8_t* font;
    uint8_t width;
    uint8_t height;
    uint8_t offset;
    uint8_t numchars;
    uint8_t nbrows;
    bool    monoSp;
};
#define MONOSPACE   1

#if defined (ARDUINO_STM32_FEATHER) || defined(ESP32)
    #undef USE_FAST_PINIO
#elif defined (__AVR__) || defined(TEENSYDUINO) || defined(ESP8266) || defined(__arm__)
    #define USE_FAST_PINIO
#endif

/// Main and core class
class TFT_22_ILI9225 {

    public:

        TFT_22_ILI9225(int8_t RST, int8_t RS, int8_t CS, int8_t SDI, int8_t CLK, int8_t LED);
        TFT_22_ILI9225(int8_t RST, int8_t RS, int8_t CS, int8_t LED);
        TFT_22_ILI9225(int8_t RST, int8_t RS, int8_t CS, int8_t SDI, int8_t CLK, int8_t LED, uint8_t brightness);
        TFT_22_ILI9225(int8_t RST, int8_t RS, int8_t CS, int8_t LED, uint8_t brightness);

        /// Initialization
#ifndef ESP32
        void begin(void);
#else
        void begin(SPIClass &spi=SPI);
#endif

        /// Clear the screen
        void clear(uint16_t withColor = COLOR_BLACK); 

        /// Invert screen
        /// @param     flag true to invert, false for normal screen
        void invertDisplay(boolean flag);

        /// Switch backlight on or off
        /// @param     flag true=on, false=off
        void setBacklight(boolean flag); 

        /// Set backlight brightness
        /// @param     brightness sets backlight brightness 0-255
        void setBacklightBrightness(uint8_t brightness); 

        /// Switch display on or off
        /// @param     flag true=on, false=off
        void setDisplay(boolean flag);  

        /// Set orientation
        /// @param     orientation orientation, 0=portrait, 1=right rotated landscape, 2=reverse portrait, 3=left rotated landscape
        void setOrientation(uint8_t orientation);  

        /// Get orientation
        /// @return    orientation orientation, 0=portrait, 1=right rotated landscape, 2=reverse portrait, 3=left rotated landscape
        uint8_t getOrientation(void); 

        /// Font size, x-axis
        /// @return    horizontal size of current font, in pixels
        // uint8_t fontX(void);

        /// Font size, y-axis
        /// @return    vertical size of current font, in pixels
        // uint8_t fontY(void); 

        /// Screen size, x-axis
        /// @return   horizontal size of the screen, in pixels
        /// @note     240 means 240 pixels and thus 0..239 coordinates (decimal)
        uint16_t maxX(void);

        /// Screen size, y-axis
        /// @return   vertical size of the screen, in pixels
        /// @note     220 means 220 pixels and thus 0..219 coordinates (decimal)
        uint16_t maxY(void);

        /// Draw circle
        /// @param    x0 center, point coordinate, x-axis
        /// @param    y0 center, point coordinate, y-axis
        /// @param    radius radius
        /// @param    color 16-bit color
        void drawCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color);  

        /// Draw solid circle
        /// @param    x0 center, point coordinate, x-axis
        /// @param    y0 center, point coordinate, y-axis
        /// @param    radius radius
        /// @param    color 16-bit color
        void fillCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint16_t color); 

        /// Set background color
        /// @param    color background color, default=black
        void setBackgroundColor(uint16_t color = COLOR_BLACK);  

        /// Draw line, rectangle coordinates
        /// @param    x1 start point coordinate, x-axis
        /// @param    y1 start point coordinate, y-axis
        /// @param    x2 end point coordinate, x-axis
        /// @param    y2 end point coordinate, y-axis
        /// @param    color 16-bit color
        void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color); 

        /// Draw rectangle, rectangle coordinates
        /// @param    x1 top left coordinate, x-axis
        /// @param    y1 top left coordinate, y-axis
        /// @param    x2 bottom right coordinate, x-axis
        /// @param    y2 bottom right coordinate, y-axis
        /// @param    color 16-bit color
        void drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color); 

        /// Draw solid rectangle, rectangle coordinates
        /// @param    x1 top left coordinate, x-axis
        /// @param    y1 top left coordinate, y-axis
        /// @param    x2 bottom right coordinate, x-axis
        /// @param    y2 bottom right coordinate, y-axis
        /// @param    color 16-bit color
        void fillRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

        /// Draw pixel
        /// @param    x1 point coordinate, x-axis
        /// @param    y1 point coordinate, y-axis
        /// @param    color 16-bit color
        void drawPixel(uint16_t x1, uint16_t y1, uint16_t color);  

        /// Draw ASCII Text (pixel coordinates)
        /// @param    x point coordinate, x-axis
        /// @param    y point coordinate, y-axis
        /// @param    s text string
        /// @param    color 16-bit color, default=white
        /// @return   x-position behind text
        uint16_t  drawText(uint16_t x, uint16_t y, STRING s, uint16_t color = COLOR_WHITE);
        
        /// width of an ASCII Text (pixel )
        /// @param    s text string
        uint16_t getTextWidth( STRING s ) ;
        
        /// Calculate 16-bit color from 8-bit Red-Green-Blue components
        /// @param    red red component, 0x00..0xff
        /// @param    green green component, 0x00..0xff
        /// @param    blue blue component, 0x00..0xff
        /// @return   16-bit color
        uint16_t setColor(uint8_t red, uint8_t green, uint8_t blue);

        /// Calculate 8-bit Red-Green-Blue components from 16-bit color
        /// @param    rgb 16-bit color
        /// @param    red red component, 0x00..0xff
        /// @param    green green component, 0x00..0xff
        /// @param    blue blue component, 0x00..0xff
        void splitColor(uint16_t rgb, uint8_t &red, uint8_t &green, uint8_t &blue);

        /// Draw triangle, triangle coordinates
        /// @param    x1 corner 1 coordinate, x-axis
        /// @param    y1 corner 1 coordinate, y-axis
        /// @param    x2 corner 2 coordinate, x-axis
        /// @param    y2 corner 2 coordinate, y-axis
        /// @param    x3 corner 3 coordinate, x-axis
        /// @param    y3 corner 3 coordinate, y-axis
        /// @param    color 16-bit color
        void drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color); 

        /// Draw solid triangle, triangle coordinates
        /// @param    x1 corner 1 coordinate, x-axis
        /// @param    y1 corner 1 coordinate, y-axis
        /// @param    x2 corner 2 coordinate, x-axis
        /// @param    y2 corner 2 coordinate, y-axis
        /// @param    x3 corner 3 coordinate, x-axis
        /// @param    y3 corner 3 coordinate, y-axis
        /// @param    color 16-bit color
        void fillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);

        /// Set current font
        /// @param    font Font name
        void setFont(uint8_t* font, bool monoSp=false ); // default = proportional
        
        /// Get current font
        _currentFont getFont();
        
        /// Draw single character (pixel coordinates)
        /// @param    x point coordinate, x-axis
        /// @param    y point coordinate, y-axis
        /// @param    ch ASCII character
        /// @param    color 16-bit color, default=white
        /// @return   width of character in display pixels
        uint16_t drawChar(uint16_t x, uint16_t y, uint16_t ch, uint16_t color = COLOR_WHITE);

        /// width of an ASCII character (pixel )
        /// @param    ch ASCII character
        uint16_t getCharWidth( uint16_t ch ) ;
        
        /// Draw bitmap
        /// @param    x point coordinate, x-axis
        /// @param    y point coordinate, y-axis
        /// @param    bitmap 
        /// @param    w width
        /// @param    h height
        /// @param    color 16-bit color, default=white
        /// @param    bg 16-bit color, background
        void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
        void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
        void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
        void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);

        void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
        void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
        
        /// Draw bitmap
        /// @param    x point coordinate, x-axis
        /// @param    y point coordinate, y-axis
        /// @param    bitmap, 2D 16bit color bitmap 
        /// @param    w width
        /// @param    h height
        void drawBitmap(uint16_t x, uint16_t y, const uint16_t** bitmap, int16_t w, int16_t h);
        void drawBitmap(uint16_t x, uint16_t y, uint16_t** bitmap, int16_t w, int16_t h);

        /// Draw bitmap
        /// @param    x point coordinate, x-axis
        /// @param    y point coordinate, y-axis
        /// @param    bitmap, 1D 16bit color bitmap
        /// @param    w width
        /// @param    h height
        void drawBitmap(uint16_t x, uint16_t y, const uint16_t* bitmap, int16_t w, int16_t h);
        void drawBitmap(uint16_t x, uint16_t y, uint16_t* bitmap, int16_t w, int16_t h);

        /// Set current GFX font
        /// @param    f GFX font name defined in include file
        void setGFXFont(const GFXfont *f = NULL);

        /// Draw a string with the current GFX font
        /// @param    x point coordinate, x-axis
        /// @param    y point coordinate, y-axis
        /// @param    s string to print
        /// @param    color 16-bit color
        void drawGFXText(int16_t x, int16_t y, STRING s, uint16_t color);
        
        /// Get the width & height of a text string with the current GFX font
        /// @param    str string to analyze
        /// @param    x point coordinate, x-axis
        /// @param    y point coordinate, y-axis
        /// @param    w width in pixels of string 
        /// @param    h height in pixels of string
        void getGFXTextExtent(STRING str, int16_t x, int16_t y, int16_t *w, int16_t *h);
        
        /// Draw a single character with the current GFX font
        /// @param    x point coordinate, x-axis
        /// @param    y point coordinate, y-axis
        /// @param    c character to draw
        /// @param    color 16-bit color
        /// @return   width of character in display pixels
        uint16_t drawGFXChar(int16_t x, int16_t y, unsigned char c, uint16_t color);
        
        void startWrite(void);
        void endWrite(void);

    private:

        void _spiWrite(uint8_t v);
        void _spiWrite16(uint16_t v);
        void _spiWriteCommand(uint8_t c);
        void _spiWriteData(uint8_t d);

        void _swap(uint16_t &a, uint16_t &b);
        void _setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
        void _setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, autoIncMode_t mode);
        void _resetWindow();
        void _drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, 
                            uint16_t color, uint16_t bg, bool transparent, bool progmem, bool Xbit );
        void _orientCoordinates(uint16_t &x1, uint16_t &y1);
        void _writeRegister(uint16_t reg, uint16_t data);
        void _writeData(uint8_t HI, uint8_t LO);
        void _writeData16(uint16_t HILO);
        void _writeCommand(uint8_t HI, uint8_t LO);
        void _writeCommand16(uint16_t HILO);
        uint16_t _maxX, _maxY, _bgColor;

#if defined (__AVR__) || defined(TEENSYDUINO)
        int8_t  _rst, _rs, _cs, _sdi, _clk, _led;
    #ifdef USE_FAST_PINIO
        volatile uint8_t *mosiport, *clkport, *dcport, *rsport, *csport;
        uint8_t  mosipinmask, clkpinmask, cspinmask, dcpinmask;
    #endif
#elif defined (__arm__)
        int32_t  _rst, _rs, _cs, _sdi, _clk, _led;
    #ifdef USE_FAST_PINIO
        volatile RwReg *mosiport, *clkport, *dcport, *rsport, *csport;
        uint32_t  mosipinmask, clkpinmask, cspinmask, dcpinmask;
    #endif
#elif defined (ESP8266) || defined (ESP32)
        int8_t  _rst, _rs, _cs, _sdi, _clk, _led;
    #ifdef USE_FAST_PINIO
        volatile uint32_t *mosiport, *clkport, *dcport, *rsport, *csport;
        uint32_t  mosipinmask, clkpinmask, cspinmask, dcpinmask;
    #endif
#else
        int8_t  _rst, _rs, _cs, _sdi, _clk, _led;
#endif
        uint8_t  _orientation, _brightness;
        
        // Corresponding modes if orientation changed:
        const autoIncMode_t modeTab [3][8] = {
        //          { R2L_BottomUp, BottomUp_R2L, L2R_BottomUp, BottomUp_L2R, R2L_TopDown,  TopDown_R2L,  L2R_TopDown,  TopDown_L2R }//
        /* 90° */   { BottomUp_L2R, L2R_BottomUp, TopDown_L2R,  L2R_TopDown,  BottomUp_R2L, R2L_BottomUp, TopDown_R2L,  R2L_TopDown },   
        /*180° */   { L2R_TopDown , TopDown_L2R,  R2L_TopDown,  TopDown_R2L,  L2R_BottomUp, BottomUp_L2R, R2L_BottomUp, BottomUp_R2L}, 
        /*270° */   { TopDown_R2L , R2L_TopDown,  BottomUp_R2L, R2L_BottomUp, TopDown_L2R,  L2R_TopDown,  BottomUp_L2R, L2R_BottomUp}
        };
 

        bool  hwSPI, blState;

        _currentFont cfont;

#ifdef ESP32
        SPIClass _spi;
#endif

    protected:

        uint32_t writeFunctionLevel;

        void getGFXCharExtent(uint8_t c, int16_t *gw, int16_t *gh, int16_t *xa);
        
        GFXfont *gfxFont;
};

class GFXcanvas16 {
public:
  GFXcanvas16(uint16_t w, uint16_t h);
  ~GFXcanvas16(void);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void fillScreen(uint16_t color);
  void byteSwap(void);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  uint16_t getPixel(int16_t x, int16_t y) const;
  uint16_t *getBuffer(void) const { return buffer; }
  int16_t width(void) const { return _width; };
  int16_t height(void) const { return _height; }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
protected:
  uint16_t getRawPixel(int16_t x, int16_t y) const;
  void drawFastRawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastRawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  int16_t WIDTH;
  int16_t HEIGHT;
  int16_t _width;
  int16_t _height;

private:
  uint16_t *buffer;
};
#endif
