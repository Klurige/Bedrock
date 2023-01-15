#ifndef __BEDROCK_MQTT_H__
#define __BEDROCK_MQTT_H__

extern char mqttClientId[];  // systemName-deviceName-macAddress.

bool mqttSetup();
void mqttLoop();
bool mqttSubscribe(const char* const topic, void (*funptr)(const uint8_t* const, unsigned int));
void mqttUnsubscribe(const char* const topic);
bool mqttPublish(const char* const topic, const uint8_t* const payload, unsigned int length, bool isRetain);
#endif  // __BEDROCK_MQTT_H__