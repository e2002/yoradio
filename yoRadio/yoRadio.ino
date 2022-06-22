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

unsigned long checkMillis = 0;
unsigned long checkInterval = 3000;

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
#if PLAYER_FORCE_MONO
  player.forceMono(true);
#endif
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
    checkConnection();
  }
//  display.loop();
  netserver.loop();
}

void checkConnection(){
  if ((WiFi.status() != WL_CONNECTED) && (millis() - checkMillis >=checkInterval)) {
    bool playing = player.isRunning();
    if (playing) player.mode = STOPPED;
    display.putRequest({NEWMODE, LOST});
    Serial.println("Lost connection, reconnecting...");
    while(true){
      if (WiFi.status() == WL_CONNECTED) break;
      WiFi.disconnect();
      WiFi.reconnect();
      delay(3000);
    }
    display.putRequest({NEWMODE, PLAYER});
    if (playing) player.request.station = config.store.lastStation;
    checkMillis = millis();
#ifdef MQTT_HOST
    connectToMqtt();
#endif
  }
}
