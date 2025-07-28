#include "Arduino.h"
#include "core/options.h"
#include "core/config.h"
#include "core/telnet.h"
#include "core/player.h"
#include "core/display.h"
#include "core/network.h"
#include "core/netserver.h"
#include "core/controls.h"
#include "core/mqtt.h"
#include "core/optionschecker.h"
#include "core/timekeeper.h"

#if USE_OTA
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#include <NetworkUdp.h>
#else
#include <WiFiUdp.h>
#endif
#include <ArduinoOTA.h>
#endif

#if DSP_HSPI || TS_HSPI || VS_HSPI
SPIClass  SPI2(HOOPSENb);
#endif

extern __attribute__((weak)) void yoradio_on_setup();

#if USE_OTA
void setupOTA(){
  if(strlen(config.store.mdnsname)>0)
    ArduinoOTA.setHostname(config.store.mdnsname);
#ifdef OTA_PASS
  ArduinoOTA.setPassword(OTA_PASS);
#endif
  ArduinoOTA
    .onStart([]() {
      player.sendCommand({PR_STOP, 0});
      display.putRequest(NEWMODE, UPDATING);
      telnet.printf("Start OTA updating %s\n", ArduinoOTA.getCommand() == U_FLASH?"firmware":"filesystem");
    })
    .onEnd([]() {
      telnet.printf("\nEnd OTA update, Rebooting...\n");
      ESP.restart();
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      telnet.printf("Progress OTA: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      telnet.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        telnet.printf("Auth Failed\n");
      } else if (error == OTA_BEGIN_ERROR) {
        telnet.printf("Begin Failed\n");
      } else if (error == OTA_CONNECT_ERROR) {
        telnet.printf("Connect Failed\n");
      } else if (error == OTA_RECEIVE_ERROR) {
        telnet.printf("Receive Failed\n");
      } else if (error == OTA_END_ERROR) {
        telnet.printf("End Failed\n");
      }
    });
  ArduinoOTA.begin();
}
#endif

void setup() {
  Serial.begin(115200);
  if(REAL_LEDBUILTIN!=255) pinMode(REAL_LEDBUILTIN, OUTPUT);
  if (yoradio_on_setup) yoradio_on_setup();
  pm.on_setup();
  config.init();
  display.init();
  player.init();
  network.begin();
  if (network.status != CONNECTED && network.status!=SDREADY) {
    netserver.begin();
    initControls();
    display.putRequest(DSP_START);
    while(!display.ready()) delay(10);
    return;
  }
  if(SDC_CS!=255) {
    display.putRequest(WAITFORSD, 0);
    Serial.print("##[BOOT]#\tSD search\t");
  }
  config.initPlaylistMode();
  netserver.begin();
  telnet.begin();
  initControls();
  display.putRequest(DSP_START);
  while(!display.ready()) delay(10);
  #ifdef MQTT_ROOT_TOPIC
    mqttInit();
  #endif
  #if USE_OTA
    setupOTA();
  #endif
  #if LED_INVERT
    if(REAL_LEDBUILTIN!=255) digitalWrite(REAL_LEDBUILTIN, true);
  #endif
  if (config.getMode()==PM_SDCARD) player.initHeaders(config.station.url);
  player.lockOutput=false;
  if (config.store.smartstart == 1) {
    delay(250);
    player.sendCommand({PR_PLAY, config.lastStation()});
  }
  config.startAsyncServicesButWait();
  pm.on_end_setup();
}

void loop() {
  timekeeper.loop1();
  telnet.loop();
  if (network.status == CONNECTED || network.status==SDREADY) {
    player.loop();
#if USE_OTA
    ArduinoOTA.handle();
#endif
  }
  loopControls();
  //netserver.loop();
}

#include "core/audiohandlers.h"

/**************************************************************************
*   Plugin BacklightDown.
*   Ver.1.0 (Maleksm) for ёРадио 20.12.2024
*   Ver.1.1 (Trip5) 2025.07.19
***************************************************************************/
#if (BRIGHTNESS_PIN!=255) && (defined(DOWN_LEVEL) || defined(DOWN_INTERVAL))
#include <Ticker.h>

/* Основные константы настроек */
#ifdef DOWN_LEVEL
  const uint8_t brightness_down_level = DOWN_LEVEL;
#else
  const uint8_t brightness_down_level = 2;   /* lowest level brightness (from 0 to 255) */
#endif
#ifdef DOWN_INTERVAL
  const uint16_t Out_Interval = DOWN_INTERVAL;
#else
  const uint16_t Out_Interval = 60;         /* interval for BacklightDown in sec (60 sec = 1 min) */
#endif

  Ticker backlightTicker;
  Ticker rampTicker;
  uint8_t current_brightness;

  void stepBacklight() {
    if (current_brightness > brightness_down_level) {
      current_brightness -= 2;
      if (current_brightness < brightness_down_level) current_brightness = brightness_down_level;
      analogWrite(BRIGHTNESS_PIN, current_brightness);
    } else {
      rampTicker.detach();
    }
  }

  void backlightDown() {
    if (network.status != SOFT_AP) {
      backlightTicker.detach();
      current_brightness = map(config.store.brightness, 0, 100, 0, 255);
      rampTicker.attach_ms(30, stepBacklight);
    }
  }

  void brightnessOn() {
    backlightTicker.detach();
    rampTicker.detach();
    analogWrite(BRIGHTNESS_PIN, map(config.store.brightness, 0, 100, 0, 255));
    backlightTicker.attach(Out_Interval, backlightDown);
  }

  void yoradio_on_setup() { brightnessOn(); }      /* Backlight ON for Setup */
  void player_on_track_change() { brightnessOn(); } /* Backlight ON for track change */
  void player_on_start_play() { brightnessOn(); }  /* Backlight ON for start play */
  void player_on_stop_play() { brightnessOn(); }   /* Backlight ON for stop play */
  void ctrls_on_loop() {                            /* Backlight ON for reg. operations */
    if (!config.isScreensaver) {
      static uint32_t prevBlPinMillis;
      if ((display.mode() != PLAYER) && (millis() - prevBlPinMillis > 1000)) {
        prevBlPinMillis = millis();
        brightnessOn();
      }
    }
  }
#endif  /*  #if BRIGHTNESS_PIN!=255 */
