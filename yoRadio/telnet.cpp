#include <stdarg.h>
#include "WiFi.h"

#include "config.h"
#include "telnet.h"
#include "player.h"
#include "display.h"

Telnet telnet;

bool Telnet::_isIPSet(IPAddress ip) {
  return ip.toString() == "0.0.0.0";
}

bool Telnet::begin() {
  if (WiFi.status() == WL_CONNECTED || _isIPSet(WiFi.softAPIP())) {
    server.begin();
    server.setNoDelay(true);
    Serial.printf("Ready! Use 'telnet %s 23' to connect\n", WiFi.localIP().toString().c_str());
    return true;
  } else {
    return false;
  }
}

void Telnet::stop() {
  server.stop();
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

void Telnet::loop() {
  uint8_t i;
  if (WiFi.status() == WL_CONNECTED) {
    if (server.hasClient()) {
      for (i = 0; i < MAX_TLN_CLIENTS; i++) {
        if (!clients[i] || !clients[i].connected()) {
          if (clients[i]) {
            clients[i].stop();
          }
          clients[i] = server.available();
          if (!clients[i]) Serial.println("available broken");
          on_connect(clients[i].remoteIP().toString().c_str(), i);
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
  yield();
}

void Telnet::print(const char *buf) {
  for (int id = 0; id < MAX_TLN_CLIENTS; id++) {
    if (clients[id] && clients[id].connected()) {
      print(id, buf);
    }
  }
  Serial.print(buf);
}

void Telnet::print(byte id, const char *buf) {
  if (clients[id] && clients[id].connected()) {
    clients[id].print(buf);
  }
}

void Telnet::printf(const char *format, ...) {
  char buf[MAX_PRINTF_LEN];
  va_list args;
  va_start (args, format );
  vsnprintf(buf, MAX_PRINTF_LEN, format, args);
  va_end (args);
  for (int id = 0; id < MAX_TLN_CLIENTS; id++) {
    if (clients[id] && clients[id].connected()) {
      clients[id].print(buf);
    }
  }
  Serial.print(buf);
}

void Telnet::printf(byte id, const char *format, ...) {
  if (clients[id] && clients[id].connected()) {
    va_list argptr;
    va_start(argptr, format);
    char *szBuffer = 0;
    const size_t nBufferLength = vsnprintf(szBuffer, 0, format, argptr) + 1;
    if (nBufferLength == 1) return;
    szBuffer = (char *) malloc(nBufferLength);
    if (! szBuffer) return;
    vsnprintf(szBuffer, nBufferLength, format, argptr);
    va_end(argptr);
    clients[id].print(szBuffer);
    free(szBuffer);
  }
}

void Telnet::on_connect(const char* str, byte clientId) {
  Serial.printf("Telnet: [%d] %s connected\n", clientId, str);
  print(clientId, "\nWelcome to ёRadio!\n(Use ^] + q  to disconnect.)\n> ");
}

void Telnet::info() {
  byte volume;
  telnet.printf("##CLI.INFO#\n");
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%S+03:00", &display.timeinfo);
  telnet.printf("##SYS.DATE#: %s\n", timeStringBuff); //TODO timezone offset
  telnet.printf("##CLI.NAMESET#: %d %s\n", config.store.lastStation, config.station.name);
  if (player.mode == PLAYING) {
    telnet.printf("##CLI.META#: %s\n",  config.station.title);
  }
  telnet.printf("##CLI.VOL#: %d\n", config.store.volume);
  if (player.mode == PLAYING) {
    telnet.printf("##CLI.PLAYING#\n");
  } else {
    telnet.printf("##CLI.STOPPED#\n");
  }
  telnet.printf("> ");
}

void Telnet::on_input(const char* str, byte clientId) {
  if (strlen(str) == 0) return;
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
    player.mode = STOPPED;
    display.title("[stopped]");
    info();
    return;
  }
  if (strcmp(str, "cli.start") == 0 || strcmp(str, "start") == 0 || strcmp(str, "cli.play") == 0 || strcmp(str, "play") == 0) {
    player.play(config.store.lastStation);
    return;
  }
  if (strcmp(str, "sys.boot") == 0 || strcmp(str, "boot") == 0 || strcmp(str, "reboot") == 0) {
    ESP.restart();
    return;
  }
  if (strcmp(str, "cli.vol") == 0 || strcmp(str, "vol") == 0) {
    printf(clientId, "##CLI.VOL#: %d\n", config.store.volume);
    return;
  }
  if (strcmp(str, "sys.date") == 0) {
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%S+03:00", &display.timeinfo);
    telnet.printf("##SYS.DATE#: %s\n", timeStringBuff); //TODO timezone offset
    return;
  }
  int volume;
  if (sscanf(str, "vol(%d)", &volume) == 1 || sscanf(str, "cli.vol(\"%d\")", &volume) == 1 || sscanf(str, "vol %d", &volume) == 1) {
    if (volume < 0) volume = 0;
    if (volume > 254) volume = 254;
    player.setVol(volume, false);
    return;
  }

  if (strcmp(str, "cli.audioinfo") == 0 || strcmp(str, "audioinfo") == 0) {
    printf(clientId, "##CLI.AUDIOINFO#: %d\n", config.store.audioinfo>0);
    return;
  }
  byte ainfo;
  if (sscanf(str, "audioinfo(%d)", &ainfo) == 1 || sscanf(str, "cli.audioinfo(\"%d\")", &ainfo) == 1 || sscanf(str, "audioinfo %d", &ainfo) == 1) {
    config.store.audioinfo = ainfo>0;
    config.save();
    return;
  }
  if (strcmp(str, "cli.smartstart") == 0 || strcmp(str, "smartstart") == 0) {
    printf(clientId, "##CLI.SMARTSTART#: %d\n", config.store.smartstart);
    return;
  }
  byte sstart;
  if (sscanf(str, "smartstart(%d)", &sstart) == 1 || sscanf(str, "cli.smartstart(\"%d\")", &sstart) == 1 || sscanf(str, "smartstart %d", &sstart) == 1) {
    config.store.smartstart = sstart;
    config.save();
    return;
  }
  if (strcmp(str, "cli.list") == 0 || strcmp(str, "list") == 0) {
    printf(clientId, "#CLI.LIST#\n");
    File file = SPIFFS.open(PLAYLIST_PATH, "r");
    if (!file || file.isDirectory()) {
      return;
    }
    char sName[BUFLEN], sUrl[BUFLEN];
    int sOvol;
    byte c = 1;
    while (file.available()) {
      if (config.parseCSV(file.readStringUntil('\n').c_str(), sName, sUrl, sOvol)) {
        printf(clientId, "#CLI.LISTNUM#: %*d: %s, %s\n", 3, c, sName, sUrl);
        c++;
      }
    }
    printf(clientId, "##CLI.LIST#\n");
    printf(clientId, "> ");
    return;
  }
  if (strcmp(str, "cli.info") == 0 || strcmp(str, "info") == 0) {
    printf(clientId, "##CLI.INFO#\n");
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%S+03:00", &display.timeinfo);
    printf(clientId, "##SYS.DATE#: %s\n", timeStringBuff); //TODO timezone offset
    printf(clientId, "##CLI.NAMESET#: %d %s\n", config.store.lastStation, config.station.name);
    if (player.mode == PLAYING) {
      printf(clientId, "##CLI.META#: %s\n", config.station.title);
    }
    printf(clientId, "##CLI.VOL#: %d\n", config.store.volume);
    if (player.mode == PLAYING) {
      printf(clientId, "##CLI.PLAYING#\n");
    } else {
      printf(clientId, "##CLI.STOPPED#\n");
    }
    printf(clientId, "> ");
    return;
  }

  uint8_t sb;
  if (sscanf(str, "play(%d)", &sb) == 1 || sscanf(str, "cli.play(\"%d\")", &sb) == 1 || sscanf(str, "play %d", &sb) == 1 ) {
    if (sb < 1) sb = 1;
    if (sb >= config.store.countStation) sb = config.store.countStation;
    player.play(sb);
    return;
  }
  telnet.printf(clientId, "unknown command: %s\n> ", str);
}
