#ifndef network_h
#define network_h
#include <Ticker.h>
#include "time.h"

#define apSsid      "yoRadioAP"
#define apPassword  "12345987"
#define TSYNC_DELAY 10800000    // 1000*60*60*3 = 3 hours

enum n_Status_e { CONNECTED, SOFT_AP, FAILED };

class Network {
  public:
    n_Status_e status;
    struct tm timeinfo;
  public:
    Network() {};
    void begin();
    void requestTimeSync(bool withTelnetOutput=false);
  private:
    Ticker ntimer;
    void raiseSoftAP();
};

extern Network network;

#endif
