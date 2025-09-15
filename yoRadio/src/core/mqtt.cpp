#include "options.h"
#ifdef MQTT_ROOT_TOPIC

#include "config.h"
#include "mqtt.h"
#include "WiFi.h"
#include "player.h"
#include "commandhandler.h"

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
char topic[100], status[BUFLEN*2];

void connectToMqtt() {
  mqttClient.connect();
}

void mqttInit() {
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  if(strlen(MQTT_USER)>0) mqttClient.setCredentials(MQTT_USER, MQTT_PASS);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectToMqtt();
}

void zeroBuffer(){ memset(topic, 0, sizeof(topic)); memset(status, 0, sizeof(status)); }

void onMqttConnect(bool sessionPresent) {
  zeroBuffer();
  sprintf(topic, "%s%s", MQTT_ROOT_TOPIC, "command");
  mqttClient.subscribe(topic, 2);
  mqttPublishStatus();
  mqttPublishVolume();
  mqttPublishPlaylist();
}

void mqttPublishStatus() {
  if(mqttClient.connected()){
    zeroBuffer();
    sprintf(topic, "%s%s", MQTT_ROOT_TOPIC, "status");
    char name[BUFLEN/2];
    char title[BUFLEN/2];
    config.escapeQuotes(config.station.name, name, sizeof(name)-10);
    config.escapeQuotes(config.station.title, title, sizeof(title)-10);
    sprintf(status, "{\"status\": %d, \"station\": %d, \"name\": \"%s\", \"title\": \"%s\", \"on\": %d}", 
            player.status()==PLAYING?1:0, config.lastStation(), name, title, config.store.dspon);
    mqttClient.publish(topic, 0, true, status);
  }
}

void mqttPublishPlaylist() {
  if(mqttClient.connected()){
    zeroBuffer();
    sprintf(topic, "%s%s", MQTT_ROOT_TOPIC, "playlist");
    sprintf(status, "http://%s%s", config.ipToStr(WiFi.localIP()), PLAYLIST_PATH);
    mqttClient.publish(topic, 0, true, status);
  }
}

void mqttPublishVolume(){
  if(mqttClient.connected()){
    zeroBuffer();
    char vol[5];
    memset(vol, 0, 5);
    sprintf(topic, "%s%s", MQTT_ROOT_TOPIC, "volume");
    sprintf(vol, "%d", config.store.volume);
    mqttClient.publish(topic, 0, true, vol);
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  if (len == 0) return;
  if(len<20){
    char buf[len+1];
    strncpy(buf, payload, len);
    buf[len]='\0';
    if(cmd.exec(buf, "")) return;
    if (strcmp(buf, "turnoff") == 0) {
      uint8_t sst = config.store.smartstart;
      config.setDspOn(0);
      player.sendCommand({PR_STOP, 0});
      delay(100);
      config.saveValue(&config.store.smartstart, sst);
      return;
    }
    if (strcmp(buf, "turnon") == 0) {
      config.setDspOn(1);
      if (config.store.smartstart == 1) player.sendCommand({PR_PLAY, config.lastStation()});
      return;
    }
    int volume;
    if ( sscanf(buf, "vol %d", &volume) == 1) {
      if (volume < 0) volume = 0;
      if (volume > 254) volume = 254;
      player.setVol(volume);
      return;
    }
    int sb;
    if (sscanf(buf, "play %d", &sb) == 1 ) {
      if (sb < 1) sb = 1;
      uint16_t cs = config.playlistLength();
      if (sb >= cs) sb = cs;
      player.sendCommand({PR_PLAY, (uint16_t)sb});
      return;
    }
  }else{
    if(len>MQTT_BURL_SIZE) return;
    strncpy(player.burl, payload, len);
    player.burl[len]='\0';
    player.sendCommand({PR_BURL, 0});
    return;
  }
  /*if (strstr(buf, "http")==0){
    if(len+1>sizeof(player.burl)) return;
    strlcpy(player.burl, payload, len+1);
    return;
  }*/
}

#endif // #ifdef MQTT_ROOT_TOPIC
