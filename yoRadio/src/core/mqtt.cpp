#include "mqtt.h"

#ifdef MQTT_ROOT_TOPIC
#include "WiFi.h"

#include "telnet.h"
#include "player.h"
#include "config.h"

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
char topic[140], status[BUFLEN*3], vol[5], buf[20];

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
  memset(topic, 0, 140);
  sprintf(topic, "%s%s", MQTT_ROOT_TOPIC, "connection");
  mqttClient.setWill(topic, 0, MQTT_RETAIN_ONLINE, "offline");
  connectToMqtt();
}

void onMqttConnect(bool sessionPresent) {
  memset(topic, 0, 140);
  sprintf(topic, "%s%s", MQTT_ROOT_TOPIC, "command");
  mqttClient.subscribe(topic, 2);
  mqttPublishOnline();
  mqttPublishStatus();
  mqttPublishVolume();
  mqttPublishPlaylist();
}

void mqttPublishOnline() {
  if(mqttClient.connected()){
    memset(topic, 0, 140);
    memset(status, 0, BUFLEN*3);
    sprintf(topic, "%s%s", MQTT_ROOT_TOPIC, "connection");
    sprintf(status, "%s", "online");
    mqttClient.publish(topic, 0, MQTT_RETAIN_ONLINE, status);
  }
}

void mqttPublishStatus() {
  if(mqttClient.connected()){
    memset(topic, 0, 140);
    memset(status, 0, BUFLEN*3);
    sprintf(topic, "%s%s", MQTT_ROOT_TOPIC, "status");
    sprintf(status, "{\"status\": %d, \"station\": %d, \"name\": \"%s\", \"title\": \"%s\", \"on\": %d}", player.status()==PLAYING?1:0, config.lastStation(), config.station.name, config.station.title, config.store.dspon);
    mqttClient.publish(topic, 0, MQTT_RETAIN, status);
  }
}

void mqttPublishPlaylist() {
  if(mqttClient.connected()){
    memset(topic, 0, 140);
    memset(status, 0, BUFLEN*3);
    sprintf(topic, "%s%s", MQTT_ROOT_TOPIC, "playlist");
    sprintf(status, "http://%s%s", WiFi.localIP().toString().c_str(), PLAYLIST_PATH);
    mqttClient.publish(topic, 0, MQTT_RETAIN, status);
  }
}

void mqttPublishVolume(){
  if(mqttClient.connected()){
    memset(topic, 0, 140);
    memset(vol, 0, 5);
    sprintf(topic, "%s%s", MQTT_ROOT_TOPIC, "volume");
    sprintf(vol, "%d", config.store.volume);
    mqttClient.publish(topic, 0, MQTT_RETAIN, vol);
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  if (len == 0) return;
  memset(buf, 0, 20);
  strlcpy(buf, payload, len+1);
  if (strcmp(buf, "prev") == 0) {
    player.prev();
    return;
  }
  if (strcmp(buf, "next") == 0) {
    player.next();
    return;
  }
  if (strcmp(buf, "toggle") == 0) {
    player.toggle();
    return;
  }
  if (strcmp(buf, "stop") == 0) {
    player.sendCommand({PR_STOP, 0});
    //telnet.info();
    return;
  }
  if (strcmp(buf, "start") == 0 || strcmp(buf, "play") == 0) {
    player.sendCommand({PR_PLAY, config.lastStation()});
    return;
  }
  if (strcmp(buf, "boot") == 0 || strcmp(buf, "reboot") == 0) {
    ESP.restart();
    return;
  }
  if (strcmp(buf, "volm") == 0) {
    player.stepVol(false);
    return;
  }
  if (strcmp(buf, "volp") == 0) {
    player.stepVol(true);
    return;
  }
  if (strcmp(buf, "turnoff") == 0) {
    uint8_t sst = config.store.smartstart;
    config.setDspOn(0);
    player.sendCommand({PR_STOP, 0});
    //telnet.info();
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
    if (sb >= config.store.countStation) sb = config.store.countStation;
    player.sendCommand({PR_PLAY, (uint16_t)sb});
    return;
  }
  if (strstr(buf, "http")==buf){
    if(len+1>sizeof(player.burl)) return;
    strlcpy(player.burl, payload, len+1);
    return;
  }
}

#endif // #ifdef MQTT_ROOT_TOPIC
