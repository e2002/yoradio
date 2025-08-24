#include "options.h"
#include <stdarg.h>
#include "config.h"
#include "player.h"
#include "network.h"
#include "telnet.h"
//#include "esp_heap_caps.h"

Telnet telnet;

bool Telnet::_isIPSet(IPAddress ip) {
  return strcmp(config.ipToStr(ip), "0.0.0.0") == 0;
}

bool Telnet::begin(bool quiet) {
  if(network.status==SDREADY) {
    BOOTLOG("Ready in SD Mode!");
    BOOTLOG("------------------------------------------------");
    Serial.println("##[BOOT]#");
    return true;
  }
  if(!quiet) Serial.print("##[BOOT]#\ttelnet.begin\t");
  if (WiFi.status() == WL_CONNECTED || _isIPSet(WiFi.softAPIP())) {
    toggle();
    if(!quiet){
      Serial.println("done");
      Serial.println("##[BOOT]#");
      BOOTLOG("Ready! Go to http:/%s/ to configure", config.ipToStr(WiFi.localIP()));
      BOOTLOG("------------------------------------------------");
      Serial.println("##[BOOT]#");
    }
    return true;
  } else {
    return false;
  }
}

void Telnet::start() {
  server.begin();
  server.setNoDelay(true);
}

void Telnet::stop() {
  server.stop();
}

void Telnet::toggle() {
  if(config.store.telnet) { start(); }else{ stop(); }
}

void Telnet::emptyClientStream(WiFiClient client) {
  client.flush();
  delay(50);
  while (client.available()) {
    client.read();
  }
}

void Telnet::cleanupClients() {
  for (int i = 0; i < MAX_TLN_CLIENTS; i++) {
    if (!clients[i].connected()) {
      if (clients[i]) {
        Serial.printf("Client [%d] is %s\n", i, clients[i].connected() ? "connected" : "disconnected");
        clients[i].stop();
      }
    }
  }
}

void Telnet::handleSerial(){
  if(Serial.available()){
    String request = Serial.readStringUntil('\n'); request.trim();
    on_input(request.c_str(), 100);
  }
}

void Telnet::loop() {
  if(network.status==SDREADY || network.status!=CONNECTED) {
    handleSerial();
    return;
  }
  uint8_t i;
  if(config.store.telnet)
    if (WiFi.status() == WL_CONNECTED) {
      if (server.hasClient()) {
        for (i = 0; i < MAX_TLN_CLIENTS; i++) {
          if (!clients[i] || !clients[i].connected()) {
            if (clients[i]) {
              clients[i].stop();
            }
            clients[i] = server.available();
            if (!clients[i]) Serial.println("available broken");
            on_connect(config.ipToStr(clients[i].remoteIP()), i);
            clients[i].setNoDelay(true);
            emptyClientStream(clients[i]);
            break;
          }
        }
        if (i >= MAX_TLN_CLIENTS) {
          server.available().stop();
        }
      }
      for (i = 0; i < MAX_TLN_CLIENTS; i++) {
        if (clients[i] && clients[i].connected() && clients[i].available()) {
          String inputstr = clients[i].readStringUntil('\n');
          inputstr.trim();
          on_input(inputstr.c_str(), i);
        }
      }
    } else {
      for (i = 0; i < MAX_TLN_CLIENTS; i++) {
        if (clients[i]) {
          clients[i].stop();
        }
      }
      delay(1000);
    }
  handleSerial();
}

void Telnet::print(const char *buf) {
  for (int id = 0; id < MAX_TLN_CLIENTS; id++) {
    if (clients[id] && clients[id].connected()) {
      print(id, buf);
    }
  }
  Serial.print(buf);
}

void Telnet::print(uint8_t id, const char *buf) {
  if (clients[id] && clients[id].connected()) {
    clients[id].print(buf);
  }
}

void Telnet::printf(const char *format, ...) {
  va_list args;
  va_start (args, format );
  vsnprintf(cmBuf, sizeof(cmBuf), format, args);
  va_end (args);
  for (int id = 0; id < MAX_TLN_CLIENTS; id++) {
    if (clients[id] && clients[id].connected()) {
      clients[id].print(cmBuf);
    }
  }
  if (strcmp(cmBuf, "> ") == 0) return;
  //if(strstr(buf,"\n> ")==NULL) Serial.print(buf);
  char *nl = strstr(cmBuf, "\n> ");
  if (nl != NULL) { cmBuf[nl-cmBuf+1] = '\0'; }
  Serial.print(cmBuf);
}

