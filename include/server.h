#ifndef __BEDROCK_SERVER_H__
#define __BEDROCK_SERVER_H__

#include <Arduino.h>

bool serverSetup();
void serverLoop();
void serverRegister(const char* const path, const char* const arg, const char* const (*callback)(const char* const param));
void serverSendEvent(const char* const label, const char* const message);
long serverGetEpochTime();
String serverGetFormattedTime();
#endif  // __BEDROCK_SERVER_H__
