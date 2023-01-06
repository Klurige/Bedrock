#ifndef __BEDROCK_MQTT_H__
#define __BEDROCK_MQTT_H__

extern char mqttClientId[];  // targetName-deviceName-macAddress.

bool mqttSetup();
void mqttLoop();
boolean mqttSubscribe(const char* const topic, void (*funptr)(const uint8_t* const, unsigned int));
void mqttUnsubscribe(const char* const topic);
void mqttPublish(const char* const topic, const uint8_t* const payload, unsigned int length, boolean isRetain);
#endif  // __BEDROCK_MQTT_H__