void Telnet::printf(uint8_t id, const char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(cmBuf, sizeof(cmBuf), format, argptr);
  va_end(argptr);
  if(id>MAX_TLN_CLIENTS){
    Serial.print(cmBuf);
    return;
  }
  if (clients[id] && clients[id].connected()) {
    clients[id].print(cmBuf);
  }
}

void Telnet::on_connect(const char* str, uint8_t clientId) {
  Serial.printf("Telnet: [%d] %s connected\n", clientId, str);
  print(clientId, "\nWelcome to ёRadio!\n(Use ^] + q  to disconnect.)\n> ");
}

void Telnet::info() {
  telnet.printf("##CLI.INFO#\n");
  strftime(config.tmpBuf, sizeof(config.tmpBuf), "%Y-%m-%dT%H:%M:%S+03:00", &network.timeinfo);
  telnet.printf("##SYS.DATE#: %s\n", config.tmpBuf); //TODO timezone offset
  telnet.printf("##CLI.NAMESET#: %d %s\n", config.lastStation(), config.station.name);
  if (player.status() == PLAYING) {
    telnet.printf("##CLI.META#: %s\n",  config.station.title);
  }
  telnet.printf("##CLI.VOL#: %d\n", config.store.volume);
  if (player.status() == PLAYING) {
    telnet.printf("##CLI.PLAYING#\n");
  } else {
    telnet.printf("##CLI.STOPPED#\n");
  }
  telnet.printf("> ");
}

