#ifndef player_h
#define player_h
#include "options.h"

#if I2S_DOUT!=255
#include "src/audioI2S/AudioEx.h"
#else
#include "src/audioVS1053/audioVS1053Ex.h"
#endif

enum audioMode_e { PLAYING, STOPPED };

struct audiorequest_t
{
  uint16_t station;
  int volume;
  bool doSave;
};

class Player: public Audio {
  public:
    audioMode_e mode;
    audiorequest_t request; 
    bool requesToStart;
  public:
    Player();
    void init();
    void loop();
    void zeroRequest();
    void play(byte stationId);
    void prev();
    void next();
    void toggle();
    void stepVol(bool up);
    void setVol(byte volume, bool inside);
    byte volToI2S(byte volume);
    void stopInfo();
};

extern Player player;


#endif
