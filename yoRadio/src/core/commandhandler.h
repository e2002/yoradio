#ifndef commandhandler_h
#define commandhandler_h

#include <Arduino.h>

class CommandHandler {
public:
  bool exec(const char *command, const char *value, uint8_t cid=0);

private:
  static bool strEquals(const char *a, const char *b) {
    return strcmp(a, b) == 0;
  }
};

extern CommandHandler cmd;

#endif
