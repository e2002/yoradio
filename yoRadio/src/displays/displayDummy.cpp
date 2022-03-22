#include "../../options.h"
#if DSP_MODEL==0

#include "displayDummy.h"
#include <SPI.h>
#include "../../player.h"
#include "../../config.h"
#include "../../network.h"

DspCore::DspCore() {

}

char* DspCore::utf8Rus(const char* str, bool uppercase) {
  int index = 0;
  static char strn[BUFLEN];
  bool E = false;
  strlcpy(strn, str, BUFLEN);
  if (uppercase) {
    bool next = false;
    for (char *iter = strn; *iter != '\0'; ++iter)
    {
      if (E) {
        E = false;
        continue;
      }
      byte rus = (byte) * iter;
      if (rus == 208 && (byte) * (iter + 1) == 129) { // ёКостыли
        *iter = (char)209;
        *(iter + 1) = (char)145;
        E = true;
        continue;
      }
      if (rus == 209 && (byte) * (iter + 1) == 145) {
        *iter = (char)209;
        *(iter + 1) = (char)145;
        E = true;
        continue;
      }
      if (next) {
        if (rus >= 128 && rus <= 143) *iter = (char)(rus + 32);
        if (rus >= 176 && rus <= 191) *iter = (char)(rus - 32);
        next = false;
      }
      if (rus == 208) next = true;
      if (rus == 209) {
        *iter = (char)208;
        next = true;
      }
      *iter = toupper(*iter);
    }
  }
  while (strn[index])
  {
    if (strn[index] >= 0xBF)
    {
      switch (strn[index]) {
        case 0xD0: {
            if (strn[index + 1] == 0x81) {
              strn[index] = 0xA8;
              break;
            }
            if (strn[index + 1] >= 0x90 && strn[index + 1] <= 0xBF) strn[index] = strn[index + 1] + 0x30;
            break;
          }
        case 0xD1: {
            if (strn[index + 1] == 0x91) {
              //strn[index] = 0xB7;
              strn[index] = 0xB8;
              break;
            }
            if (strn[index + 1] >= 0x80 && strn[index + 1] <= 0x8F) strn[index] = strn[index + 1] + 0x70;
            break;
          }
      }
      int sind = index + 2;
      while (strn[sind]) {
        strn[sind - 1] = strn[sind];
        sind++;
      }
      strn[sind - 1] = 0;
    }
    index++;
  }
  return strn;
}

void DspCore::apScreen() {

}

void DspCore::initD(uint16_t &screenwidth, uint16_t &screenheight) {

}

void DspCore::drawLogo() {

}

void DspCore::drawPlaylist(uint16_t currentItem, char* currentItemText) {

}

void DspCore::clearDsp() {

}

void DspCore::drawScrollFrame(uint16_t texttop, uint16_t textheight, uint16_t bg) {

}

void DspCore::getScrolBbounds(const char* text, const char* separator, byte textsize, uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth) {

}

void DspCore::clearScroll(uint16_t texttop, uint16_t textheight, uint16_t bg) {

}

void DspCore::centerText(const char* text, byte y, uint16_t fg, uint16_t bg) {

}

void DspCore::rightText(const char* text, byte y, uint16_t fg, uint16_t bg) {

}

void DspCore::displayHeapForDebug() {

}

void DspCore::printClock(const char* timestr) {

}

void DspCore::drawVolumeBar(bool withNumber) {

}

void DspCore::drawNextStationNum(uint16_t num) {

}

void DspCore::frameTitle(const char* str) {

}

void DspCore::rssi(const char* str) {
;
}

void DspCore::ip(const char* str) {

}

void DspCore::set_TextSize(uint8_t s) {

}

void DspCore::set_TextColor(uint16_t fg, uint16_t bg) {

}

void DspCore::set_Cursor(int16_t x, int16_t y) {

}

void DspCore::printText(const char* txt) {

}

void DspCore::loop() {

}

#endif
