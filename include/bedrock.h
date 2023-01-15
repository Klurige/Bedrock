#ifndef __BEDROCK_H__
#define __BEDROCK_H__

#include <Arduino.h>

void bedrockInitialise(const char* const params[][2], const char* const files[][2], int maxKeyValueLength);
void bedrockRegisterTask(bool (*taskSetup)(), void (*taskLoop)(), int taskPeriodMs);
void bedrockRegisterEvent(const char* const path, const char* const arg, const char* const (*callback)(const char* const param));
void bedrockSendEvent(const char* const label, const char* const message);
bool bedrockSubscribe(const char* const topic, void (*funptr)(const uint8_t* const, unsigned int));
bool bedrockPublish(const char* const topic, const uint8_t* const payload, unsigned int length, boolean isRetain);
void bedrockSetup();
void bedrockLoop();
long bedrockEpochTime();

#endif  // __BEDROCK_H__