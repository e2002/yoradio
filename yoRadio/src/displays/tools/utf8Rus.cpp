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
#ifdef DSP_LCD
  static const char* mapD0[] = {
    "A","B","V","G","D","E","ZH","Z","I","Y",
    "K","L","M","N","O","P","R","S","T","U",
    "F","H","TS","CH","SH","SHCH","'","YU","'","E","YU","YA"
  };
#endif
  for (int i = 0; str[i] && outPos < BUFLEN - 1; i++) {
    uint8_t c = (uint8_t)str[i];
    if (c == 0xD0 && str[i+1]) {
      uint8_t n = (uint8_t)str[++i];
      if (n == 0x81) {                  // Ё
      #ifdef DSP_LCD
        const char* t = "YO";
        for (; *t && outPos < BUFLEN-1; t++) out[outPos++] = *t;
      #else
        out[outPos++] = uppercase ? 0xA8 : 0xB8;
      #endif
      } else if (n >= 144 && n <= 191) {
      #ifdef DSP_LCD
        if(n>=176) n-=32;
        const char* t = mapD0[n - 0x90];
        for (; *t && outPos < BUFLEN-1; t++) out[outPos++] = *t;
      #else
        uint8_t ch = n + 48;
        if(n>=176 && uppercase) ch-=32;
        out[outPos++] = ch;
      #endif
      }
    } else if (c == 0xD1 && str[i+1]) {
      uint8_t n = (uint8_t)str[++i];
      if (n == 0x91) {                  // ё
      #ifdef DSP_LCD
        const char* t = "YO";
        for (; *t && outPos < BUFLEN-1; t++) out[outPos++] = *t;
      #else
        out[outPos++] = uppercase ? 0xA8 : 0xB8;
      #endif
      } else if (n >= 128 && n <= 143) {
      #ifdef DSP_LCD
        n+=16;
        const char* t = mapD0[n - 128];
        for (; *t && outPos < BUFLEN-1; t++) out[outPos++] = *t;
      #else
        uint8_t ch = n + 112;
        if(uppercase) ch-=32;
        out[outPos++] = ch;
      #endif
      }
    } else {                              // ASCII
    #ifdef DSP_LCD
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

