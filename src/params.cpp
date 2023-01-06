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
const char* paramTargetName;
const char* paramWifiPassword;
const char* paramMqttServer;
int paramMqttPort;
const char* paramMqttName;
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
                case 'T':
                    paramTargetName = params[paramIndex][1];
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
                        case 'e':
                            paramMqttName = params[paramIndex][1];
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

char* customTargetName = NULL;
const char* const paramsGetTargetName() {
    if (customTargetName == NULL) {
        customTargetName = (char*)calloc(BEDROCK_KEY_MAX_LENGTH, 1);
        filesGetKeyValue("targetName", customTargetName, BEDROCK_KEY_MAX_LENGTH);
        if (strlen(customTargetName) == 0) {
            strncpy_P(customTargetName, paramTargetName, BEDROCK_VALUE_MAX_LENGTH);
        }
    }
    return customTargetName;
}
