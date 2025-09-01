#include "Arduino.h"
#include "../../core/options.h"
#include "../dspcore.h"
#include "utf8Rus.h"

size_t strlen_utf8(const char* s) {
  size_t count = 0;
  while (*s) {
    count++;
    if ((*s & 0xF0) == 0xF0) { // 4-byte character
      s += 4;
    } else if ((*s & 0xE0) == 0xE0) { // 3-byte character
      s += 3;
    } else if ((*s & 0xC0) == 0xC0) { // 2-byte character
      s += 2;
    } else { // 1-byte character (ASCII)
      s += 1;
    }
  }
  return count;
}

char* utf8Rus(const char* str, bool uppercase) {
  static char out[BUFLEN];
  int outPos = 0;
#if defined(DSP_LCD) && !defined(LCD_RUS)
  static const char* mapD0[] = {
    "A","B","V","G","D","E","ZH","Z","I","Y",
    "K","L","M","N","O","P","R","S","T","U",
    "F","H","TS","CH","SH","SHCH","'","YU","'","E","YU","YA"
  };
#endif
#if defined(DSP_LCD) && defined(LCD_RUS)
  // except 0401 --> 0xa2 = Ё, 0451 --> 0xb5 = ё
  static const unsigned char utf_recode[] PROGMEM =
  {
    0x41,0xa0,0x42,0xa1,0xe0,0x45,0xa3,0xa4,0xa5,0xa6,0x4b,0xa7,0x4d,0x48,0x4f,
    0xa8,0x50,0x43,0x54,0xa9,0xaa,0x58,0xe1,0xab,0xac,0xe2,0xad,0xae,0x62,0xaf,0xb0,0xb1,
    0x61,0xb2,0xb3,0xb4,0xe3,0x65,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0x6f,
    0xbe,0x70,0x63,0xbf,0x79,0xe4,0x78,0xe5,0xc0,0xc1,0xe6,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7
  };
#endif
  for (int i = 0; str[i] && outPos < BUFLEN - 1; i++) {
    uint8_t c = (uint8_t)str[i];
    if (c == 0xD0 && str[i+1]) {
      uint8_t n = (uint8_t)str[++i];
      if (n == 0x81) {                  // Ё
      #if defined(DSP_LCD) && !defined(LCD_RUS)
        const char* t = "YO";
        for (; *t && outPos < BUFLEN-1; t++) out[outPos++] = *t;
      #else
        out[outPos++] = uppercase ? 0xA8 : 0xB8;
      #endif
      } else if (n >= 144 && n <= 191) {
      #if defined(DSP_LCD) && !defined(LCD_RUS)
        if(n>=176) n-=32;
        const char* t = mapD0[n - 0x90];
        for (; *t && outPos < BUFLEN-1; t++) out[outPos++] = *t;
      #else
        #if defined(DSP_LCD) && defined(LCD_RUS)
          if(n>=176) n-=32;
          out[outPos++] = utf_recode[n - 0x90];
        #else
          uint8_t ch = n + 48;
          if(n>=176 && uppercase) ch-=32;
          out[outPos++] = ch;
        #endif
      #endif
      }
    } else if (c == 0xD1 && str[i+1]) {
      uint8_t n = (uint8_t)str[++i];
      if (n == 0x91) {                  // ё
      #if defined(DSP_LCD) && !defined(LCD_RUS)
        const char* t = "YO";
        for (; *t && outPos < BUFLEN-1; t++) out[outPos++] = *t;
      #else
        out[outPos++] = uppercase ? 0xA8 : 0xB8;
      #endif
      } else if (n >= 128 && n <= 143) {
      #if defined(DSP_LCD) && !defined(LCD_RUS)
        n+=16;
        const char* t = mapD0[n - 128];
        for (; *t && outPos < BUFLEN-1; t++) out[outPos++] = *t;
      #else
        #if defined(DSP_LCD) && defined(LCD_RUS)
          n+=16;
          out[outPos++] = utf_recode[n - 128];
        #else
          uint8_t ch = n + 112;
          if(uppercase) ch-=32;
          out[outPos++] = ch;
        #endif
      #endif
      }
    } else {                              // ASCII
    #if defined(DSP_LCD) && !defined(LCD_RUS)
      char ch = (char)toupper(c);
      if (ch == 7) ch = (char)165;
      if (ch == 9) ch = (char)223;
      out[outPos++] = ch;
    #else
      out[outPos++] = uppercase ? toupper(c) : c;
    #endif
    }
  }
  out[outPos] = 0;
  return out;
}

