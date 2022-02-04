#ifndef network_h
#define network_h

#define apSsid      "yoRadioAP"
#define apPassword  "12345987"

enum n_Status_e { CONNECTED, SOFT_AP, FAILED };

class Network {
  public:
    n_Status_e status;
  public:
    Network() {};
    void begin();
  private:
    void raiseSoftAP();
};

extern Network network;

#endif
