#ifndef telnet_h
#define telnet_h

#include <WiFi.h>

#define MAX_TLN_CLIENTS 5
#define MAX_PRINTF_LEN BUFLEN+50

class Telnet {
  public:
    Telnet() {};
    bool begin(bool quiet=false);
    void loop();
    void stop();
    void print(byte id, const char *buf);
    void print(const char *buf);
    void printf(byte id, const char *format, ...);
    void printf(const char *format, ...);
    void cleanupClients();
    void info();
  protected:
    WiFiServer server = WiFiServer(23);
    WiFiClient clients[MAX_TLN_CLIENTS];
    void emptyClientStream(WiFiClient client);
    void on_connect(const char* str, byte clientId);
    void on_input(const char* str, byte clientId);
  private:
    bool _isIPSet(IPAddress ip);
    void handleSerial();
};

extern Telnet telnet;

#endif
