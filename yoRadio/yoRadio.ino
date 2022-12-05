/* ============================================================================================================
 *    ёRadio
 * ============================================================================================================
 *    Web-radio based on
 *    ESP32-audioI2S     https://github.com/schreibfaul1/ESP32-audioI2S  
 *    or/and
 *    ESP32-vs1053_ext   https://github.com/schreibfaul1/ESP32-vs1053_ext
 *    libraries
 * ============================================================================================================
 *    Project home       https://github.com/e2002/yoradio
 *    Wiki               https://github.com/e2002/yoradio/wiki
 *    Описание на 4PDA   https://4pda.to/forum/index.php?s=&showtopic=1010378&view=findpost&p=112992611
 *    Как это прошить?   https://4pda.to/forum/index.php?act=findpost&pid=112992611&anchor=Spoil-112992611-2
 * ============================================================================================================
 *    Here goes!
 * ============================================================================================================
 */
#include "Arduino.h"
#include "src/core/options.h"
#include "src/core/config.h"
#include "src/core/telnet.h"
#include "src/core/player.h"
#include "src/core/display.h"
#include "src/core/network.h"
#include "src/core/netserver.h"
#include "src/core/controls.h"
#include "src/core/mqtt.h"
#include "src/core/optionschecker.h"

extern __attribute__((weak)) void yoradio_on_setup();

void setup() {
  Serial.begin(115200);
  if(LED_BUILTIN!=255) pinMode(LED_BUILTIN, OUTPUT);
  if (yoradio_on_setup) yoradio_on_setup();
  config.init();
  display.init();
  player.init();
  network.begin();
  if (network.status != CONNECTED) {
    netserver.begin();
    display.putRequest(DSP_START);
    while(!display.ready()) delay(10);
    return;
  }
  netserver.begin();
  telnet.begin();
  #if PLAYER_FORCE_MONO
    player.forceMono(true);
  #endif
  player.setVol(config.store.volume, true);
  initControls();
  display.putRequest(DSP_START);
  while(!display.ready()) delay(10);
  #ifdef MQTT_HOST
    mqttInit();
  #endif
  if (config.store.smartstart == 1) player.play(config.store.lastStation);
}

void loop() {
  telnet.loop();
  if (network.status == CONNECTED) {
    player.loop();
    loopControls();
    checkConnection();
  }
  netserver.loop();
}

void checkConnection(){
  static uint32_t checkMillis = 0;
  static uint32_t checkInterval = 3000;
  if ((WiFi.status() != WL_CONNECTED) && (millis() - checkMillis >=checkInterval)) {
    bool playing = player.isRunning();
    if (playing) player.mode = STOPPED;
    display.putRequest(NEWMODE, LOST);
    Serial.println("Lost connection, reconnecting...");
    while(true){
      if (WiFi.status() == WL_CONNECTED) break;
      WiFi.disconnect();
      WiFi.reconnect();
      delay(3000);
    }
    display.putRequest(NEWMODE, PLAYER);
    if (playing) player.request.station = config.store.lastStation;
    checkMillis = millis();
    #ifdef MQTT_HOST
      connectToMqtt();
    #endif
  }
}

//=============================================//
//              Audio handlers                 //
//=============================================//

void audio_info(const char *info) {
  if(config.store.audioinfo) telnet.printf("##AUDIO.INFO#: %s\n", info);
  #ifdef USE_NEXTION
    nextion.audioinfo(info);
  #endif
  if (strstr(info, "failed!") != NULL || strstr(info, " 404") != NULL || strstr(info, " 403") != NULL || strstr(info, "address is empty") != NULL) player.stop(info);
  if (strstr(info, "not supported") != NULL || strstr(info, "Account already in use") != NULL || strstr(info, "HTTP/1.0 401") != NULL) player.stop(info);
}

void audio_bitrate(const char *info)
{
  if(config.store.audioinfo) telnet.printf("%s %s\n", "##AUDIO.BITRATE#:", info);
  config.station.bitrate = atoi(info) / 1000;
  display.putRequest(DBITRATE);
  #ifdef USE_NEXTION
    nextion.bitrate(config.station.bitrate);
  #endif
  netserver.requestOnChange(BITRATE, 0);
}

bool printable(const char *info) {
  bool p = true;
  for (int c = 0; c < strlen(info); c++)
  {
    if ((uint8_t)info[c] > 0x7e || (uint8_t)info[c] < 0x20) p = false;
  }
  if (!p) p = (uint8_t)info[0] >= 0xC2 && (uint8_t)info[1] >= 0x80 && (uint8_t)info[1] <= 0xBF;
  return p;
}

void audio_showstation(const char *info) {
  if (strlen(info) > 0) {
    bool p = printable(info);
    config.setTitle(p?info:config.station.name);
    netserver.requestOnChange(TITLE, 0);
  }
}

void audio_showstreamtitle(const char *info) {
  DBGH();
  if (strstr(info, "Account already in use") != NULL || strstr(info, "HTTP/1.0 401") != NULL){
    player.stop(info);
    return;
  }
  if (strlen(info) > 0) {
    bool p = printable(info);
    #ifdef DEBUG_TITLES
      config.setTitle(DEBUG_TITLES);
    #else
      config.setTitle(p?info:config.station.name);
    #endif
    netserver.requestOnChange(TITLE, 0);
  }
}
