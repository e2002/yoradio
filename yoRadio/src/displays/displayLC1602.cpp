#include "../../options.h"

#if DSP_MODEL==DSP_1602I2C

#include "displayLC1602.h"
#include "../../player.h"
#include "../../config.h"
#include "../../network.h"

#ifndef SCREEN_ADDRESS
#define SCREEN_ADDRESS 0x27 ///< See datasheet for Address or scan it https://create.arduino.cc/projecthub/abdularbi17/how-to-scan-i2c-address-in-arduino-eaadda
#endif

const byte controlspaces[] = { CLOCK_SPACE, VOL_SPACE };

DisplayLC1602::DisplayLC1602(): LiquidCrystal_I2C(SCREEN_ADDRESS, 16, 2, I2C_SDA, I2C_SCL) {

}

void DisplayLC1602::apScreen() {
  setCursor(0,0);
  print("YORADIO AP MODE");
  setCursor(0,1);
  print(WiFi.softAPIP().toString().c_str());
}

void DisplayLC1602::initD(uint16_t &screenwidth, uint16_t &screenheight) {
  init();
  backlight();
  screenwidth = 16;
  screenheight = 2;
  swidth = screenwidth;
  sheight = screenheight;
  fillSpaces = true;
}

void DisplayLC1602::drawLogo() {

}

void DisplayLC1602::drawPlaylist(uint16_t currentItem, char* currentItemText) {
  centerText("NEXT STATION", 0, 0, 0);
  for (byte i = 0; i < PLMITEMS; i++) {
    plMenu[i][0] = '\0';
  }
  config.fillPlMenu(plMenu, currentItem, PLMITEMS);
  for (byte i = 0; i < PLMITEMS; i++) {
    strlcpy(currentItemText, plMenu[i], PLMITEMLENGHT - 1);
  }
}

void DisplayLC1602::clearDsp() {
  clear();
}

void DisplayLC1602::drawScrollFrame(uint16_t texttop, uint16_t textheight, uint16_t bg) {

}

void DisplayLC1602::getScrolBbounds(const char* text, const char* separator, byte textsize, uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth) {
  tWidth = strlen(text);
  tHeight = 1;
  sWidth = strlen(separator);
}

void DisplayLC1602::clearScroll(uint16_t texttop, uint16_t textheight, uint16_t bg) {
  for(uint16_t x=0; x<swidth-controlspaces[texttop]; x++){
    setCursor(x, texttop);
    print(" ");
  }
}

void DisplayLC1602::centerText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  byte x=(strlen(text)>swidth)?0:(swidth-strlen(text))/2;
  setCursor(x, y);
  print(text);
}

void DisplayLC1602::rightText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  byte x=swidth-strlen(text);
  setCursor(x-1, y);
  print(" ");
  setCursor(x, y);
  print(text);
}

void DisplayLC1602::displayHeapForDebug() {

}

void DisplayLC1602::printClock(const char* timestr) {
  rightText(timestr, 0, 0, 0);
}

void DisplayLC1602::printClock(struct tm timeinfo, bool dots, bool redraw) {

}

void DisplayLC1602::drawVolumeBar(bool withNumber) {
  char volstr[4];
  sprintf(volstr, "%02d", config.store.volume);
  if (withNumber) {;
    centerText("   ", 1, 0, 0);
    centerText(volstr, 1, TFT_LOGO, TFT_BG);
  }else{
    rightText("   ", 1, 0, 0);
    rightText(volstr, 1, TFT_LOGO, TFT_BG);
  }
}

void DisplayLC1602::drawNextStationNum(uint16_t num) {
  char numstr[7];
  sprintf(numstr, "%d", num);
  centerText("       ", 1, 0, 0);
  centerText(numstr, 1, TFT_LOGO, TFT_BG);
}

void DisplayLC1602::frameTitle(const char* str) {
  centerText(str, TFT_FRAMEWDT, TFT_LOGO, TFT_BG);
}

void DisplayLC1602::rssi(const char* str) {

}

void DisplayLC1602::ip(const char* str) {

}

