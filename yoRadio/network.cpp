#include "network.h"
#include "WiFi.h"
#include "display.h"
#include "options.h"

Network network;

void Network::begin() {
  config.initNetwork();
  if (config.ssidsCount == 0) {
    raiseSoftAP();
    return;
  }
  byte ls = (config.store.lastSSID == 0 || config.store.lastSSID > config.ssidsCount) ? 0 : config.store.lastSSID - 1;
  byte startedls = ls;
  byte errcnt = 0;
  WiFi.mode(WIFI_STA);
  char buf[40] = { 0 };
  while (true) {
    Serial.printf("Attempt to connect to %s\n", config.ssids[ls].ssid);
    snprintf(buf, sizeof(buf) - 1, "ATTEMPT TO %s", config.ssids[ls].ssid);
    display.bootString(buf, 110);
    WiFi.begin(config.ssids[ls].ssid, config.ssids[ls].password);
    strcpy(buf, ".");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      strcat(buf, ".");
      display.bootString(buf, 90);
      errcnt++;
      if (errcnt > 16) {
        errcnt = 0;
        ls++;
        if (ls > config.ssidsCount - 1) ls = 0;
        break;
      }
    }
    if(WiFi.status() != WL_CONNECTED && ls==startedls){
      raiseSoftAP();
      return;
    }
    if (WiFi.status() == WL_CONNECTED) {
      config.setLastSSID(ls + 1);
      break;
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
  status = CONNECTED;
}

void Network::raiseSoftAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, apPassword);
  status = SOFT_AP;
}
