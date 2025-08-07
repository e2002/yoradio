# ёRadio
<img src="images/yologo.png" width="190" height="142">

##### Web-radio based on [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) or/and [ESP32-vs1053_ext](https://github.com/schreibfaul1/ESP32-vs1053_ext) library
---
- [Hardware](#hardware)
- [Connection tables](#connection-tables)
- [Software dependencies](#dependencies)
- [Hardware setup](#hardware-setup)
- [Quick start](#quick-start)
- [Detailed start](https://github.com/e2002/yoradio/wiki/How-to-flash)
- [Update](#update)
- [Update over web-interface](#update-over-web-interface)
- [Controls](Controls.md)
- [MQTT](#mqtt)
- [Home Assistant](#home-assistant)
- [More features](#more-features)
- [Plugins](#plugins)
- [Version history](#version-history)
- [Описание на 4PDA](https://4pda.to/forum/index.php?s=&showtopic=1010378&view=findpost&p=112992611)
---
#### NEW!
##### yoRadio Printed Circuit Boards repository:
[<img src="images/yopcb.jpg" width="830" height="auto" />](https://github.com/e2002/yopcb)

https://github.com/e2002/yopcb

---
<img src="images/img0_2.jpg" width="830" height="467">

##### More images in [Images.md](Images.md)

---
## Hardware
#### Required:
**ESP32 board**: https://aliexpress.com/item/32847027609.html \
**I2S DAC**, roughly like this one: https://aliexpress.com/item/1005001993192815.html \
https://aliexpress.com/item/1005002011542576.html \
or **VS1053b module** : https://aliexpress.com/item/32893187079.html \
https://aliexpress.com/item/32838958284.html \
https://aliexpress.com/item/32965676064.html

#### Optional:
##### Displays
- **ST7735** 1.8' or 1.44' https://aliexpress.com/item/1005002822797745.html
- or **SSD1306** 0.96' 128x64 I2C https://aliexpress.com/item/1005001621806398.html
- or **SSD1306** 0,91' 128x32 I2C https://aliexpress.com/item/32798439084.html
- or **Nokia5110** 84x48 SPI https://aliexpress.com/item/1005001621837569.html
- or **ST7789** 2.4' 320x240 SPI https://aliexpress.com/item/32960241206.html
- or **ST7789** 1.3' 240x240 SPI https://aliexpress.com/item/32996979276.html
- or **SH1106** 1.3' 128x64 I2C https://aliexpress.com/item/32683094040.html
- or **LCD1602** 16x2 I2C https://aliexpress.com/item/32305776560.html
- or **LCD1602** 16x2 without I2C https://aliexpress.com/item/32305776560.html
- or **SSD1327** 1.5' 128x128 I2C https://aliexpress.com/item/1005001414175498.html
- or **ILI9341** 3.2' 320x240 SPI https://aliexpress.com/item/33048191074.html
- or **ILI9341** 2.8' 320x240 SPI https://aliexpress.com/item/1005004502250619.html
- or **SSD1305 (SSD1309)** 2.4' 128x64 SPI/I2C https://aliexpress.com/item/32950307344.html
- or **SH1107** 0.96' 128x64 I2C https://aliexpress.com/item/4000551696674.html
- or **GC9106** 0.96' 160x80 SPI (looks like ST7735S, but it's not him) https://aliexpress.com/item/32947890530.html
- or **LCD2004** 20x4 I2C https://aliexpress.com/item/32783128355.html
- or **LCD2004** 20x4 without I2C https://aliexpress.com/item/32783128355.html
- or **ILI9225** 2.0' 220x176 SPI https://aliexpress.com/item/32952021835.html
- or **Nextion displays** - [more info](https://github.com/e2002/yoradio/tree/main/nextion)
- or **ST7796** 3.5' 480x320 SPI https://aliexpress.com/item/1005004632953455.html?sku_id=12000029911293172
- or **GC9A01A** 1.28' 240x240 https://aliexpress.com/item/1005004069703494.html?sku_id=12000029869654615
- or **ILI9488** 3.5' 480x320 SPI https://aliexpress.com/item/1005001999296476.html?sku_id=12000018365356570
- or **ILI9486** (Testing mode) 3.5' 480x320 SPI https://aliexpress.com/item/1005001999296476.html?sku_id=12000018365356568
- or **SSD1322** 2.8' 256x64 SPI https://aliexpress.com/item/1005003480981568.html
- or **ST7920** 2.6' 128x64 SPI https://aliexpress.com/item/32699482638.html
- or **ST7789** 2.25' 284x76 SPI https://aliexpress.ru/item/1005009016973081.html

(see [Wiki](https://github.com/e2002/yoradio/wiki/Available-display-models) for more details)

##### Controls
- Three tact buttons https://www.aliexpress.com/item/32907144687.html
- Encoder https://www.aliexpress.com/item/32873198060.html
- Joystick https://aliexpress.com/item/4000681560472.html \
https://aliexpress.com/item/4000699838567.html
- IR Control https://www.aliexpress.com/item/32562721229.html \
https://www.aliexpress.com/item/33009687492.html
- Touchscreen https://aliexpress.com/item/33048191074.html

##### RTC
- DS1307 or DS3231 https://aliexpress.com/item/4001130860369.html

---
## Connection tables
##### Use [this tool](https://e2002.github.io/docs/myoptions-generator.html) to build your own connection table and myoptions.h file.
<img src="images/myoptions-generator.png" width="830" height="527"><br />

https://e2002.github.io/docs/myoptions-generator.html

---
## Dependencies
#### Libraries:
**Library Manager**: Adafruit_GFX, Adafruit_ST7735\*, Adafruit_SSD1306\*, Adafruit_PCD8544\*, Adafruit_SH110X\*, Adafruit_SSD1327\*, Adafruit_ILI9341\*, Adafruit_SSD1305\*, TFT_22_ILI9225\* (\* depending on display model), OneButton, IRremoteESP8266, XPT2046_Touchscreen, RTCLib \
**Github**: ~~[ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer), [AsyncTCP](https://github.com/me-no-dev/AsyncTCP), [async-mqtt-client](https://github.com/marvinroger/async-mqtt-client) (if you need MQTT support)~~ <<< **starting with version 0.8.920, these libraries have been moved into the project, and there is no need to install them additionally.**

#### Tool:
[ESP32 Filesystem Uploader](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/)

**See [wiki](https://github.com/e2002/yoradio/wiki/How-to-flash#preparing) for details**

---
## Hardware setup
Don't edit the options.h! \
Hardware is adjustment in the **[myoptions.h](examples/myoptions.h)** file.

**Important!**
You must choose between I2S DAC and VS1053 by disabling the second module in the settings:
````c++
// If I2S DAC used:
#define I2S_DOUT      27
#define VS1053_CS     255
// If VS1053 used:
#define I2S_DOUT      255
#define VS1053_CS     27
````
Define display model:
````c++
#define DSP_MODEL  DSP_ST7735 /*  default - DSP_DUMMY  */
````
The ST7735 display submodel:
````c++
#define DTYPE INITR_BLACKTAB // 1.8' https://aliexpress.ru/item/1005002822797745.html
//#define DTYPE INITR_144GREENTAB // 1.44' https://aliexpress.ru/item/1005002822797745.html
````
Rotation of the display:
````c++
#define TFT_ROTATE 3 // 270 degrees
````
##### Note: If INITR_BLACKTAB dsp have a noisy line on one side of the screen, then in Adafruit_ST7735.cpp:
````c++
  // Black tab, change MADCTL color filter
  if ((options == INITR_BLACKTAB) || (options == INITR_MINI160x80)) {
    uint8_t data = 0xC0;
    sendCommand(ST77XX_MADCTL, &data, 1);
    _colstart = 2; // ← add this line
    _rowstart = 1; // ← add this line
  }
````

---
## Quick start
<img src="images/board4.jpg" width="830" height="473"><br />

- <span style="color: red; font-weight: bold; font-size: 22px;text-decoration: underline;">Arduino IDE version 2.x.x is not supported. Use Arduino IDE 1.8.19</span>
- <span style="color: red; font-weight: bold; font-size: 22px;text-decoration: underline;">ESP32 core version 2.0.0 or higher is [required](https://github.com/espressif/arduino-esp32)!</span>

1. Generate a myoptions.h file for your hardware configuration using [this tool](https://e2002.github.io/docs/myoptions-generator.html).
2. Put myoptions.h file next to yoRadio.ino.
3. Replace file Arduino/libraries/Adafruit_GFX_Library/glcdfont.c with file [yoRadio/fonts/glcdfont.c](yoRadio/fonts/glcdfont.c)
4. Restart Arduino IDE.
5. In ArduinoIDE - upload sketch data via Tools→ESP32 Sketch Data Upload ([it's here](images/board2.jpg))
6. Upload the sketch to the board
7. Connect to yoRadioAP access point with password 12345987, go to http://192.168.4.1/ configure and wifi connections.  \
_\*this step can be skipped if you add WiFiSSID WiFiPassword pairs to the [yoRadio/data/data/wifi.csv](yoRadio/data/data/wifi.csv) file (tab-separated values, one line per access point) before uploading the sketch data in step 1_
8. After successful connection go to http://\<yoipaddress\>/ , add stations to playlist (or import WebStations.txt from KaRadio)
9. Well done!

**See [wiki](https://github.com/e2002/yoradio/wiki/How-to-flash#build--flash) for details**

---
## Update
1. Backup your settings: \
download _http://\<yoradioip\>/data/playlist.csv_ and _http://\<yoradioip\>/data/wifi.csv_ and place them in the yoRadio/data/data/ folder
2. In ArduinoIDE - upload sketch data via Tools→ESP32 Sketch Data Upload
3. Upload the sketch to the board
4. Go to page _http://\<yoradioip\>/_ in the browser and press Ctrl+F5 to update the scripts.
5. Well done!

## Update over web-interface
1. Backup your settings: \
download _http://\<yoradioip\>/data/playlist.csv_ and _http://\<yoradioip\>/data/wifi.csv_ and place them in the yoRadio/data/data/ folder
2. Get firmware binary: Sketch → Export compiled binary
3. Get SPIFFS binary: disconnect ESP32 from your computer, click on **ESP32 Data Sketch Upload**. \
 You will get an error and file path

 <img src="images/getspiffs.jpg" width="830" height="208">

4. Go to page _http://\<yoradioip\>/update_ and upload yoRadio.ino.esp32.bin and yoRadio.spiffs.bin in turn, checking the appropriate upload options.
5. Well done!

---
## MQTT
1. Copy file examples/mqttoptions.h to yoRadio/ directory
2. In the mqttoptions.h file, change the options to the ones you need
3. Well done!

---
## Home Assistant
<img src="images/ha.jpg" width="500" height="270"><br />

0. Requires [MQTT integration](https://www.home-assistant.io/integrations/mqtt/)
1. Copy directory HA/custom_components/yoradio to .homeassistant/custom_components/
2. Add yoRadio entity into .homeassistant/configuration.yaml ([see example](HA/example_configuration.yaml))
3. Restart Home Assistant
4. Add Lovelace Media Player card to UI (or [mini-media-player](https://github.com/kalkih/mini-media-player) card)
5. Well done!

---
## More features
- Can add up to 65535 stations to a playlist. Supports and imports [KaRadio](https://github.com/karawin/Ka-Radio32) playlists (WebStations.txt)
- Telnet with KaRadio format output \
  **see [list of available commands](https://github.com/e2002/yoradio/wiki/List-of-available-commands-(UART-telnet-GET-POST))**

- MQTT support \
 **Topics**: \
 **MQTT_ROOT_TOPIC/command**     - Commands \
 **MQTT_ROOT_TOPIC/status**      - Player status \
 **MQTT_ROOT_TOPIC/playlist**    - Playlist URL \
 **MQTT_ROOT_TOPIC/volume**      - Current volume \
 **Commands**: \
 **prev**          - prev station \
 **next**          - next station \
 **toggle**        - start/stop playing \
 **stop**          - stop playing \
 **start, play**   - start playing \
 **boot, reboot**  - reboot \
 **volm**          - step vol down \
 **volp**          - step vol up \
 **vol x**         - set volume \
 **play x**        - play station x

- Home Assistant support

---
## Plugins
The `Plugin` class serves as a base class for creating plugins that hook into various system events.  
To use it, inherit from `Plugin` and override the necessary virtual methods.  
Place your new class in the `src/plugins/<MyPlugin>` directory
More details can be found in the comments within the `yoRadio/src/pluginsManager/pluginsManager.h` file and at [here](https://github.com/e2002/yoradio/blob/main/yoRadio/src/pluginsManager/README.md).  
Additional examples are provided in the `examples/plugins` folder.
Work is in progress...

---
## Version history

### v0.9.574 Trip5/2025.08.07

- Trip5's changes merged with v0.9.574

#### v0.9.574
- fixed compilation error for certain displays when `#define DSP_INVERT_TITLE false` is set
- fixed compilation error for `DSP_DUMMY`

#### v0.9.570
- added support for ST7789 284x76 2.25' SPI displays https://aliexpress.ru/item/1005009016973081.html \
  note: the brightness pin of this display should be pulled up to GND

### v0.9.561
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required !!!**\
  or-> just upload `yoRadio/data/www/script.js.gz` to Webboard Uploader http://radioipaddr/webboard \
  After updating please clear browser cache.
- fixed error when switching to SD Card mode
- fixed issue causing random reboots
- fixed preview playback bug in Playlist Editor

### v0.9.555
- fixed error "assert failed: udp_new_ip_type /IDF/components/lwip/lwip/src/core/udp.c:1278 (Required to lock TCPIP core functionality!)"\
  part #2
- weather synchronization code rewritten

### v0.9.553
- fix "No 'Access-Control-Allow-Origin' header is present on the requested resource" on saving playlist\
  just reupload the file `script.js.gz` with Webboard uploader
- fixed error "assert failed: udp_new_ip_type /IDF/components/lwip/lwip/src/core/udp.c:1278 (Required to lock TCPIP core functionality!)"
- fixed error "Exception in status_listener when handling msg" in HA component

### v0.9.552 Trip5/2025.07.28

- Trip5's changes merged with 0.9.552
- likely unfixed: ESP cores below 3.0.0 (should I undo maleksm's audio decoders?)

### v0.9.552
- fixed compilation error for ESP cores version below 3.0.0\
  Thanks to @salawalas ! https://github.com/e2002/yoradio/pull/197/
- disabled websocket reconnection on all pages except the start page "/"\
  just reupload the file `script.js.gz`

### v0.9.550
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!**\
or-> just upload all files from data/www (11 pcs) to Webboard Uploader http://radioipaddr/webboard
- fixed the issue with selecting all rows in the playlist editor
- netserver optimization – Part 2
- cleanup – Part 1
- page class migrated from LinkedList to std::list (Huge thanks to @vortigont!)
  https://github.com/vortigont/yoradio/commit/b6d7fdd973bfa7395a894ceceaef40925b3f5161#diff-5df2b3b2edb81bdf3594469c55ec7093c641d13a2555a0cea25e7f3380c7de1a
- added WebSocket connection check in the web interface
- buffer indicator added to the web interface
- display performance optimization (Big thanks to @vortigont!) https://github.com/e2002/yoradio/pull/196/
- audio buffer size setting for modules without PSRAM moved to the web interface (new value applies after reboot, optimal value is 7)
- added option in the web interface to disable the Telnet server
- added option in the web interface to enable the Watchdog that stops connect to broken streams
- settings for time and weather synchronization intervals have been added to the web interface
- bug fixes, optimization


### v0.9.533 Trip5/2025.07.23

- minor additions to UI / display options
  - one-click option ignores long-press right/left
  - 12-hour clock
  - hide volume page (instead of using a `#define`)

### v0.9.533 Trip5/2025.07.20

- BREAKING CHANGES
  - requires larger app partitions
    - if using 4MB flash sizeESP32, use the `ESP32-4MB.csv` as your partition file (see the text file for more)
  - many settings will be reset to defaults
  - notes were added to `config.cpp` how to handle breaking and non-breaking store updates in the future
  - future re-flashes should not lose settings after this update
  - capability to update firmware and download SPIFFS files from online has been added
- library dependency: `bblanchon/ArduinoJson@^6.21.3`
  - https://github.com/bblanchon/ArduinoJson
- EEProm storage removed in favor of Preferences
  - config.store variables may still be used as before
  - no need to handle store version changes except by adding old ones to the "remove" list in `config.cpp`
    - char handling improved throughout (size may be changed later with 1 edit to `config.h`)
      - affects timezone, mdnsname, weather coordinate variables
- shorter searching for Wi-fi message
- LED_INVERT fixed (`#define LED_INVERT`)
- fixes for screens that can't display certain characters (add to `myoptions.h`)
  - `#define YO_FIX` // changes ёRadio to yoRadio for screens that can't print ё
  - `#define PRINT_FIX` // fix Chinese certain screens so they don't display gibberish
    - hijacks `utf8RusGFX.h` (which is meant for Russian model displays?)
      - does not interrupt this function unless `PRINT_FIX` defined
    - will transliterate as many European characters as possible by dropping accents
    - will transliterate Cyrillic... probably badly
    - will transliterate various punctuation and currency symbols
    - any characters that can't be handled will be replaced by a space
- ESPFileUpdater can update and download files (used in multiple places)
  - a new library created for this project
  - this may be used to download / update any file from online to SPIFFS
  - add `#define ESPFILEUPDATER_DEBUG` to `myoptions.h` to get verbose output
- Online updating for pre-built BIN files and other assets
  - Uses ESPFileUpdater when SPIFFS is empty or incomplete if the running program is a pre-built firmware
  - the online URL path used to download SPIFFS file assets (for the running version):
    - `#define FILESURL "https://github.com/trip5/yoradio/releases/download/2025.07.19/"`
  - the online URL path used to OTA update firmware:
    - `#define UPDATEURL "https://github.com/trip5/yoradio/releases/latest/download/"`
  - the online file that the current version can compare it's version against:
    - `#define CHECKUPDATEURL "https://raw.githubusercontent.com/trip5/yoradio/refs/heads/trip5/yoRadio/src/core/options.h"`
  - the above file must contain a line that is defined by this setting:
    - `#define VERSIONSTRING "#define YOVERSION"` (followed by a version string)
  - which `.bin` file to be used for online OTA updates can be specified as:
    - `#define FIRMWARE "firmware_sh1106_pcm_remote.bin"`
  - all of these can be automatically defined by `platformio.ini` amd `myoptions.h`
    - an example of this automatic building is at `https://github.com/trip5/yoradio/tree/trip5/`
    - check out `platformio-trip5-builds.yml` to see how firmwares are built and files made available online
      - First commit your changes to Github
      - Tag your local most recent commit and push it to Github:
        - `git tag 2025.07.20`
        - `git push origin 2025.07.20`
      - Re-doing a tagged release:
        - `git tag -d 2025.07.20`
        - `git tag -a 2025.07.20 -m "2025.07.20"`
        - `git push origin 2025.07.20 --force`
    - this means you can just download a .bin file and flash from a command line (see the Release page for detailed instructions)
- implements proper timezones
  - uses ESPFileUpdater to download an up-to-date json to be used as a selector in the WebUI
  - fetches from https://raw.githubusercontent.com/trip5/timezones.json/refs/heads/master/timezones.gz.json
    - this can be a gzipped or regular json
    - uses github workflow to automatically update whenever tzdb has changed
    - also created for this project
    - another json.gz can be used by a define in `myoptions.h` (default in quotes)
      - `#define TIMEZONES_JSON_URL "https://raw.githubusercontent.com/trip5/timezones.json/master/timezones.json.gz"`
  - will handle Daylight Savings Times
    - does not affect timed functions that use ticks (screensaver, weather)
    - may have side-effects on other timed functions that don't use ticks
  - timezone offset completely removed
  - timezones may be changed through WebUI or telnet
  - telnet has some extra functions added
    - `TZO ±X` now just changes to a GMT-type (non-standard is OK)
    - `TZO ±X:XX` now just changes to a custom timezone (based on GMT-type)
    - `TZPOSIX` command can create a custom timezone (be careful)
    - `PLAY url` can play a station not on the playlist
  - Nextion displays only show timezone now, cannot change timezone
    - there may be a good way to implement with + & - with GMT timezones (see `telnet.cpp` for some idea)
    - I have no Nextion display to test (sorry!)
- regional defaults now be defined in `myoptions.h` (defaults in quotes):
  - `#define TIMEZONE_NAME "Europe/Moscow"`
  - `#define TIMEZONE_POSIX "MSK-3"`
  - `#define SNTP1 "pool.ntp.org"`
  - `#define SNTP2 "0.ru.pool.ntp.org"`
  - `#define WEATHERLAT "55.7512"`
  - `#define WEATHERLON "37.6184"`
  - all can still be edited using WebUI
- Radio station search via Radio Browser API
  - performs search queries to a https://www.radio-browser.info/ server
  - uses ESPFileUpdater to download an up-to-date json of API servers
    - another json can be used by a define in options.h
      - `#define RADIO_BROWSER_SERVERS_URL "https://all.api.radio-browser.info/json/servers"` in `myoptions.h`
  - handles down API servers gracefully (and they do go down fairly often)
  - uses ESPFileUpdater to download JSON search results directly from the API to the ESP's file system
    - previous searches are saved and not lost on reboot (100 results, page number)
    - saved searches will be deleted on reboot if they are older than 24 hours
  - search results shows station name, country code, codec, bitrate
  - stations can be previewed (through the radio) with a play button that does not add it to the playlist
    - if matches URL in the playlist will play the station from the playlist
  - stations can be added with a plus button
    - will not add a station which has the same URL as one already in the playlist (http & https considered same)
- Playback queue (either from playlist, search, or telnet) put into an RTOS task that runs in the background
  - stability improved but slight delay added
- Improved JSON and CSV file importing
  - CSV import made more resilient
    - can import any type of list with fields separated by tabs or spaces
    - missing name or ovol fields will continue to be imported
    - for example, all of these are valid:
      - space-separated values:
        - `nap.casthost.net:8793/stream Intra Nature Radio`
        - `Ambient Sleeping Pill http://radio.stereoscenic.com/asp-s`
        - `Super Relax FM https://streams.radio.menu/listen/super-relax-fm/radio.mp3 0`
      - tab-separated values:
        - `Traxx FM - Ambient	http://traxx011.ice.infomaniak.ch/traxx011-low.mp3	0`
        - `Positively Meditation	0	https://streaming.positivity.radio/pr/posimeditation/icecast.audio`
        - `0	Cryosleep	http://streams.echoesofbluemars.org/8000/cryosleep`
        - `https://ice4.somafm.com/darkzone-128-mp3	Soma FM - Dark Zone 0`
      - just the URL
        - `https://ice4.somafm.com/dronezone-128-mp3`
    - the old CSV parser is still used for the yoRadio playlist internally
  - JSON import made more resilient
    - can handle line-by-line files that are not enclosed in [ ]
      - ie. each line looks like
        - `{"name":"Swinging radio 60s","host":"http://s2.xrad.io","file":"/8058/stream","port":"0","ovol":"0"}`
    - can handle proper json files where all entries are enclosed in [ ]
      - the field "url_resolved" will be preferred
      - fallback to "url"
      - finally, fallback to "host" and "file" and "port" - which will be combined into a url
  - in both parsers, only the url is mandatory
    - if "name" is absent, it will be assumed to be the url even without http(s)://
    - if "ovol" is absent, is will be assumed to be 0
- Added some of Maleksm's additions and changes from v0.9.434m (04.04.25) (with some changes)
  - includes improved decoding for VS1053 and PCM decoder and various codecs
  - ESP8266 support completely removed
  - includes BacklightDown plugin (tweaked to be non-blocking)
    - dims the display after awhile
    - `#define BRIGHTNESS_PIN` must be in your `myoptions.h`
    - `#define DOWN_LEVEL 2` (brightness level 0 to 255, default 2 )
    - `#define DOWN_INTERVAL 60` (seconds to dim, default 60 = 60 seconds)
  - use `#define HIDE_VOLPAGE` to hide the separate page showing volume (uses the progress bar instead)

### v0.9.533
- fixed compilation error for esp32 core version lower than 3.0.0
- fixed error setting display brightness to 1
- fixed error setting IR tolerance value (upload a new file `options.html.gz` via WEB Board Uploader and press Ctrl+F5 on the settings page)

### v0.9.530
- optimization of webserver/socket code in netserver.cpp, part#1
- added support for ArduinoOTA (OTA update from Arduino IDE) (disabled by default)\
  to enable you need to add to myoptions.h: `#define USE_OTA true`\
  set password: in myoptions.h `#define OTA_PASS "myotapassword12345"`
- in web interface added basic HTTP authentication capability (disabled by default)\
  to enable you need to add to myoptions.h:\
  `#define HTTP_USER "user"`\
  `#define HTTP_PASS "password"`
- added "emergency firmware uploader" form (for unforeseen cases) http://ipaddress/emergency
- added config (sys.config) telnet command that displays the same information usually shown over serial at boot.
- bug fixes 🪲

### v0.9.515
- fixed a bug with resetting all parameters when resetting only one section of parameters

### v0.9.512
- fixed bug with saving ntp server #1 value

### v0.9.511
In this version, the contents of the data/www directory have changed, so that the first time you flash it, you will be greeted by WEB Board Uploader. Just upload all the files from data/www (11 pcs) to it\
or -> **!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!**
- fixed a bug with saving smartstart mode
- fixed a bug with no restart when initially uploading files to spiffs
- fixed a bug with hanging on unavailable hosts
- fixed a bug with attempting to connect with an empty playlist
- fixed a bug with passing strings with quotes in mqtt
- fixing some other bugs
- web interface rewritten from scratch (well, almost), bugs added 👍
- added listening to links in the browser in playlistEditor
- buttons reboot (reboot) format (spiffs format) and reset (reset settings to default) have been added to the settings
- the beginnings of theming (theme.css) (just a list of global colors that can be changed, and then uploaded to theme.css via WB uploader)

### v0.9.434
- fixed the issue with exiting Screensaver Blank Screen mode via button presses and IR commands.
- reduced the minimum frequency for tone control on I2S modules to 80Hz.
- increased the display update task delay to 10 TICKS.
  to revert to the previous setting, add `#define DSP_TASK_DELAY 2` to `myoptions.h`.
- when ENCODER2 is connected, the UP and DOWN buttons now work as PREV and NEXT (single click).
- implemented backlight off in Screensaver Blank Screen mode.

### v0.9.428
- fixed freezing after SD scanning during playback
- AsyncWebSocket queue increased to 128
- fixed VU meter  overlapping the clock on displays
- fixed Guru Meditation error when loading in SD mode with SD card removed

### v0.9.420
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!**
- added screensaver mode during playback, configurable via the web interface, pull request[#129](https://github.com/e2002/yoradio/pull/129)
- added blank screen mode to screensaver, configurable via the web interface, pull request[#129](https://github.com/e2002/yoradio/pull/129)
  Thanks to @trip5 for the amazing code!
- speeding up indexing of SD cards (advice - don't put all files in one folder)
- i don't remember (honestly) why the AsyncTCP server worked on the same core with the player, now it works on the same core with the display
  `#define CONFIG_ASYNC_TCP_RUNNING_CORE 0`
- bug fixes

### v0.9.412
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!**
- added mDNS support, configurable via the web interface, pull[#125](https://github.com/e2002/yoradio/pull/125)
- added a setting that allows you to switch stations with the UP and DOWN buttons immediately, bypassing the playlist, configurable via the web interface, pull[#125](https://github.com/e2002/yoradio/pull/125)

### v0.9.399
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!**
- added a screensaver mode, configurable via the web interface.
- changes to the tone control algorithm for the VS1053.

### v0.9.390
- updated the VU meter algorithms - shamelessly borrowed from @schreibfaul1, ([thanks a lot!](https://github.com/schreibfaul1/ESP32-audioI2S/blob/1296374fc513a6d6bfaa3b1ca08f6ba938b18d99/src/Audio.cpp#L5030))
- fixed the magic error "HSPI" redefined.

### v0.9.380
- fixed compilation error for ESP32 cores >= 3.1.0
- fixed freezing error with incorrectly configured RTC module
- [www|uart|telnet] new command `mode` - change SD/WEB mode. (0 - WEB, 1 - SD, 2 - Toggle)
  example: http://\<ipaddress\>/?mode=2

#### v0.9.375
- fixed the issue with saving settings for TIMEZONE.

### v0.9.373
- fixed the issue with displaying the settings page on fresh ESP modules after saving the weather key
 (a [reset](https://github.com/e2002/yoradio/wiki/List-of-available-commands-(UART-telnet-GET-POST)) may be required)

#### v0.9.370
- fixed the issue with saving settings on fresh ESP modules.

#### v0.9.369
- fixed the issue with the non-functional HSPI bus

#### v0.9.368
- SD Card - optimization and bug fixes
- Config - improvements and bug fixes
- Added stream format display in the web interface **!!! A full update is required, including SPIFFS Data !!!**  
  *(Alternatively, upload the new `style.css.gz` and `script.js.gz` files via the web interface.)*
- The content of `yoRadio.ino` has been moved to `src/main.cpp`
- [www|uart|telnet] new command: `reset` - resets settings to default values. [More details](https://github.com/e2002/yoradio/wiki/List-of-available-commands-(UART-telnet-GET-POST))
- Fixed compilation error: `'ets_printf' was not declared in this scope`

#### v0.9.351
- fixed freezing when loading without plugins in some configurations "running dots"

#### v0.9.350
- **Added parameters for configuring `LED_BUILTIN` on ESP32S3 modules:**
  - `USE_BUILTIN_LED`: Determines whether to use the built-in `LED_BUILTIN` (default is `true`).
  - `LED_BUILTIN_S3`: Specifies a custom pin for the built-in `LED_BUILTIN`. Used in combination with `USE_BUILTIN_LED = false` (default is `255`).

  **Note:** For ESP32S3 boards, no changes are required by default; the onboard LED will work as expected.  
  These settings were added to allow disabling the built-in LED or reassigning it to a custom pin.

- **New class for plugin management**, enabling multiple plugins to be assigned to each function.  
  More details can be found in the comments within the `yoRadio/src/pluginsManager/pluginsManager.h` file and at [here](https://github.com/e2002/yoradio/blob/main/yoRadio/src/pluginsManager/README.md).  
  Additional examples are provided in the `examples/plugins` folder.
  **Backward compatibility:** The old method of adding plugins will remain functional for some time in future versions but will eventually be deprecated and removed.

#### v0.9.342b
- fixed compilation error for OLED displays

#### v0.9.340b
- fixed compilation error audioVS1053Ex.cpp:181:5: error: 'sdog' was not declared in this scope

#### v0.9.337b (homeassistant component)
- fixed the error of subscribing to mqtt topic on some systems

#### v0.9.337b
- added support for Arduino ESP32 v3.0.0 and later
- disabled SD indexing on startup; now the card is indexed only if the `data/index.dat` file is missing from the card
- IRremoteESP8266 library integrated into the project (`yoRadio/src/IRremoteESP8266`)

#### v0.9.313b
- added support for ESP32-S3 boards (ESP32 S3 Dev Module) (esp32 cores version 3.x.x is not supported yet)
- fixes in displaying sliders in the web interface

#### v0.9.300 (homeassistant component)
- HA component >> bug fixes in the component for newer versions of Home Assistant

#### v0.9.300
- added the ability to play SDCARD without an Internet connection. More in [Wiki](https://github.com/e2002/yoradio/wiki/A-little-about-SD-CARD-and-RTC)

#### v0.9.280
- fixed an issue where it was impossible to reconnect when the WiFi connection was lost

#### v0.9.273
- fixed an "Guru Meditation Error" when playing streams with the ESP32 v2.0.10 and higher core installed

#### v0.9.260
- fixed date display bug for ILI9488/ILI9486 displays

#### v0.9.259
- fixed a hang bug when switching to SD mode after removing the SD
- fixed a hangup error when the connection to the stream was lost in WEB mode

#### v0.9.250
- added support for DS1307 or DS3231 RTC module (you need to install the RTCLib library in the library manager)
- setup
```
#define RTC_MODULE	DS3231	/* or DS1307	*/
#define RTC_SDA       <pin>
#define RTC_SCL       <pin>
```

#### v0.9.242
- fixed a hang bug when scrolling through an SD playlist with an encoder in configurations with VS1053B
- fixed a hang bug when quickly switching SD / WEB modes from the WEB interface in configurations with VS1053B
- fixes in the logic of work

#### v0.9.236
- fix compilation error 'class NetServer' has no member named 'resetQueue'

#### v0.9.235
- SD card playlist moved from SPIFFS to SD card
- new parameter #define SD_MAX_LEVELS - Search depth for files on SD card
- fixed bugs with SD card in multi-threaded mode

#### v0.9.220
- fixed SD prelist indexing error when switching Web>>SD
- fixed a bug of switching to the next track when accidentally playing SD
- fixed import of large playlists (tried). PS: import playlist size is limited by SPIFFS size (SPIFFS.totalBytes()/100*65-SPIFFS.usedBytes() = approximately 60kb )
- new url parameter - http://YPRADIOIP/?clearspiffs - for clearing tails from SD playlist
- optimization of the issuance of the WEB-interface
- brought back the functionality of the track slider
- fixing bugs in the application logic

#### v0.9.201
- fixed a bug when importing a playlist

#### v0.9.200
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!**
- implementation of WEB/SD mode switching without reboot
- replacement of SD cards without turning off the power
- switching WEB / SD from the web interface. full update required, including SPIFFS Data
- fixing the Home Assistant integration behavior logic
- SD_HSPI parameter now works. Pins HSPI - 13(MOSI) 12(MISO) 14(CLK)
- new parameter SD_SPIPINS. ```#define SD_SPIPINS sck, miso, mosi```
- sck, miso, mosi - any available pins. Used for "TTGO Tm Music Album" boards ```#define SD_SPIPINS 14, 2, 15```
- fixed a bug with garbage appearing on display ILI9225
- the slider for moving along the SD track is temporarily not working.
- bug fixes

#### v0.9.180
- OneButton library moved to the project

#### v0.9.177
- fixed bitrate display error when playing SD on VS1053B modules

#### v0.9.174
- added forced shutdown of smartstart when WebSocket freezes on problem stations
- added bitrate icon when playing files from SD card
- fix html markup errors

#### v0.9.161
- fixed errors 403 Account already in use, 401 Authorization required
- fixed bitrate icon overflow bug
- fix html markup errors

#### v0.9.156
- fixed bug of random change of playback location when playing files from SD card

#### v0.9.155
- added bitrate badget for displays ST7789, ST7796, ILI9488, ILI9486, ILI9341, ILI9225 and ST7735(BLACKTAB)
  (disable: #define BITRATE_FULL false)
- fixed a bug with garbage appearing on display ILI9225

#### v0.9.143
- fixed NOKIA5110 display invert/off bug

#### v0.9.142
- fixed a bug with smartstart playback when the power is turned off

#### v0.9.141
- fixed error reconnecting to WiFi when connection is lost
- ADDED a compilation error when choosing a board other than "ESP32 Dev Module" or "ESP32 Wrover Module"

#### v0.9.130
- fixed crash in configurations with NOKIA5110 displays
- fixed bug with displaying buffer indicator when switching audioinfo
- fixed bug with displaying the buffer indicator when the connection is lost
- fixed bug of MUTE_PIN failure when connection is lost
- other minor fixes

#### v0.9.122
- fixed a bug in the operation of SSD1305 displays
- fixed bug in operation of LCD1602/2004 displays
- fixed errors in Serial Monitor output

#### v0.9.110
- optimization and bug fixes (display, player, netserver, telnet. mqtt)

#### v0.9.084
- monospace fonts for clock on TFT displays. Fonts can be restored to their original form by adding the ```#define CLOCKFONT_MONO false``` parameter to the myoptions.h file
- new parameter ```#define COLOR_CLOCK_BG R,G,B``` - color of inactive clock segments

#### v0.9.058
- added support for ST7920 128x64 2.6' OLED display https://aliexpress.com/item/32699482638.html

#### v0.9.045
- added support for SSD1322 256x64 2.8' OLED display https://aliexpress.com/item/1005003480981568.html

#### v0.9.022
- optimization of the display of the list of stations
- now the playlist size can be changed with one parameter in the yoRadio/src/displays/conf/display<span>_XXXX_</span>conf.h file --> _const ScrollConfig playlistConf_ param #3
- fixed fonts for ILI9225 display
- fixes in Nextion displays
- bug fixes (including BUFFER FILLED IN 403 MS)

#### v0.9.001
- fixed compilation error netserver.cpp:63:28 for some configurations

#### v0.9.000
- added WEB Board Uploader. ESP32 Filesystem Uploader is no longer needed, the initial setup can be done in the browser. (see [wiki](https://github.com/e2002/yoradio/wiki/WEB-Board-Uploader) for more info)
- fixed error getting weather for some locations

#### v0.8.990
- fixed error displaying access point credentials when DSP_INVERT_TITLE is false
- fixed compilation error for OLED displays when DSP_INVERT_TITLE is false

#### v0.8.988
- **DSP_INVERT_TITLE** now works for all displays when assigned
   ```#define DSP_INVERT_TITLE false```
   the display title takes on a "classic" look (light letters on a dark background)
- advanced weather display - wind direction and strength, feels like
- sea level pressure changed to surface pressure
- added degree icon [\*ps]
- displaying the WiFi signal level in graphical form [\*ps]

[\*ps] - **glcdfont.c** from the **Adafruit_GFX_Library** library has been changed to add new icons, so for the correct display of all this, you need to replace the specified file in the Adafruit_GFX library with the file from the yoRadio/fonts/ folder

#### v0.8.962
- fixed reboot error after sending media from Home Assistant
- fixed bug when playing media from Home Assistant for VS1053
- fix grammar errors

#### v0.8.950
- added support for remote media playback from Home Assistant (Local Media, Radio Browser, TTS)

#### v0.8.933 (homeassistant component)
- HA component >> fixed bugs of getting and generating a playlist

#### v0.8.933
- added support for ILI9488 display
- added support for ILI9486 display in testing mode

#### v0.8.920
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!** \
**Please backup playlist.csv and wifi.csv before updating.**
- fixed bug with displaying horizontal scroll in playlist
- fixed compilation error with IR_PIN=255
- libraries async-mqtt-client, AsyncTCP, ESPAsyncWebServer moved to the project
- new parameter #define XTASK_MEM_SIZE - buffer size for AsyncTCP task (4096 by default)

#### v0.8.901
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!** \
**Please backup playlist.csv and wifi.csv before updating.**
- added SD Card support (more info in [connection table](#connection-tables) and [examples/myoptions.h](https://github.com/e2002/yoradio/blob/main/examples/myoptions.h))
- added MODE button to switch SD/WEB modes (more info in [Controls.md](https://github.com/e2002/yoradio/blob/main/Controls.md))
- asterisk on the remote control now switches SD/WEB modes
- double click BTN_CENTER and ENC_BTNB now toggles SD/WEB modes
- bug fixes

#### v0.8.173
- bootlog added
- fixed work of start/stop button in configurations with DSP_DUMMY

#### v0.8.138
- fixed unclosed comment in examples/myoptions.h

#### v0.8.137
- fixed compilation error without encoder

#### v0.8.135
- added numeric IR remote buttons in configurations with DSP_DUMMY
- fixed navigation bug in playlist with more than 255 stations
- fixed work of encoders in configurations with DSP_DUMMY
- fixed missing volume value bug when switching to volume control dialog
- LED_BUILTIN is now 255 by default (off)

#### v0.8.112
- fixed compilation error with BOOMBOX_STYLE parameter
- fixes in default configuration for GC9A01A display

#### v0.8.100
- added support for GC9A01A display https://aliexpress.com/item/1005004069703494.html?sku_id=12000029869654615

#### v0.8.089
- increased length of SSID string to 30 characters (requires full update + ESP32 Data Upload)
- fixed artifacts when adjusting the volume on OLED displays
- fixed bug with missing current station in playlist on OLED displays
- new parameter DSP_INVERT_TITLE - invert colors in station name for OLED displays (more details in examples/myoptions.h)

#### v0.8.03b
- added support for ST7796 display
- added support for capacitive touch GT911
- HSPI bus support added - DSP_HSPI, VS_HSPI, TS_HSPI options More details in examples/myoptions.h
- changed the method of connecting the touchscreen in myoptions.h Now instead of specifying TS_CS, you must specify TS_MODEL (by default TS_MODEL_UNDEFINED) More details in examples/myoptions.h
- new parameters TS_SDA, TS_SCL, TS_INT, TS_RST for GT911 touchscreen
- new parameters LIGHT_SENSOR and AUTOBACKLIGHT - to automatically adjust the brightness of the display. More details in examples/myoptions.h
- new parameter LED_INVERT (true/false) - to invert the behavior of the built-in LED
- fixed bug with extra sign } in humidity value

#### v0.8.02b
- fixed artifacts when displaying the volume level
- changes in mytheme.h . Added colors COLOR_PL_CURRENT, COLOR_PL_CURRENT_BG, COLOR_PL_CURRENT_FILL. Details in [examples/mytheme.h](examples/mytheme.h)

#### v0.8.01b
- fix INITR_MINI160x80 compiling error
- fix ENC_INTERNALPULLUP description in examples/myoptions.h

#### v0.8.00b
- rewritten the display engine
- added the ability to position widgets on the display using configuration files. More info in yoRadio/src/displays/conf/ and here https://github.com/e2002/yoradio/wiki/Widgets
- the VU_PARAMS3 parameter is deprecated. VUmeter configuration is done through yoRadio/src/displays/conf/ configs
- added bitrate display on displays
- added the ability to display the weather on all displays except LCD1602
- examples of plug-ins related to displaying information on the display are outdated and no longer work. The examples have been removed from the examples/plugins folder.
- the structure of the project files has been changed so that I don’t know what.
- localization of information displayed on the display (rus, en). Option L10N_LANGUAGE (EN by default. see examples/myoptions.h for details)
- changes in mytheme.h . Added colors COLOR_STATION_BG, COLOR_STATION_FILL, COLOR_BITRATE
- optimization, refactoring
- bugs fixes
- bugs adding
- probably something else that I forgot .__.

#### v0.7.540
- fixed compilation error when using NEXTION display with DUMMY display

#### v0.7.534
- added control via uart (see [list of commands](https://github.com/e2002/yoradio/wiki/List-of-available-commands-(UART-telnet-GET-POST))). The uart and telnet commands are the same.
- added additional commands
- added control via GET/POST (see [list of commands](https://github.com/e2002/yoradio/wiki/List-of-available-commands-(UART-telnet-GET-POST)))
- fixed clock operation when configured with DSP_DUMMY
- fixed RSSI display in web interface when configured with DSP_DUMMY
- added brightness control/on/off nextion displays from the web interface
- new parameter WAKE_PIN (to wake up esp after sleep command earlier than given time (see examples/myoptions.h and list of commands)
- minor memory optimization

#### v0.7.490
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!** \
**Please backup playlist.csv and wifi.csv before updating.**
- fixed playlist break down when saving it
- fixed bug with cropped song titles on single line displays (GC9106, ST7735mini, N5110 etc.)
- netserver - optimization and refactoring
- web interface optimization
- the AUDIOBUFFER_MULTIPLIER parameter is deprecated. New parameter AUDIOBUFFER_MULTIPLIER2. If everything works fine, then it is better not to touch it.
- new setting VS_PATCH_ENABLE (see PS)
- fixing other bugs

_**PS:** A bug was found with the lack of sound on some (not all) green VS1053 boards.
If there is no sound, you need to assign in myoptions_
```
#define VS_PATCH_ENABLE false
```
_On red boards and normally working green boards, nothing else needs to be done._

#### v0.7.414
- fixed non latin long titles of songs error

#### v0.7.402
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!** \
**Please backup playlist.csv and wifi.csv before updating.**
- added the ability to themize color displays. Details in [examples/mytheme.h](examples/mytheme.h)
- in this connection, examples of plugins displayhandlers.ino and rssibitrate.ino have been updated
- parameter VU_PARAMS2 is deprecated. New parameter - VU_PARAMS3. Details in [yoRadio/display_vu.h](yoRadio/display_vu.h)
- added deepsleep capability for LCD_I2C and OLED displays
- in this connection, a full update with Sketch data upload is required
- in this connection, example of plugin deepsleep.ino (examples/plugins/deepsleep.ino) have been updated
- some bug fixes

#### v0.7.355
- updating libraries ESP32-audioI2S and ESP32-vs1053_ext to the latest version
- optimization of the web interface during playback
- fixed one js bug. a [full update](#update-over-web-interface) with Sketch data upload is desirable
- plugin example for esp deep sleep when playback is stopped (examples/plugins/deepsleep.ino)

#### v0.7.330
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!** \
**Please backup playlist.csv and wifi.csv before updating.**
- added the ability to configure parameters through the [web interface](images/settings.png)
- new parameter BRIGHTNESS_PIN - pin for adjusting the brightness of the display. Details in [examples/myoptions.h](examples/myoptions.h#L105)
- the weather plugin is integrated into the code, the settings are made through the web interface

_**PS:** Due to the change in the storage location of settings in the ESP memory, settings such as:_ \
**smartstart, audioinfo, time zone, IR remote, last volume level, last played station, equalizer** \
_will have to be configured again through the web interface. Please understand and forgive._

#### v0.7.017
- fix initialization of some vs1053b green boards
- fix VU initialization on vs1053b boards

#### v0.7.010
- fixed choppy of sound when volume adjustment
- fixed initialisation of Nextion displays

#### v0.7.000
- added support for Nextion displays ([more info](nextion/README.md))
- fixed work of VU Meter
- fixed time lag when adjusting the volume / selecting a station
- optimization of work with the DSP_DUMMY option
- some bug fixes

#### v0.6.530
- adding VU meter for displays ST7735 160x80, GC9106 160x80, ILI9225 220x176, ST7789 240x240
- TFT_22_ILI9225 library is integrated into the project

#### v0.6.494
- adding VU meter for displays ST7735 160x128, ST7735 128x128, ILI9341 320x240, ST7789 320x240 \
  option ENABLE_VU_METER (see [myoptions.h](examples/myoptions.h) for example) \
  **!!! Important !!!** \
  if you enable this feature on the esp32 wroom, due to lack of memory, you must modify the file Arduino/libraries/AsyncTCP/src/AsyncTCP.cpp \
  **replace the line 221** \
  _xTaskCreateUniversal(_async_service_task, "async_tcp", 8192 * 2, NULL, 3, &_async_service_task_handle, CONFIG_ASYNC_TCP_RUNNING_CORE);_ \
  **with** \
  _xTaskCreateUniversal(_async_service_task, "async_tcp", 8192 / 2, NULL, 3, &_async_service_task_handle, CONFIG_ASYNC_TCP_RUNNING_CORE);_

#### v0.6.450
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!**
- adding an IR remote control has been moved to the web-interface (more info in [Controls.md](Controls.md#ir-receiver))
- fixed broken internal DAC on esp32 core 2.0.3 and highest

#### v0.6.400
- fixed compilation errors with esp32 core 2.0.4

#### v0.6.380
**!!! a [full update](#update-over-web-interface) with Sketch data upload is required. After updating please press CTRL+F5 in browser !!!**
- fixed a bug when saving a playlist with special characters in the name and url
- fixed a bug when saving wifi settings with special characters in the name and password
- fixed css bugs

#### v0.6.357
- remove ZERO WIDTH NO-BREAK SPACE (BOM, ZWNBSP) from stream title

#### v0.6.355
- added support for ST7789 1.3' 240x240 SPI displays \
  _!!! Important !!! This display requires further development when used in conjunction with the VS1053 module._ \
  See this link for details https://www.instructables.com/Adding-CS-Pin-to-13-LCD/

#### v0.6.348
- fixed display bugs in the rssibitrate plugin
- fixed some compilation warnings

#### v0.6.345
- fix compilation error in rssibitrate plugin with ILI9225 display

#### v0.6.344
- fixed SPI-display bugs when used with VS1053B module
- added example plugin for analog volume control ([examples/plugins/analogvolume.ino](examples/plugins/analogvolume.ino))
- added example plugin for backlight control depending on playback ([examples/plugins/backlightcontrols.ino](examples/plugins/backlightcontrols.ino))
- added example plugin for replace a RSSI to bitrate information alternately ([examples/plugins/rssibitrate.ino](examples/plugins/rssibitrate.ino))

#### v0.6.320
- fixed ILI9225 display bug when used with VS1053B module
- fixed ILI9225 plugin support

#### v0.6.313
- added support for ILI9225 220x176 SPI displays
- added support for I2S internal DAC, option I2S_INTERNAL (see [myoptions.h](examples/myoptions.h#L111) for example) \
 _(this option worked only with esp32 core version==2.0.0)_
- new option SOFT_AP_REBOOT_DELAY (see [myoptions.h](examples/myoptions.h) for example)
- fixed MQTT connection when WiFi reconnected
- fixed date display for ILI9341 displays
- fixed garbage on volume control with displays ILI9341

#### v0.6.290
- fixed interface blocking error when synchronizing time
- time sync optimization
- new option **SNTP_SERVER**, to set your custom server for synchronization (see [myoptions.h](examples/myoptions.h) for example)

#### v0.6.278
- added support for LCD2004 displays
- added support for SSD1305/SSD1309 I2C displays
- fixed rotation of SH1106 display

#### v0.6.263
- fixed encoder internal pullup

#### v0.6.262
- change encoder library to [ai-esp32-rotary-encoder](https://github.com/igorantolic/ai-esp32-rotary-encoder) (injected to project)
- added new option VOL_ACCELERATION - volume adjustment acceleration by encoder (see [myoptions.h](examples/myoptions.h) for example)
- fixed connection error with http-stations on esp32-core v2.0.3
- fixed css errors (a [full update](#update-over-web-interface) is required)

#### v0.6.250
- added update via web-interface \
 **Attention! Full firmware with chip re-partitioning is required!** see [board setup example](#quick-start)
- fixed choppy when switching stations via Home Assistant

#### v0.6.220
- new option PLAYER_FORCE_MONO (with i2S DAC only)
- change default scroll speed in DSP_NOKIA5110
- improved reconnect to WiFi on connection loss

#### v0.6.210
- fixed choppy playback on DSP_ST7735 displays used with VS1053
- new option PL_WITH_NUMBERS (show the number of station in the playlist)
- fixed compiling error with DSP_DUMMY option
- correction of displays GC9106 and SSD1305

#### v0.6.202
- fixed errors in the operation of the second encoder
- rewrote [plugin example](examples/plugins/displayhandlers.ino)
- fixed compilation errors on macOS #2

#### v0.6.200
- please backup your playlist and wifi settings before updating (export)
- accelerated displays up to ~30fps (everything except LCD)
- corrections/additions in the WEB interface (a [full update](#update) is required)
- rewrote [plugin example](examples/plugins/displayhandlers.ino)
- fixed compilation errors on macOS
- changed the logic of the second encoder (switching to the volume control mode by double click)
- optimization, bug fixes
- probably some other things that I forgot about %)

#### v0.6.120
- added support for GC9106 160x80 SPI displays
- fixed compiling error with DSP_DUMMY option
- fixed compiling error with DSP_1602I2C / DSP_1602 option

#### v0.6.110
- the logic of division by cores has been changed
- fixed choppy playback (again)
- improvements in the stability of the web interface
- increased smoothness of the encoder
- bug fixes
- bug fixes

#### v0.6.012
- fixed choppy playback

#### v0.6.010
- added displays [SSD1327](https://aliexpress.com/item/1005001414175498.html), [ILI9341](https://aliexpress.com/item/33048191074.html), [SSD1305/SSD1309](https://aliexpress.com/item/32950307344.html), [SH1107](https://aliexpress.com/item/4000551696674.html), [1602](https://aliexpress.com/item/32685016568.html)
- added [touchscreen](Controls.md#touchscreen) support
- tasks are divided into cores, now the sound is not interrupted when selecting stations / volume
- increased speed of some displays
- optimization of algorithms, bugs fixes

#### v0.5.070
- added something similar to plugins

#### v0.5.035
- added two buttons BTN_UP, BTN_DOWN
- added the pins for the second encoder ENC2_BTNL, ENC2_BTNB, ENC2_BTNR
- fixed display of playlist with SSD1306 configuration
- improvements in the displays work
- bugs fixes, some improvements

#### v0.5.020
- added support for SSD1306 128x32 I2C displays

#### v0.5.010
- added support for ST7789 320x240 SPI displays
- added support for SH1106 I2C displays
- added support for 1602 16x2 I2C displays
- a little modified control logic
- added buttons long press feature
- small changes in options.h, check the correctness of your myoptions.h file
- bugs fixes

#### v0.4.323
- fixed bug [Equalizer not come visible after go to playlist](https://github.com/e2002/yoradio/issues/1) \
 (a [full update](#update) is required)
- fixed playlist filling error in home assistant component

#### v0.4.322
- fixed garbage in MQTT payload

#### v0.4.320
- MQTT support

<img src="images/mqtt.jpg" width="680" height="110">

#### v0.4.315
- added support for digital buttons for the IR control \
(num keys - enter number of station, ok - play, hash - cancel)
- added buttons for exporting settings from the web interface
- added MUTE_PIN to be able to control the audio output
- fixed js/html bugs (a [full update](#update) is required)

#### v0.4.298
- fixed playlist scrollbar in Chrome (a [full update](#update) is required)

#### v0.4.297
- fix _"Could not decode a text frame as UTF-8"_ websocket error _//Thanks for [Verholazila](https://4pda.to/forum/index.php?s=&showtopic=1010378&view=findpost&p=113551446)_
- fix display of non-latin characters in the web interface
- fix css in Chrome (a [full update](#update) is required)

#### v0.4.293
- IR repeat fix

#### v0.4.292
- added support for IR control
- new options in options.h (ENC_INTERNALPULLUP, ENC_HALFQUARD, BTN_INTERNALPULLUP, VOL_STEP) _//Thanks for [Buska1968](https://4pda.to/forum/index.php?s=&showtopic=1010378&view=findpost&p=113385448)_
- compilation error for module SSD1306 with arduino-esp32 version newest than 2.0.0
- fix compiler warnings in options.h
- fix some compiler warnings

#### v0.4.260
- added control of balance and equalizer for VS1053
- **TFT_ROTATE** and st7735 **DTYPE** moved to myoptions.h

#### v0.4.251
- fixed compilation error bug when using VS1053 together with ST7735

#### v0.4.249
- fix VS1003/1053 reseting
- fix css in Firefox
- fix font in NOKIA5110 display

#### v0.4.248
- added support for VS1053 module _in testing mode_

#### v0.4.210
- added timezone config by telnet
- fix telnet output
- some separation apples and oranges

#### v0.4.199
- excluded required installation of all libraries for displays

#### v0.4.197
- added support for Nokia 5110 SPI displays
- some bugs fixes

#### v0.4.183
- ovol reading bug

#### v0.4.182
- display connection algorithm changed
- added support for myoptions.h file for custom settings

#### v0.4.180
- vol steps 0..256 (in ESP32-audioI2S)

#### v0.4.177
- added support for SSD1306 I2C displays
- fixed broken buttons.
