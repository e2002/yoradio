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
  Serial.println("Time to wake up neo!");
  if(YO_LED_BUILTIN!=255) pinMode(YO_LED_BUILTIN, OUTPUT);
#if defined (YO_LED_BUILTIN2)
  pinMode(YO_LED_BUILTIN2, OUTPUT);
  digitalWrite(YO_LED_BUILTIN2, LED_INVERT);

  //ledcAttachPin(YO_LED_BUILTIN2, 0); // assign a led pins to a channel
  /*
     Initialize channels
     channels 0-15, resolution 1-16 bits, freq limits depend on resolution
     ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
  */
  //ledcSetup(0, 8000, 7); // 12 kHz PWM, 8-bit resolution

#endif
#if defined (YO_LED_BUILTIN3)
  pinMode(YO_LED_BUILTIN3, OUTPUT);
  digitalWrite(YO_LED_BUILTIN3, LED_INVERT);
#endif
  if (yoradio_on_setup) yoradio_on_setup();
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
  if (config.getMode()==PM_SDCARD) player.initHeaders(config.station.url);
  player.lockOutput=false;
  if (config.store.smartstart == 1) player.sendCommand({PR_PLAY, config.store.lastStation});
}

#if 0
int loop_cnt;
#endif

#if 0
uint32_t loop_ts;
uint32_t delta;
#endif

void loop() {
  telnet.loop();
  if (network.status == CONNECTED || network.status==SDREADY) {
    player.loop();
    //loopControls();
  }
  loopControls();
  netserver.loop();

#if 0
  delta = millis() - loop_ts;
  if (delta>30)
    Serial.printf("loop delta %d ms\n", delta);
  loop_ts = millis();
#endif

#if 0
  loop_cnt++;
  if (loop_cnt == 10000) {
    loop_cnt = 0;
    //Serial.println("yoRadio on esp32-s3");

    /*
    static char __stats_buffer[1024];
    vTaskGetRunTimeStats(__stats_buffer);
    Serial.printf("%s\n", __stats_buffer);
    vTaskList(__stats_buffer);
    Serial.printf( "%s\n", __stats_buffer);
    */
  }
#endif
}

#include "src/core/audiohandlers.h"
