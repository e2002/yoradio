#ifndef utf8RusLCD_h
#define  utf8RusLCD_h

char* DspCore::utf8Rus(const char* str, bool uppercase) {
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
