#include <SPI.h>
#include <Adafruit_GFX.h>

#define ST7920_HEIGHT 	64		//64 pixels tall display
#define ST7920_WIDTH	128		//128 pixels wide display


#define ST7920_DISPLAYOFF 0x08
#define ST7920_DISPLAYON 0x0C
//#define ST7920_NORMALDISPLAY 0xA6
//#define ST7920_INVERSEDISPLAY 0xA7

#define BLACK 0					//Defines color - Black color -> Bit in buffer is set to one
#define WHITE 1					//Defines color - White color -> Bit in buffer is set to zero

class ST7920 : public Adafruit_GFX {
  public:
   	//ST7920(int8_t CS);
    ST7920(SPIClass *spi, int8_t cs_pin, uint32_t bitrate=8000000UL);
    ~ST7920(void);
  	void          begin(void);
  	void          clearDisplay(void);
  	void          invertDisplay(bool flag);
  	void          doSleep(bool flag);
  	void          display();
  	void          drawPixel(int16_t x, int16_t y, uint16_t color);
    void          ST7920Data(uint8_t data);
  	void          ST7920Command(uint8_t data);
  private:
    SPIClass      *spi;
    uint8_t       *buffer;
    int8_t        csPin;
    SPISettings   spiSettings;
    bool          _invert, _dosleep;
};


