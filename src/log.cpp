#include <Arduino.h>
#include <server.h>

#if defined(BEDROCK_LOG_ON) && BEDROCK_LOG_ON
#ifndef BEDROCK_LOG_SIZE
#define BEDROCK_LOG_SIZE 100
#endif
#ifndef BEDROCK_LOG_MESSAGE_LENGTH
#define BEDROCK_LOG_MESSAGE_LENGTH 100
#endif

// Size of log message when printing is log message length + largest timestamp (4294967295) + delimiter
#define BEDROCK_LOG_MESSAGE_PRINT_LENGTH BEDROCK_LOG_MESSAGE_LENGTH + 10 + 2

#define LOG_FORWARD(X) (X < BEDROCK_LOG_SIZE - 1) ? X++ : X = 0
#define LOG_REVERSE(X) (X > 0) ? X-- : X = BEDROCK_LOG_SIZE - 1
static int logHead = 0;
static int logTail = 0;

static int jsonIndex = 0;      // Used as ring buffer walker when exporting as json.
static int jsonStep = 0;       // json statemachine
static uint32_t jsonSkip = 0;  // Skip this and older values.
static int jsonEnd = 0;        // Last log message to include in json export.

typedef struct logMessage {
    uint32_t millis;
    char message[BEDROCK_LOG_MESSAGE_LENGTH];
} logMessage;

logMessage logMessages[BEDROCK_LOG_SIZE];
void logServer(const char* const format, ...) {
    va_list args;
    va_start(args, format);
    if (logTail == logHead && logMessages[logHead].millis != 0) {
        LOG_FORWARD(logTail);
    }
    logMessages[logHead].millis = millis();
    // Never store a log message at 0 ms.
    if (logMessages[logHead].millis == 0) logMessages[logHead].millis = 1;
    vsnprintf(logMessages[logHead].message, BEDROCK_LOG_MESSAGE_LENGTH, format, args);
    va_end(args);

    char buffer[BEDROCK_LOG_MESSAGE_PRINT_LENGTH];
    snprintf(buffer, BEDROCK_LOG_MESSAGE_PRINT_LENGTH, "%" PRIu32 ": %s", logMessages[logHead].millis, logMessages[logHead].message);
    LOG_FORWARD(logHead);

    serverSendEvent("logMessage", buffer);
#if defined(BEDROCK_DEBUG_SERIAL) && BEDROCK_DEBUG_SERIAL
    Serial.printf("%s\n", buffer);
#endif
}

void logSetup() {
    for (logHead = 0; logHead < BEDROCK_LOG_SIZE; logHead++) {
        logMessages[logHead].millis = 0;
        logMessages[logHead].message[0] = '\0';
    }
    logHead = logTail;
#if defined(BEDROCK_DEBUG_SERIAL) && BEDROCK_DEBUG_SERIAL
    Serial.begin(115200);
#endif
}

/**
 * @brief Start json log array.
 * {
 *   "messages":
 *   [
 *      { "timestamp": "12345", "message": "something happened" },
 *      { "timestamp": "12346", "message": "something else happened" },
 *      { "timestamp": "12347", "message": "something happened again" }
 *   ]
 * }
 *
 * @param skip skip all messages older than skip ms.
 * @param buffer
 * @param maxLen
 * @return size_t
 */
void logJsonStart(uint32_t skip) {
    uint32_t oldest = logMessages[logTail].millis;
    jsonStep = 0;
    jsonSkip = skip;
    jsonEnd = 0;
    if (oldest > skip) {
        jsonIndex = logTail;
    } else {
        jsonIndex = logHead;
        LOG_REVERSE(jsonIndex);
        while (logHead != jsonIndex && logMessages[jsonIndex].millis > skip) {
            LOG_REVERSE(jsonIndex);
        }
        LOG_FORWARD(jsonIndex);
    }
}

/**
 * @brief Get the next log message in a json object.
 *
 * @param buffer
 * @param maxLen
 * @return size_t
 */
size_t logJsonNext(char* const buffer, size_t maxLen) {
    size_t len = 0;
    switch (jsonStep) {
        case 0: {
            len = snprintf((char*)buffer, maxLen, "{\n\t\"messages\":\n\t[\n");
            if (jsonIndex == jsonEnd && logMessages[jsonEnd].millis <= jsonSkip) {
                // List is empty.
                jsonStep = 2;
            } else {
                jsonStep = 1;
            }
        } break;
        case 1: {
            len = snprintf(buffer, maxLen, "\t\t{ \"timestamp\": \"%" PRIu32 "\", \"message\": \"%s\" }", logMessages[jsonIndex].millis, logMessages[jsonIndex].message);
            LOG_FORWARD(jsonIndex);
            if (jsonIndex != jsonEnd) {
                len += snprintf((buffer + len), maxLen - len, ",\n");
            } else {
                len += snprintf(((char*)buffer + len), maxLen - len, "\n");
                jsonStep = 2;
            }
        } break;
        case 2: {
            len = snprintf(buffer, maxLen, "\t]\n}\n");
            jsonStep = 3;
        } break;
        default:
            buffer[0] = '\0';
            len = 0;
    }
    return len;
}

#else  // BEDROCK_LOG_ON
void logServer(const char* const format, ...) {}
void logSetup() {}
void logJsonStart() {}
size_t logJsonNext(const char* buffer, size_t maxLen) { return 0; }

#endif  // BEDROCK_LOG_ON
