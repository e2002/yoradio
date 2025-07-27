#ifndef mqtt_h
#define mqtt_h
#include "options.h"

#ifdef MQTT_ROOT_TOPIC
#include "../async-mqtt-client/AsyncMqttClient.h"

void mqttInit();
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void mqttPublishStatus();
void mqttPublishPlaylist();
void mqttPublishVolume();
void zeroBuffer();

#endif // #ifdef MQTT_ROOT_TOPIC


#endif
