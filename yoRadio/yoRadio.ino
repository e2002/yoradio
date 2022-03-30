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

#if CORE_FOR_LOOP_CONTROLS != 255
TaskHandle_t TaskCore0;
#ifndef CORE_FOR_LOOP_STACK_SIZE
#define CORE_FOR_LOOP_STACK_SIZE  16384
#endif
#endif

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN, LOW);
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
  if(CORE_FOR_LOOP_CONTROLS == 255){  
    initControls();
    display.start();
  }
  
#ifdef MQTT_HOST
  mqttInit();
#endif

#if CORE_FOR_LOOP_CONTROLS != 255
  BaseType_t coreId = xPortGetCoreID();
  BaseType_t newCoreId = (CORE_FOR_LOOP_CONTROLS==2)?!coreId:CORE_FOR_LOOP_CONTROLS;
  xTaskCreatePinnedToCore(
      loopCore0,                  /* Task function. */
      "TaskCore0",                /* name of task. */
      CORE_FOR_LOOP_STACK_SIZE,   /* Stack size of task */
      NULL,                       /* parameter of the task */
      1,                          /* priority of the task */
      &TaskCore0,                 /* Task handle to keep track of created task */
      newCoreId);                 /* pin task to core CORE_FOR_LOOP_CONTROLS */
#endif

  delay(500);
  if(config.store.smartstart==1) player.play(config.store.lastStation);
}

#if CORE_FOR_LOOP_CONTROLS != 255
void setupCore0(){
  initControls();
  display.start();
}

void loopCore0( void * pvParameters ){
  setupCore0();
  for(;;){
    micros();
    if (network.status == CONNECTED) {
      loopControls();
    }
    display.loop();
    vTaskDelay(10);
  }
}
#endif

void loop() {
  if (network.status == CONNECTED) {
    telnet.loop();
    player.loop();
    if(CORE_FOR_LOOP_CONTROLS==255) loopControls();
  }
  if(CORE_FOR_LOOP_CONTROLS==255 || network.status != CONNECTED) display.loop();
  netserver.loop();
}