void Telnet::printHeapFragmentationInfo(uint8_t id){
  size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
  float fragmentation = 100.0 * (1.0 - ((float)largestBlock / (float)freeHeap));
  printf(id, "\n*************************************\n");
  printf(id, "* Free heap: %u bytes\n", freeHeap);
  printf(id, "* Largest free block: %u bytes\n", largestBlock);
  printf(id, "* Fragmentation: %.2f%%\n", fragmentation);
  printf(id, "*************************************\n\n");
}
void Telnet::on_input(const char* str, uint8_t clientId) {
  if (strlen(str) == 0) return;
  if(network.status == CONNECTED){
    if (strcmp(str, "cli.prev") == 0 || strcmp(str, "prev") == 0) {
      player.prev();
      return;
    }
    if (strcmp(str, "cli.next") == 0 || strcmp(str, "next") == 0) {
      player.next();
      return;
    }
    if (strcmp(str, "cli.toggle") == 0 || strcmp(str, "toggle") == 0) {
      player.toggle();
      return;
    }
    if (strcmp(str, "cli.stop") == 0 || strcmp(str, "stop") == 0) {
      player.sendCommand({PR_STOP, 0});
      //info();
      return;
    }
    if (strcmp(str, "cli.start") == 0 || strcmp(str, "start") == 0 || strcmp(str, "cli.play") == 0 || strcmp(str, "play") == 0) {
      player.sendCommand({PR_PLAY, config.lastStation()});
      return;
    }
    if (strcmp(str, "cli.vol") == 0 || strcmp(str, "vol") == 0) {
      printf(clientId, "##CLI.VOL#: %d\n> ", config.store.volume);
      return;
    }
    if (strcmp(str, "cli.vol-") == 0 || strcmp(str, "vol-") == 0) {
      player.stepVol(false);
      return;
    }
    if (strcmp(str, "cli.vol+") == 0 || strcmp(str, "vol+") == 0) {
      player.stepVol(true);
      return;
    }
    if (strcmp(str, "sys.date") == 0 || strcmp(str, "date") == 0 || strcmp(str, "time") == 0) {
      network.requestTimeSync(true, clientId > MAX_TLN_CLIENTS?clientId:0);
      return;
    }
    int volume;
    if (sscanf(str, "vol(%d)", &volume) == 1 || sscanf(str, "cli.vol(\"%d\")", &volume) == 1 || sscanf(str, "vol %d", &volume) == 1) {
      if (volume < 0) volume = 0;
      if (volume > 254) volume = 254;
      player.setVol(volume);
      return;
    }
    if (strcmp(str, "cli.audioinfo") == 0 || strcmp(str, "audioinfo") == 0) {
      printf(clientId, "##CLI.AUDIOINFO#: %d\n> ", config.store.audioinfo > 0);
      return;
    }
    int ainfo;
    if (sscanf(str, "audioinfo(%d)", &ainfo) == 1 || sscanf(str, "cli.audioinfo(\"%d\")", &ainfo) == 1 || sscanf(str, "audioinfo %d", &ainfo) == 1) {
      config.saveValue(&config.store.audioinfo, ainfo > 0);
      printf(clientId, "new audioinfo value is: %d\n> ", config.store.audioinfo);
      return;
    }
    if (strcmp(str, "cli.smartstart") == 0 || strcmp(str, "smartstart") == 0) {
      printf(clientId, "##CLI.SMARTSTART#: %d\n> ", config.store.smartstart);
      return;
    }
    int sstart;
    if (sscanf(str, "smartstart(%d)", &sstart) == 1 || sscanf(str, "cli.smartstart(\"%d\")", &sstart) == 1 || sscanf(str, "smartstart %d", &sstart) == 1) {
      config.saveValue(&config.store.smartstart, static_cast<uint8_t>(sstart));
      printf(clientId, "new smartstart value is: %d\n> ", config.store.smartstart);
      return;
    }
    if (strcmp(str, "cli.list") == 0 || strcmp(str, "list") == 0) {
      printf(clientId, "#CLI.LIST#\n");
      File file = SPIFFS.open(PLAYLIST_PATH, "r");
      if (!file || file.isDirectory()) {
        return;
      }
      int sOvol;
      uint8_t c = 1;
      while (file.available()) {
        if (config.parseCSV(file.readStringUntil('\n').c_str(), config.tmpBuf, config.tmpBuf2, sOvol)) {
          printf(clientId, "#CLI.LISTNUM#: %*d: %s, %s\n", 3, c, config.tmpBuf, config.tmpBuf2);
          c++;
        }
      }
      printf(clientId, "##CLI.LIST#\n");
      printf(clientId, "> ");
      return;
    }
    if (strcmp(str, "cli.info") == 0 || strcmp(str, "info") == 0) {
      printf(clientId, "##CLI.INFO#\n");
      strftime(config.tmpBuf, sizeof(config.tmpBuf), "%Y-%m-%dT%H:%M:%S", &network.timeinfo);
      if (config.store.tzHour < 0) {
        printf(clientId, "##SYS.DATE#: %s%03d:%02d\n", config.tmpBuf, config.store.tzHour, config.store.tzMin);
      } else {
        printf(clientId, "##SYS.DATE#: %s+%02d:%02d\n", config.tmpBuf, config.store.tzHour, config.store.tzMin);
      }
      printf(clientId, "##CLI.NAMESET#: %d %s\n", config.lastStation(), config.station.name);
      if (player.status() == PLAYING) {
        printf(clientId, "##CLI.META#: %s\n", config.station.title);
      }
      printf(clientId, "##CLI.VOL#: %d\n", config.store.volume);
      if (player.status() == PLAYING) {
        printf(clientId, "##CLI.PLAYING#\n");
      } else {
        printf(clientId, "##CLI.STOPPED#\n");
      }
      printf(clientId, "> ");
      return;
    }
    int sb;
    if (sscanf(str, "play(%d)", &sb) == 1 || sscanf(str, "cli.play(\"%d\")", &sb) == 1 || sscanf(str, "play %d", &sb) == 1 ) {
      if (sb < 1) sb = 1;
      uint16_t cs = config.playlistLength();
      if (sb >= cs) sb = cs;
      player.sendCommand({PR_PLAY, (uint16_t)sb});
      return;
    }
    #ifdef USE_SD
    int mm;
    if (sscanf(str, "mode %d", &mm) == 1 ) {
      if (mm > 2) mm = 0;
      if(mm==2)
        config.changeMode();
      else
        config.changeMode(mm);
      return;
    }
    #endif
    if (strcmp(str, "sys.tzo") == 0 || strcmp(str, "tzo") == 0) {
      printf(clientId, "##SYS.TZO#: %d:%d\n> ", config.store.tzHour, config.store.tzMin);
      return;
    }
    //int16_t tzh, tzm;
    int tzh, tzm;
    if (sscanf(str, "tzo(%d:%d)", &tzh, &tzm) == 2 || sscanf(str, "sys.tzo(\"%d:%d\")", &tzh, &tzm) == 2 || sscanf(str, "tzo %d:%d", &tzh, &tzm) == 2) {
      if (tzh < -12) tzh = -12;
      if (tzh > 14) tzh = 14;
      if (tzm < 0) tzm = 0;
      if (tzm > 59) tzm = 59;
      config.setTimezone((int8_t)tzh, (int8_t)tzm);
      if(tzh<0){
        printf(clientId, "new timezone offset: %03d:%02d\n", config.store.tzHour, config.store.tzMin);
      }else{
        printf(clientId, "new timezone offset: %02d:%02d\n", config.store.tzHour, config.store.tzMin);
      }
      network.requestTimeSync(true);
      return;
    }
    if (sscanf(str, "tzo(%d)", &tzh) == 1 || sscanf(str, "sys.tzo(\"%d\")", &tzh) == 1 || sscanf(str, "tzo %d", &tzh) == 1) {
      if (tzh < -12) tzh = -12;
      if (tzh > 14) tzh = 14;
      config.setTimezone((int8_t)tzh, 0);
      if(tzh<0){
        printf(clientId, "new timezone offset: %03d:%02d\n", config.store.tzHour, config.store.tzMin);
      }else{
        printf(clientId, "new timezone offset: %02d:%02d\n", config.store.tzHour, config.store.tzMin);
      }
      network.requestTimeSync(true);
      return;
    }
    if (sscanf(str, "dspon(%d)", &tzh) == 1 || sscanf(str, "cli.dspon(\"%d\")", &tzh) == 1 || sscanf(str, "dspon %d", &tzh) == 1) {
      config.setDspOn(tzh!=0);
      return;
    }
    if (sscanf(str, "dim(%d)", &tzh) == 1 || sscanf(str, "cli.dim(\"%d\")", &tzh) == 1 || sscanf(str, "dim %d", &tzh) == 1) {
      if (tzh < 0) tzh = 0;
      if (tzh > 100) tzh = 100;
      config.store.brightness = (uint8_t)tzh;
      config.setBrightness(true);
      return;
    }
    if (sscanf(str, "sleep(%d,%d)", &tzh, &tzm) == 2 || sscanf(str, "cli.sleep(\"%d\",\"%d\")", &tzh, &tzm) == 2 || sscanf(str, "sleep %d %d", &tzh, &tzm) == 2) {
      if(tzh>0 && tzm>0) {
        printf(clientId, "sleep for %d minutes after %d minutes ...\n> ", tzh, tzm);
        config.sleepForAfter(tzh, tzm);
      }else{
        printf(clientId, "##CMD_ERROR#\tunknown command <%s>\n> ", str);
      }
      return;
    }
    if (sscanf(str, "sleep(%d)", &tzh) == 1 || sscanf(str, "cli.sleep(\"%d\")", &tzh) == 1 || sscanf(str, "sleep %d", &tzh) == 1) {
      if(tzh>0) {
        printf(clientId, "sleep for %d minutes ...\n> ", tzh);
        config.sleepForAfter(tzh);
      }else{
        printf(clientId, "##CMD_ERROR#\tunknown command <%s>\n> ", str);
      }
      return;
    }
  }
  if (strcmp(str, "sys.version") == 0 || strcmp(str, "version") == 0) {
    printf(clientId, "##SYS.VERSION#: %s\n> ", YOVERSION);
    return;
  }
  if (strcmp(str, "sys.boot") == 0 || strcmp(str, "boot") == 0 || strcmp(str, "reboot") == 0) {
    ESP.restart();
    return;
  }
  if (strcmp(str, "sys.reset") == 0 || strcmp(str, "reset") == 0) {
    config.reset();
    return;
  }
  if (strcmp(str, "wifi.list") == 0 || strcmp(str, "wifi") == 0) {
    printf(clientId, "#WIFI.SCAN#\n");
    int n = WiFi.scanNetworks();
    if (n == 0) {
        printf(clientId, "no networks found\n");
    } else {
      for (int i = 0; i < n; ++i) {
        printf(clientId, "%d", i + 1);
        printf(clientId, ": ");
        printf(clientId, "%s", WiFi.SSID(i));
        printf(clientId, " (");
        printf(clientId, "%d", WiFi.RSSI(i));
        printf(clientId, ")");
        printf(clientId, (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
        printf(clientId, "\n");
        delay(10);
      }
    }
    printf(clientId, "#WIFI.SCAN#\n> ");
    return;
  }
  if (strcmp(str, "wifi.con") == 0 || strcmp(str, "conn") == 0) {
    printf(clientId, "#WIFI.CON#\n");
    File file = SPIFFS.open(SSIDS_PATH, "r");
    if (file && !file.isDirectory()) {
      uint8_t c = 1;
      while (file.available()) {
        if (config.parseSsid(file.readStringUntil('\n').c_str(), config.tmpBuf, config.tmpBuf2)) {
          printf(clientId, "%d: %s, %s\n", c, config.tmpBuf, config.tmpBuf2);
          c++;
        }
      }
    }
    printf(clientId, "##WIFI.CON#\n> ");
    return;
  }
  if (strcmp(str, "wifi.station") == 0 || strcmp(str, "station") == 0 || strcmp(str, "ssid") == 0) {
    printf(clientId, "#WIFI.STATION#\n");
    File file = SPIFFS.open(SSIDS_PATH, "r");
    if (file && !file.isDirectory()) {
      uint8_t c = 1;
      while (file.available()) {
        if (config.parseSsid(file.readStringUntil('\n').c_str(), config.tmpBuf, config.tmpBuf2)) {
          if(c==config.store.lastSSID) printf(clientId, "%d: %s, %s\n", c, config.tmpBuf, config.tmpBuf2);
          c++;
        }
      }
    }
    printf(clientId, "##WIFI.STATION#\n> ");
    return;
  }
  if (sscanf(str, "wifi.con(\"%[^\"]\",\"%[^\"]\")", config.tmpBuf, config.tmpBuf2) == 2 || sscanf(str, "wifi.con(%[^,],%[^)])", config.tmpBuf, config.tmpBuf2) == 2 || sscanf(str, "wifi.con(%[^ ] %[^)])", config.tmpBuf, config.tmpBuf2) == 2 || sscanf(str, "wifi %[^ ] %s", config.tmpBuf, config.tmpBuf2) == 2) {
    snprintf(cmBuf, sizeof(cmBuf), "New SSID: \"%s\" with PASS: \"%s\" for next boot\n> ", config.tmpBuf, config.tmpBuf2);
    printf(clientId, cmBuf);
    printf(clientId, "...REBOOTING...\n> ");
    memset(cmBuf, 0, sizeof(cmBuf));
    snprintf(cmBuf, sizeof(cmBuf), "%s\t%s", config.tmpBuf, config.tmpBuf2);
    config.saveWifiFromNextion(cmBuf);
    return;
  }
  if (strcmp(str, "wifi.status") == 0 || strcmp(str, "status") == 0) {
    printf(clientId, "#WIFI.STATUS#\nStatus:\t\t%d\nMode:\t\t%s\nIP:\t\t%s\nMask:\t\t%s\nGateway:\t%s\nRSSI:\t\t%d dBm\n##WIFI.STATUS#\n> ", 
      WiFi.status(), WiFi.getMode()==WIFI_STA?"WIFI_STA":"WIFI_AP", 
      WiFi.getMode()==WIFI_STA?config.ipToStr(WiFi.localIP()):config.ipToStr(WiFi.softAPIP()),
      WiFi.getMode()==WIFI_STA?config.ipToStr(WiFi.subnetMask()):"255.255.255.0",
      WiFi.getMode()==WIFI_STA?config.ipToStr(WiFi.gatewayIP()):config.ipToStr(WiFi.softAPIP()),
      WiFi.RSSI()
    );
    return;
  }
  if (strcmp(str, "wifi.rssi") == 0 || strcmp(str, "rssi") == 0) {
    printf(clientId, "#WIFI.RSSI#\t%d dBm\n> ", WiFi.RSSI());
    return;
  }
  if (strcmp(str, "sys.heap") == 0 || strcmp(str, "heap") == 0) {
    //printf(clientId, "Free heap:\t%d bytes\n> ", xPortGetFreeHeapSize());
    printHeapFragmentationInfo(clientId);
    return;
  }
  if (strcmp(str, "sys.config") == 0 || strcmp(str, "config") == 0) {
    config.bootInfo();
    //printf(clientId, "Free heap:\t%d bytes\n> ", xPortGetFreeHeapSize());
    printHeapFragmentationInfo(clientId);
    return;
  }
  if (strcmp(str, "wifi.discon") == 0 || strcmp(str, "discon") == 0 || strcmp(str, "disconnect") == 0) {
    printf(clientId, "#WIFI.DISCON#\tdisconnected...\n> ");
    WiFi.disconnect();
    return;
  }
  telnet.printf(clientId, "##CMD_ERROR#\tunknown command <%s>\n> ", str);
}
