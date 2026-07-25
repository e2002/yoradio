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
    }
#if !defined(DSP_LCD)
    // Czech (and Slovak-compatible) diacritics -> font glyphs at 0x80-0x9D
    // (see yoRadio/fonts/glcdfont.c). Glyphs live in glcdfont.c, which only
    // applies to GFX displays, hence the !DSP_LCD guard.
    else if (c == 0xC3 && str[i+1]) {
      uint8_t n = (uint8_t)str[++i];
      switch (n) {
        case 0x81: out[outPos++] = 0x8F; break;                          // Á
        case 0x89: out[outPos++] = 0x92; break;                          // É
        case 0x8D: out[outPos++] = 0x94; break;                          // Í
        case 0x93: out[outPos++] = 0x96; break;                          // Ó
        case 0x9A: out[outPos++] = 0x9A; break;                          // Ú
        case 0x9D: out[outPos++] = 0x9C; break;                          // Ý
        case 0xA1: out[outPos++] = uppercase ? 0x8F : 0x80; break;       // á
        case 0xA9: out[outPos++] = uppercase ? 0x92 : 0x83; break;       // é
        case 0xAD: out[outPos++] = uppercase ? 0x94 : 0x85; break;       // í
        case 0xB3: out[outPos++] = uppercase ? 0x96 : 0x87; break;       // ó
        case 0xBA: out[outPos++] = uppercase ? 0x9A : 0x8B; break;       // ú
        case 0xBD: out[outPos++] = uppercase ? 0x9C : 0x8D; break;       // ý
        default: break;                                                  // non-Czech: skip
      }
    } else if (c == 0xC4 && str[i+1]) {
      uint8_t n = (uint8_t)str[++i];
      switch (n) {
        case 0x8C: out[outPos++] = 0x90; break;                          // Č
        case 0x8E: out[outPos++] = 0x91; break;                          // Ď
        case 0x9A: out[outPos++] = 0x93; break;                          // Ě
        case 0x8D: out[outPos++] = uppercase ? 0x90 : 0x81; break;       // č
        case 0x8F: out[outPos++] = uppercase ? 0x91 : 0x82; break;       // ď
        case 0x9B: out[outPos++] = uppercase ? 0x93 : 0x84; break;       // ě
        default: break;
      }
    } else if (c == 0xC5 && str[i+1]) {
      uint8_t n = (uint8_t)str[++i];
      switch (n) {
        case 0x87: out[outPos++] = 0x95; break;                          // Ň
        case 0x98: out[outPos++] = 0x97; break;                          // Ř
        case 0xA0: out[outPos++] = 0x98; break;                          // Š
        case 0xA4: out[outPos++] = 0x99; break;                          // Ť
        case 0xAE: out[outPos++] = 0x9B; break;                          // Ů
        case 0xBD: out[outPos++] = 0x9D; break;                          // Ž
        case 0x88: out[outPos++] = uppercase ? 0x95 : 0x86; break;       // ň
        case 0x99: out[outPos++] = uppercase ? 0x97 : 0x88; break;       // ř
        case 0xA1: out[outPos++] = uppercase ? 0x98 : 0x89; break;       // š
        case 0xA5: out[outPos++] = uppercase ? 0x99 : 0x8A; break;       // ť
        case 0xAF: out[outPos++] = uppercase ? 0x9B : 0x8C; break;       // ů
        case 0xBE: out[outPos++] = uppercase ? 0x9D : 0x8E; break;       // ž
        default: break;
      }
    }
#endif
    else {                              // ASCII
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

