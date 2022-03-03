#include "options.h"

#define MQTT_HOST   "192.168.7.3"
#define MQTT_PORT   1883
#define MQTT_USER   ""
#define MQTT_PASS   ""

/*
 * HWID's:
 * 0: with yellow-blue SSD1306
 * 1: white SSD1306 without controls
 * 2: ST7735 with encoder
 * 3: Nokia 5110 dev board
 * 4: VS1053 dev
 * 5: VS1053 UNO3 Shield
 */
#if HWID==0
#define MQTT_ROOT_TOPIC  "yoradio/tv/"
#elif HWID==1
#define MQTT_ROOT_TOPIC  "yoradio/work/"
#elif HWID==2
#define MQTT_ROOT_TOPIC  "yoradio/st7735/"
#elif HWID==3
#define MQTT_ROOT_TOPIC  "yoradio/nokia5110/"
#elif HWID==4
#define MQTT_ROOT_TOPIC  "yoradio/vs1053dev/"
#elif HWID==5
#define MQTT_ROOT_TOPIC  "yoradio/kitchen/"
#endif
/*
Topics:
MQTT_ROOT_TOPIC/command     // Commands
MQTT_ROOT_TOPIC/status      // Player status
MQTT_ROOT_TOPIC/playlist    // Playlist URL
MQTT_ROOT_TOPIC/volume      // Current volume

Commands:
prev          // prev station
next          // next station
toggle        // start/stop playing
stop          // stop playing
start, play   // start playing
boot, reboot  // reboot
vol x         // set volume
play x        // play station x
*/
