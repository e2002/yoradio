#include "Arduino.h"

#include "options.h"
#if DSP_MODEL==DSP_DUMMY
#define DUMMYDISPLAY
#endif
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
  config.init();
  display.init();
  player.init();
  network.begin();
  if (network.status != CONNECTED) {
    netserver.begin();
    display.start();
    return;
  }
  netserver.begin();
  telnet.begin();
  player.setVol(config.store.volume, true);
  initControls();
  display.start();

#ifdef MQTT_HOST
  mqttInit();
#endif

  if (config.store.smartstart == 1) player.play(config.store.lastStation);
}

void loop() {
  if (network.status == CONNECTED) {
    telnet.loop();
    player.loop();
    loopControls();
  }
//  display.loop();
  netserver.loop();
}
