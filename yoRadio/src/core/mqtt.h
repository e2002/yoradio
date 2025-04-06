#ifndef mqtt_h
#define mqtt_h
#include "options.h"
#ifdef MQTT_ROOT_TOPIC
//#if __has_include("../../mqttoptions.h")
//#include "../../mqttoptions.h"
#include "../async-mqtt-client/AsyncMqttClient.h"


void mqttInit();
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void mqttPublishOnline();
void mqttPublishStatus();
void mqttPublishPlaylist();
void mqttPublishVolume();

#endif // #ifdef MQTT_ROOT_TOPIC


#endif
