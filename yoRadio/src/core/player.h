#ifndef player_h
#define player_h
#include "options.h"

#if I2S_DOUT!=255 || I2S_INTERNAL
  #include "../audioI2S/AudioEx.h"
#else
  #include "../audioVS1053/audioVS1053Ex.h"
#endif

#ifndef MQTT_BURL_SIZE
  #define MQTT_BURL_SIZE  512
#endif

#define PLERR_LN        64
#define SET_PLAY_ERROR(...) {char buff[512 + 64]; sprintf(buff,__VA_ARGS__); setError(buff);}

enum playerRequestType_e : uint8_t { PR_PLAY = 1, PR_STOP = 2, PR_PREV = 3, PR_NEXT = 4, PR_VOL = 5, PR_CHECKSD = 6 };
struct playerRequestParams_t
{
  playerRequestType_e type;
  int payload;
};

enum plStatus_e : uint8_t{ PLAYING = 1, STOPPED = 2 };

class Player: public Audio {
  private:
    uint32_t    _volTicks;   /* delayed volume save  */
    bool        _volTimer;   /* delayed volume save  */
    uint32_t    _resumeFilePos;
    plStatus_e  _status;
    char        _plError[PLERR_LN];
  private:
    void _stop(bool alreadyStopped = false);
    void _play(uint16_t stationId);
    void _loadVol(uint8_t volume);
  public:
    SemaphoreHandle_t playmutex=NULL;
    bool lockOutput = true;
    bool resumeAfterUrl = false;
    uint32_t sd_min, sd_max;
    #ifdef MQTT_ROOT_TOPIC
    char      burl[MQTT_BURL_SIZE];  /* buffer for browseUrl  */
    #endif
  public:
    Player();
    void init();
    void loop();
    void initHeaders(const char *file);
    void setError(const char *e);
    bool hasError() { return strlen(_plError)>0; }
    void sendCommand(playerRequestParams_t request);
    #ifdef MQTT_ROOT_TOPIC
    void browseUrl();
    #endif
    bool remoteStationName = false;
    plStatus_e status() { return _status; }
    void prev();
    void next();
    void toggle();
    void stepVol(bool up);
    void setVol(uint8_t volume);
    uint8_t volToI2S(uint8_t volume);
    void stopInfo();
    void setOutputPins(bool isPlaying);
    void setResumeFilePos(uint32_t pos) { _resumeFilePos = pos; }
};

extern Player player;

extern __attribute__((weak)) void player_on_start_play();
extern __attribute__((weak)) void player_on_stop_play();
extern __attribute__((weak)) void player_on_track_change();
extern __attribute__((weak)) void player_on_station_change();

#endif
