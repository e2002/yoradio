#ifndef printFix_h
#define printFix_h

#include "Arduino.h"

char* printFix(const char* src) {
  
    #ifndef PRINT_FIX
      return(src);
    #endif

    static char buf[BUFLEN];

    int outIdx = 0;
    const char* p = src;
    while (*p && outIdx < BUFLEN - 1) {
      if ((uint8_t)*p < 0x80) {
        buf[outIdx++] = *p++;
        continue;
      }
      
      uint8_t first = (uint8_t)*p;
      uint8_t second = (uint8_t)*(p + 1);

      // Handle 3-byte UTF-8 sequences
      if (first >= 0xE0 && first <= 0xEF) {
        uint8_t third = (uint8_t)*(p + 2);
        bool processed = false;
        // Ellipsis
        if (first == 0xE2 && second == 0x80 && third == 0xA6) {
          buf[outIdx++] = '.';
          buf[outIdx++] = '.';
          buf[outIdx++] = '.';
          processed = true;
        }
        // Trademark
        if (first == 0xE2 && second == 0x84 && third == 0xA2) {
          buf[outIdx++] = 'T';
          buf[outIdx++] = 'M';
          processed = true;
        }
        
        if(processed) {
          p += 3;
        } else {
          buf[outIdx++] = ' '; // Replace with space if unknown
          p += 3;
        }
        continue;
      }

      // Handle 2-byte UTF-8 sequences
      if (first >= 0xC2 && first <= 0xDF) {
        bool processed = false;
        if (first == 0xC3) {
          processed = true;
          switch (second) {
            case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: buf[outIdx++] = 'A'; break; // ÀÁÂÃÄÅ
            case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: buf[outIdx++] = 'a'; break; // àáâãäå
            case 0x86: buf[outIdx++] = 'A'; buf[outIdx++] = 'E'; break; // Æ
            case 0xA6: buf[outIdx++] = 'a'; buf[outIdx++] = 'e'; break; // æ
            case 0x88: case 0x89: case 0x8A: case 0x8B: buf[outIdx++] = 'E'; break; // ÈÉÊË
            case 0xA8: case 0xA9: case 0xAA: case 0xAB: buf[outIdx++] = 'e'; break; // èéêë
            case 0x8C: case 0x8D: case 0x8E: case 0x8F: buf[outIdx++] = 'I'; break; // ÌÍÎÏ
            case 0xAC: case 0xAD: case 0xAE: case 0xAF: buf[outIdx++] = 'i'; break; // ìíîï
            case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x98: buf[outIdx++] = 'O'; break; // ÒÓÔÕÖØ
            case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB8: buf[outIdx++] = 'o'; break; // òóôõöø
            case 0x99: case 0x9A: case 0x9B: case 0x9C: buf[outIdx++] = 'U'; break; // ÙÚÛÜ
            case 0xB9: case 0xBA: case 0xBB: case 0xBC: buf[outIdx++] = 'u'; break; // ùúûü
            case 0x87: buf[outIdx++] = 'C'; break; // Ç
            case 0xA7: buf[outIdx++] = 'c'; break; // ç
            case 0x91: buf[outIdx++] = 'N'; break; // Ñ
            case 0xB1: buf[outIdx++] = 'n'; break; // ñ
            case 0x9D: buf[outIdx++] = 'Y'; break; // Ý
            case 0xBD: case 0xBF: buf[outIdx++] = 'y'; break; // ýÿ
            case 0x9F: buf[outIdx++] = 's'; buf[outIdx++] = 's'; break; // ß
            default: processed = false; break;
          }
        }
        if (!processed && (first == 0xD0 || first == 0xD1)) {
          processed = true;
          uint16_t code = ((first & 0x1F) << 6) | (second & 0x3F);
          switch (code) {
            case 0x410: case 0x430: buf[outIdx++] = 'A'; break; // Аа
            case 0x411: case 0x431: buf[outIdx++] = 'B'; break; // Бб
            case 0x412: case 0x432: buf[outIdx++] = 'V'; break; // Вв
            case 0x413: case 0x433: buf[outIdx++] = 'G'; break; // Гг
            case 0x414: case 0x434: buf[outIdx++] = 'D'; break; // Дд
            case 0x415: case 0x435: buf[outIdx++] = 'E'; break; // Ее
            case 0x401: case 0x451: buf[outIdx++] = 'E'; break; // Ёё
            case 0x416: case 0x436: buf[outIdx++] = 'Z'; break; // Жж
            case 0x417: case 0x437: buf[outIdx++] = 'Z'; break; // Зз
            case 0x418: case 0x438: buf[outIdx++] = 'I'; break; // Ии
            case 0x419: case 0x439: buf[outIdx++] = 'Y'; break; // Йй
            case 0x41A: case 0x43A: buf[outIdx++] = 'K'; break; // Кк
            case 0x41B: case 0x43B: buf[outIdx++] = 'L'; break; // Лл
            case 0x41C: case 0x43C: buf[outIdx++] = 'M'; break; // Мм
            case 0x41D: case 0x43D: buf[outIdx++] = 'N'; break; // Нн
            case 0x41E: case 0x43E: buf[outIdx++] = 'O'; break; // Оо
            case 0x41F: case 0x43F: buf[outIdx++] = 'P'; break; // Пп
            case 0x420: case 0x440: buf[outIdx++] = 'R'; break; // Рр
            case 0x421: case 0x441: buf[outIdx++] = 'S'; break; // Сс
            case 0x422: case 0x442: buf[outIdx++] = 'T'; break; // Тт
            case 0x423: case 0x443: buf[outIdx++] = 'U'; break; // Уу
            case 0x424: case 0x444: buf[outIdx++] = 'F'; break; // Фф
            case 0x425: case 0x445: buf[outIdx++] = 'H'; break; // Хх
            case 0x426: case 0x446: buf[outIdx++] = 'C'; break; // Цц
            case 0x427: case 0x447: buf[outIdx++] = 'C'; break; // Чч
            case 0x428: case 0x448: buf[outIdx++] = 'S'; break; // Шш
            case 0x429: case 0x449: buf[outIdx++] = 'S'; break; // Щщ
            case 0x42A: case 0x44A: buf[outIdx++] = '\''; break; // Ъъ
            case 0x42B: case 0x44B: buf[outIdx++] = 'Y'; break; // Ыы
            case 0x42C: case 0x44C: buf[outIdx++] = '\''; break; // Ьь
            case 0x42D: case 0x44D: buf[outIdx++] = 'E'; break; // Ээ
            case 0x42E: case 0x44E: buf[outIdx++] = 'U'; break; // Юю
            case 0x42F: case 0x44F: buf[outIdx++] = 'A'; break; // Яя
            default: processed = false; break;
          }
        }
        if (!processed && first == 0xC5) {
            processed = true;
            switch(second) {
                case 0x92: buf[outIdx++] = 'O'; buf[outIdx++] = 'E'; break; // Œ
                case 0x93: buf[outIdx++] = 'o'; buf[outIdx++] = 'e'; break; // œ
                default: processed = false; break;
            }
        }
        if (!processed && first == 0xC2) {
          processed = true;
          switch (second) {
            case 0xAB: buf[outIdx++] = '"'; break; // «
            case 0xBB: buf[outIdx++] = '"'; break; // »
            case 0xA9: buf[outIdx++] = '('; buf[outIdx++] = 'c'; buf[outIdx++] = ')'; break; // ©
            case 0xAE: buf[outIdx++] = '('; buf[outIdx++] = 'R'; buf[outIdx++] = ')'; break; // ®
            case 0x9D: buf[outIdx++] = '['; break; // [ multiplication sign
            case 0x9E: buf[outIdx++] = ' '; break; // non-breaking space
            case 0x9F: buf[outIdx++] = ']'; break; // ] division sign
            case 0xA0: buf[outIdx++] = '|'; break; // ¦ broken bar
            case 0xA1: buf[outIdx++] = '!'; break; // ¡ inverted exclamation mark
            case 0xA2: buf[outIdx++] = 'c'; break; // ¢ cent sign
            case 0xA3: buf[outIdx++] = '#'; break; // £ pound sign
            case 0x96: buf[outIdx++] = '-'; break; // – en dash
            case 0x97: buf[outIdx++] = '-'; break; // — em dash
            case 0x91: buf[outIdx++] = '\''; break; // ‘ left single quotation mark
            case 0x92: buf[outIdx++] = '\''; break; // ’ right single quotation mark
            case 0x93: buf[outIdx++] = '"'; break; // “ left double quotation mark
            case 0x94: buf[outIdx++] = '"'; break; // ” right double quotation mark
            case 0xBF: buf[outIdx++] = '?'; break; // ¿ inverted question mark
            default: processed = false; break;
          }
        }
        
        if(processed) {
          p += 2;
        } else {
          buf[outIdx++] = ' ';
          p += 2;
        }
        continue;
      }
      
      // Fallback for other multi-byte characters or invalid UTF-8
      int bytesToSkip = 0;
      if (first >= 0xC0 && first <= 0xDF) bytesToSkip = 2;      // 2-byte sequence
      else if (first >= 0xE0 && first <= 0xEF) bytesToSkip = 3; // 3-byte sequence
      else if (first >= 0xF0 && first <= 0xF7) bytesToSkip = 4; // 4-byte sequence
      else bytesToSkip = 1;                                     // Invalid/single byte

      buf[outIdx++] = ' '; // Replace with a space
      p += bytesToSkip;
    }
    buf[outIdx] = 0;
    return buf;
}

#endif