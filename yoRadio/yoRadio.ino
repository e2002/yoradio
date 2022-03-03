#include "Arduino.h"

#include "options.h"
#include "config.h"
#include "telnet.h"
#include "player.h"
#include "display.h"
#include "player.h"
#include "network.h"
#include "netserver.h"
#include "controls.h"
#include "mqtt.h"

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN, LOW);
  config.init();
  display.init();
  player.init();
  player.setOutputPins(false);
  network.begin();
  if (network.status != CONNECTED) {
    netserver.begin();
    display.start();
    return;
  }
  initControls();
  netserver.begin();
  telnet.begin();
  player.setVol(config.store.volume, true);
  display.start();
  if(config.store.smartstart==1) player.play(config.store.lastStation);
#ifdef MQTT_HOST
  mqttInit();
#endif
}

void loop() {
  if (network.status == CONNECTED) {
    telnet.loop();
    player.loop();
    loopControls();
  }
  display.loop();
  netserver.loop();
}
