#include "../core/options.h"
#include "../core/player.h"
#if DSP_MODEL==DSP_ST7920

#include "Adafruit_GFX.h"
#include "ST7920.h"

//uint8_t buff[1024];    //This array serves as primitive "Video RAM" buffer

//This display is split into two halfs. Pages are 16bit long and pages are arranged in that way that are lied horizontaly instead of verticaly, unlike SSD1306 OLED, Nokia 5110 LCD, etc.
//After 8 horizonral page is written, it jumps to half of the screen (Y = 32) and continues until 16 lines of page have been written. After that, we have set cursor in new line.

#define TAKE_MUTEX() if(player.mutex_pl) xSemaphoreTake(player.mutex_pl, portMAX_DELAY)
#define GIVE_MUTEX() if(player.mutex_pl) xSemaphoreGive(player.mutex_pl)
#define st7920_swap(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

void ST7920::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if(x<0 || x>=ST7920_WIDTH || y<0 || y>=ST7920_HEIGHT || _dosleep) return;
  switch (getRotation()) {
    case 1:
        st7920_swap(x, y);
        x = WIDTH - x - 1;
        break;
    case 2:
        x = WIDTH  - x - 1;
        y = HEIGHT - y - 1;
        break;
    case 3:
        st7920_swap(x, y);
        y = HEIGHT - y - 1;
        break;
    default: break;
  }
  uint8_t y0 = 0, x0 = 0;                //Define and initilize varilables for skiping rows
  uint16_t data, n;                    //Define variable for sending data itno buffer (basicly, that is one line of page)
  if (y > 31) {                      //If Y coordinate is bigger than 31, that means we have to skip into that row, but we have to do that by adding 
    y -= 32;
    y0 = 16;
  }
  x0 = x % 16;
  x /= 16;
  data = 0x8000 >> x0;
  n = (x * 2) + (y0) + (32 * y);
  if(_invert) color = !color;
  if (!color) {
    buffer[n] &= (~data >> 8);
    buffer[n + 1] &= (~data & 0xFF);
  }else{
    buffer[n] |= (data >> 8);
    buffer[n + 1] |= (data & 0xFF);
  }
}

ST7920::ST7920(SPIClass *spi, int8_t cs_pin, uint32_t bitrate) : 
  Adafruit_GFX(ST7920_WIDTH, ST7920_HEIGHT), spi(spi), buffer(NULL),
  csPin(cs_pin) {
  spiSettings = SPISettings(bitrate, MSBFIRST, SPI_MODE3);
}

ST7920::~ST7920(void) {
  if(buffer) {
    free(buffer);
    buffer = NULL;
  }
}

void ST7920::begin(void) {
  buffer = (uint8_t *)malloc( 1024 );
  _invert = false;
  _dosleep = false;
  spi->begin();
  pinMode(csPin, OUTPUT);
  ST7920Command(0x30); //LCD_BASIC);
  ST7920Command(0x30); //LCD_BASIC);
  ST7920Command(0x01); //LCD_CLS);
  delay(2);
  ST7920Command(0x06); //LCD_ADDRINC);
    
  ST7920Command(0x0C); // ON
  ST7920Command(0x34); //LCD_EXTEND);
  ST7920Command(0x36); //LCD_GFXMODE);
}

void ST7920::clearDisplay() {
    long* p = (long*)&buffer;
    for (int i = 0; i < 256; i++) {
      p[i] = 0;
    }
}

void ST7920::display() {
  if(_dosleep) return;
  int x = 0, y = 0, n = 0;
//  ST7920Command(B00100100); //EXTENDED INSTRUCTION SET
//  ST7920Command(B00100110); //EXTENDED INSTRUCTION SET
  for (y = 0; y < 32; y++) {
    ST7920Command(0x80 | y);
    ST7920Command(0x80 | x);
    for (x = 0; x < 16; x++) {
        ST7920Data(buffer[n]);
        ST7920Data(buffer[n + 1]);
        n += 2;
    }
  }
}

void ST7920::doSleep(bool flag) {
  _dosleep = flag;
  ST7920Command(flag?ST7920_DISPLAYOFF:ST7920_DISPLAYON);
  delay(200);
  if(flag) ST7920Command(0x01);
}

void ST7920::invertDisplay(bool flag) {
  for(int i = 0; i<1024; i++) {
    buffer[i] = ~buffer[i];
  }
  _invert = flag;
}

void ST7920::ST7920Data(uint8_t data) { //RS = 1 RW = 0
  TAKE_MUTEX();
  spi->beginTransaction(spiSettings);
  digitalWrite(csPin, HIGH);
  spi->transfer(B11111010);
  spi->transfer((data & B11110000));
  spi->transfer((data & B00001111) << 4);
  digitalWrite(csPin, LOW);
  spi->endTransaction();
  GIVE_MUTEX();
  delayMicroseconds(38);
}

void ST7920::ST7920Command(uint8_t data) { //RS = 0 RW = 0
  TAKE_MUTEX();
  spi->beginTransaction(spiSettings);
  digitalWrite(csPin, HIGH);
  spi->transfer(B11111000);
  spi->transfer((data & B11110000));
  spi->transfer((data & B00001111) << 4);
  digitalWrite(csPin, LOW);
  spi->endTransaction();
  GIVE_MUTEX();
  delayMicroseconds(38);
}

#endif //#if DSP_MODEL==DSP_ST7920