void DisplayLC1602::set_TextSize(uint8_t s) {

}

void DisplayLC1602::set_TextColor(uint16_t fg, uint16_t bg) {

}

void DisplayLC1602::set_Cursor(int16_t x, int16_t y) {
  if(x<0) {
    xOffset=-x;
    x=0;
  }else{
    xOffset=0;
  }
  nextX=0;
  yOffset = y;
  setCursor(x, y);
}

void DisplayLC1602::printText(const char* txt) {
  char tmp[swidth+1] = {0};
  int16_t numchars = fillSpaces?swidth-controlspaces[yOffset]:swidth;
  strlcpy(tmp, txt+xOffset, numchars+1-nextX);
  print(tmp);
  xOffset=(int16_t)(strlen(txt)-xOffset)<=0?xOffset-strlen(txt):0;
  nextX=nextX+strlen(tmp);
  if(nextX>numchars) nextX=numchars;
  setCursor(nextX, yOffset);
}

void DisplayLC1602::loop() {
  if (checkdelay(SCROLLTIME, loopdelay)) {
    //display();
  }
  yield();
}

boolean DisplayLC1602::checkdelay(int m, unsigned long & tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

char* DisplayLC1602::utf8Rus(const char* str, bool uppercase) {
  int index = 0;
  static char strn[BUFLEN];
  static char newStr[BUFLEN];
  bool E = false;
  strlcpy(strn, str, BUFLEN);
  newStr[0] = '\0';
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

  while (strn[index])
  {
    if (strlen(newStr) > BUFLEN - 2) break;
    if (strn[index] >= 0xBF)
    {
      switch (strn[index]) {
        case 0xD0: {
            switch (strn[index + 1])
            {
              case 0x90: strcat(newStr, "A"); break;
              case 0x91: strcat(newStr, "B"); break;
              case 0x92: strcat(newStr, "V"); break;
              case 0x93: strcat(newStr, "G"); break;
              case 0x94: strcat(newStr, "D"); break;
              case 0x95: strcat(newStr, "E"); break;
              case 0x96: strcat(newStr, "ZH"); break;
              case 0x97: strcat(newStr, "Z"); break;
              case 0x98: strcat(newStr, "I"); break;
              case 0x99: strcat(newStr, "Y"); break;
              case 0x9A: strcat(newStr, "K"); break;
              case 0x9B: strcat(newStr, "L"); break;
              case 0x9C: strcat(newStr, "M"); break;
              case 0x9D: strcat(newStr, "N"); break;
              case 0x9E: strcat(newStr, "O"); break;
              case 0x9F: strcat(newStr, "P"); break;
              case 0xA0: strcat(newStr, "R"); break;
              case 0xA1: strcat(newStr, "S"); break;
              case 0xA2: strcat(newStr, "T"); break;
              case 0xA3: strcat(newStr, "U"); break;
              case 0xA4: strcat(newStr, "F"); break;
              case 0xA5: strcat(newStr, "H"); break;
              case 0xA6: strcat(newStr, "TS"); break;
              case 0xA7: strcat(newStr, "CH"); break;
              case 0xA8: strcat(newStr, "SH"); break;
              case 0xA9: strcat(newStr, "SHCH"); break;
              case 0xAA: strcat(newStr, "'"); break;
              case 0xAB: strcat(newStr, "YU"); break;
              case 0xAC: strcat(newStr, "'"); break;
              case 0xAD: strcat(newStr, "E"); break;
              case 0xAE: strcat(newStr, "YU"); break;
              case 0xAF: strcat(newStr, "YA"); break;
            }
            break;
          }
        case 0xD1: {
            if (strn[index + 1] == 0x91) {
              strcat(newStr, "YO"); break;
              break;
            }
            break;
          }
      }
      int sind = index + 2;
      while (strn[sind]) {
        strn[sind - 1] = strn[sind];
        sind++;
      }
      strn[sind - 1] = 0;
    } else {
      char Temp[2] = {(char) strn[index] , 0 } ;
      strcat(newStr, Temp);
    }
    index++;
  }
  return newStr;
}

#endif
