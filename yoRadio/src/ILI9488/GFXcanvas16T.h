#ifndef _GFXcanvas16T_H_
#define _GFXcanvas16T_H_

#include <Adafruit_GFX.h>

template<const uint16_t w, uint16_t h>
class GFXcanvas16T : public Adafruit_GFX
{
  public:
    GFXcanvas16T() : Adafruit_GFX(w, h) {};
    void drawPixel(int16_t x, int16_t y, uint16_t color)
    {
      if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return;
      int16_t t;
      switch (rotation) {
        case 1:
          t = x;
          x = WIDTH  - 1 - y;
          y = t;
          break;
        case 2:
          x = WIDTH  - 1 - x;
          y = HEIGHT - 1 - y;
          break;
        case 3:
          t = x;
          x = y;
          y = HEIGHT - 1 - t;
          break;
      }
      buffer[x + y * WIDTH] = color;
    }
    void fillScreen(uint16_t color)
    {
      uint32_t i, pixels = WIDTH * HEIGHT;
      for (i = 0; i < pixels; i++) buffer[i] = color;
    }
    uint16_t *getBuffer(void)
    {
      return buffer;
    }
#if 1
    template<class T>
    void print(T target, const char* name = 0, bool as_progmem = false)
    {
      static const uint16_t per_line = 10;
      uint32_t remain = WIDTH * HEIGHT;
      uint16_t* p = buffer;
      if (!name) name = "canvas16";
      if (as_progmem) target.print(F("const "));
      target.print(F("uint16_t ")); target.print(name); target.print(F("[] "));
      if (as_progmem) target.print(F("PROGMEM "));
      target.println(F("=")); target.println(F("{"));
      while (remain > per_line)
      {
        target.print(F("  "));
        for (uint16_t i = 0; i < per_line; i++)
        {
          target.print(F("0x")); target.print(*p++, HEX); target.print(F(", "));
        }
        remain -= per_line;
        if (remain > 0) target.println();
#if defined (ESP8266)
        yield();
#endif
      }
      if (remain > 0) target.print(F("  "));
      while (remain-- > 0)
      {
        target.print(F("0x")); target.print(*p++, HEX); target.print(F(", "));
      }
      target.println();
      target.println(F("};"));
    }
#endif
  private:
    uint16_t buffer[w * h];
};

#endif
