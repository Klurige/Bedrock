#include <Arduino.h>
#include <files.h>
#include <params.h>

#if defined(BEDROCK_DEBUG_MISC) && BEDROCK_DEBUG_MISC
#define BEDROCK_LOG
#endif
#include <log.h>

/**
 * The following list of parameters must match
 * the list found in params.h
 */

const char* paramWifiNetwork;
const char* paramSystemName;
const char* paramDeviceName;
const char* paramWifiPassword;
const char* paramMqttServer;
int paramMqttPort;
const char* paramMqttUser;
const char* paramMqttPassword;
const char* paramWebUser;
const char* paramWebPassword;
const char* paramNtpServerAddress;
const char* paramVersion;
const char* paramBuildTimestamp;
const char* paramPublicKey;

void paramsInitialise(const char* const params[][2]) {
    BEDROCK_DEBUG("Reading params");
    int paramIndex = 0;
    char key[BEDROCK_KEY_MAX_LENGTH];
    while (paramIndex >= 0) {
        strncpy_P(key, (char*)pgm_read_dword(&(params[paramIndex][0])), BEDROCK_KEY_MAX_LENGTH);
        if (strlen(key) == 0) {
            BEDROCK_DEBUG("Found end of param list. Got %d params.", paramIndex);
            break;
        } else {
            BEDROCK_DEBUG("Got a parameter: %s", key);
            switch (key[0]) {
                case 'N':
                    paramNtpServerAddress = params[paramIndex][1];
                    break;
                case 'V':
                    paramVersion = params[paramIndex][1];
                    break;
                case 'B':
                    paramBuildTimestamp = params[paramIndex][1];
                    break;
                case 'P':
                    paramPublicKey = params[paramIndex][1];
                    break;
                case 'S':
                    paramSystemName = params[paramIndex][1];
                    break;
                case 'D':
                    paramDeviceName = params[paramIndex][1];
                    break;
                case 'M':
                    switch (key[7]) {
                        case 'v':
                            paramMqttServer = params[paramIndex][1];
                            break;
                        case 't':
                            strncpy_P(key, params[paramIndex][1], BEDROCK_KEY_MAX_LENGTH);
                            paramMqttPort = atoi(key);
                            break;
                        case 'r':
                            paramMqttUser = params[paramIndex][1];
                            break;
                        case 's':
                            paramMqttPassword = params[paramIndex][1];
                            break;
                        default:
                            BEDROCK_ERROR("Unknown MQTT key: %s", key);
                    }
                    break;
                case 'W':
                    switch (key[4]) {
                        case 'N':
                            paramWifiNetwork = params[paramIndex][1];
                            break;
                        case 'P':
                            paramWifiPassword = params[paramIndex][1];
                            break;
                        case 's':
                            paramWebUser = params[paramIndex][1];
                            break;
                        case 'a':
                            paramWebPassword = params[paramIndex][1];
                            break;
                        default:
                            BEDROCK_ERROR("Unknown Wifi/Web key: %s", key);
                    }
                    break;
                default:
                    BEDROCK_ERROR("Unknown key: %s", key);
            }
        }
        paramIndex++;
    }
}

void getParamFromStorageOrConfig(const char* const paramName, const char* const configParam, char* const param) {
    char storedName[BEDROCK_KEY_MAX_LENGTH];

    filesGetKeyValue(paramName, storedName, BEDROCK_KEY_MAX_LENGTH);
    if (storedName[0] == '\0') {
        if (param[0] == '\0') {
            strncpy_P(param, configParam, BEDROCK_VALUE_MAX_LENGTH);
        }
    } else {
        if (strncmp(param, storedName, BEDROCK_KEY_MAX_LENGTH) != 0) {
            strncpy(param, storedName, BEDROCK_KEY_MAX_LENGTH);
        }
    }
    param[BEDROCK_KEY_MAX_LENGTH - 1] = '\0';
}

size_t paramsGetSystemName(char* const param, size_t size) {
    strncpy_P(param, paramSystemName, BEDROCK_VALUE_MAX_LENGTH);
    return strlen(param);
}

size_t paramsGetDeviceName(char* const param, size_t size) {
    getParamFromStorageOrConfig("DeviceName", paramDeviceName, param);
    return strlen(param);
}
