#ifndef __BEDROCK_PARAMS_H__
#define __BEDROCK_PARAMS_H__

#define BEDROCK_VALUE_MAX_LENGTH 32
#define BEDROCK_KEY_MAX_LENGTH 24

extern const char* paramMqttServer;
extern int paramMqttPort;
extern const char* paramMqttUser;
extern const char* paramMqttPassword;
extern const char* paramVersion;
extern const char* paramBuildTimestamp;
extern const char* paramPublicKey;
extern const char* paramWifiNetwork;
extern const char* paramWifiPassword;
extern const char* paramWebUser;
extern const char* paramWebPassword;
extern const char* paramNtpServerAddress;

void paramsInitialise(const char* const params[][2]);
size_t paramsGetSystemName(char* const param, size_t size);
size_t paramsGetDeviceName(char* const param, size_t size);

#endif  // __BEDROCK_PARAMS_H__
