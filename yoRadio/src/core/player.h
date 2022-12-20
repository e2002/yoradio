#ifndef player_h
#define player_h
#include "options.h"

#if I2S_DOUT!=255 || I2S_INTERNAL
  #include "../audioI2S/AudioEx.h"
#else
  #include "../audioVS1053/audioVS1053Ex.h"
#endif

enum audioMode_e { PLAYING, STOPPED };

struct audiorequest_t
{
  uint16_t station;
  int volume;
  bool doSave;
};
class Player: public Audio {
  private:
    uint32_t  volTicks;   /* delayed volume save  */
    bool      volTimer;   /* delayed volume save  */
  public:
    audioMode_e mode; 
    audiorequest_t request;
    bool requestToStart;
    void zeroRequest();
    SemaphoreHandle_t playmutex=NULL;
    bool lockOutput = true;
  public:
    Player();
    void init();
    void loop();
    void initHeaders(const char *file);
    void loopreader();
    void play(uint16_t stationId, uint32_t filePos=0);
    void stop(const char *nttl = NULL);
    void prev();
    void next();
    void toggle();
    void stepVol(bool up);
    void setVol(byte volume, bool inside);
    byte volToI2S(byte volume);
    void stopInfo();
    void setOutputPins(bool isPlaying);
};

extern Player player;

extern __attribute__((weak)) void player_on_start_play();
extern __attribute__((weak)) void player_on_stop_play();
extern __attribute__((weak)) void player_on_track_change();
extern __attribute__((weak)) void player_on_station_change();

#endif
