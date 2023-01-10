#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <LittleFS.h>
#include <NTP.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <base64.h>
#include <files.h>
#include <mqtt.h>
#include <params.h>

#if defined(BEDROCK_DEBUG_WEB) && BEDROCK_DEBUG_WEB
#define BEDROCK_LOG
#endif
#include <log.h>

#ifndef BEDROCK_SSE_MAX_EVENTS
#error "BEDROCK_SSE_MAX_EVENTS must be defined at compile time (platformio.ini)."
#endif

typedef struct sseEventCallback {
    const char* path;
    const char* arg;
    const char* const (*callback)(const char* const param);
} sseEventCallback;

sseEventCallback sseEventCallbacks[BEDROCK_SSE_MAX_EVENTS];
int sseEventCallbackIndex = 0;

// Callbacks for web server events, such as reboot, factory reset, etc.
// A callback is registered when a request is detected, and executed at
// next iteration is serverLoop(). Hardly ever more than one incoming request
// at a time, but is set too handle a few extras just in case.
#define MAX_SERVER_CALLBACKS 8
void (*requestQueue[MAX_SERVER_CALLBACKS])(void);
int requestQueueHead = 0;
int requestQueueTail = 0;

char username[BEDROCK_VALUE_MAX_LENGTH];
char password[BEDROCK_VALUE_MAX_LENGTH];

void initRequestQueue() {
    for (int i = 0; i < MAX_SERVER_CALLBACKS; i++) {
        requestQueue[i] = nullptr;
    }
}

int enqueueRequest(void (*serverRequest)(void)) {
    if (requestQueue[requestQueueHead] == nullptr) {
        requestQueue[requestQueueHead] = serverRequest;
        if (requestQueueHead == MAX_SERVER_CALLBACKS - 1) {
            requestQueueHead = 0;
        } else {
            requestQueueHead++;
        }
        return 200;
    } else {
        BEDROCK_ERROR("Request queue is full. This will not end well!");
        return 400;
    }
}

void (*dequeueRequest(void))(void) {
    void (*serverRequest)(void) = nullptr;
    if (requestQueueHead != requestQueueTail) {
        if (requestQueue[requestQueueTail] != nullptr) {
            serverRequest = requestQueue[requestQueueTail];
            requestQueue[requestQueueTail] = nullptr;
            (requestQueueTail < MAX_SERVER_CALLBACKS - 1) ? requestQueueTail++ : requestQueueTail = 0;
        }
    }
    return serverRequest;
}

AsyncWebServer webServer(80);

AsyncEventSource events("/events");

WiFiUDP ntpUDP;

NTP timeClient(ntpUDP);

// NTP update interval
long lastTime = 0;
#define NTP_UPDATE_DELAY_MS 120000l
String bootTime;

String serverGetFormattedTime() {
    return timeClient.formattedTime("%F %T");
}

long serverGetEpochTime() {
    return timeClient.epoch();
}

void serverSendEvent(const char* const label, const char* const message) {
#ifdef BEDROCK_LOG_ON
    if (strcmp(label, "logMessage") != 0) {
        BEDROCK_DEBUG("Sending event. label: %s message: %s", label, message);
    }
#endif
    events.send(message, label, millis());
}

void authenticate(AsyncWebServerRequest* request) {
    while (!request->authenticate(username, password)) {
        BEDROCK_DEBUG("Requesting authentication");
        request->requestAuthentication();
    }
}

bool isAuthenticated(AsyncWebServerRequest* request) {
    if (!request->authenticate(username, password)) {
        request->requestAuthentication();
        return false;
    } else {
        return true;
    }
}

String getTargetName() {
    char buffer[2 * BEDROCK_VALUE_MAX_LENGTH];
    size_t sysSize = paramsGetSystemName(buffer, BEDROCK_VALUE_MAX_LENGTH);
    buffer[sysSize] = '-';
    paramsGetDeviceName(&buffer[sysSize + 1], BEDROCK_VALUE_MAX_LENGTH);
    return String(buffer);
}

String templateProcessor(const String& var) {
    char buf[BEDROCK_VALUE_MAX_LENGTH];
    if (var == F("VERSION")) return FPSTR(paramVersion);
    if (var == F("BUILD_TIMESTAMP")) return FPSTR(paramBuildTimestamp);
    if (var == F("BOOT_TIME")) return bootTime;
    if (var == F("CURRENT_TIME")) return serverGetFormattedTime();
    if (var == F("TARGETNAME")) return getTargetName();
    if (var == F("SYSTEMNAME")) {
        paramsGetSystemName(buf, BEDROCK_VALUE_MAX_LENGTH);
        return String(buf);
    }
    if (var == F("DEVICENAME")) {
        paramsGetDeviceName(buf, BEDROCK_VALUE_MAX_LENGTH);
        return String(buf);
    }
    if (var == F("MQTTCLIENTID")) return FPSTR(mqttClientId);
    return String();
}

void notFound(AsyncWebServerRequest* request) {
    String msg = String("Page could not be found: ") + request->url();
    request->send(404, "text/plain", msg);
}

void factoryReset() {
    BEDROCK_DEBUG("Factory reset.");
    filesDeleteEverything();
    ESP.restart();
}

void reboot() {
    BEDROCK_DEBUG("Reboot.");
    ESP.restart();
}

void onUpload(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
    if (isAuthenticated(request)) {
        filesOnUpload(filename, index, data, len, final);
    }
}

#define UPLOADREQUEST_BUFFER_SIZE 256
void uploadRequest(AsyncWebServerRequest* request) {
    if (isAuthenticated(request)) {
        int uploadFileResult;
        char buffer[UPLOADREQUEST_BUFFER_SIZE];
        String filename;
        if (!request->hasParam("image", true, true)) {
            BEDROCK_ERROR("Parameter image is missing.");
            uploadFileResult = VERIFY_CONTENTS_MISSING;
        } else {
            AsyncWebParameter* imageParam = request->getParam("image", true, true);
            filename = imageParam->value();
            BEDROCK_DEBUG("Filename is %s", filename.c_str());

            if (!request->hasParam("signature", true)) {
                BEDROCK_ERROR("Signature is missing.");
                uploadFileResult = VERIFY_SIGNATURE_MISSING;
                // TODO: Remove file.
            } else {
                AsyncWebParameter* signatureParam = request->getParam("signature", true);
                int signLen = b64_decode(buffer, (signatureParam->value()).c_str(), signatureParam->value().length());
                uploadFileResult = filesValidate(filename, buffer, signLen);
                if (uploadFileResult == VERIFY_OK && filename.startsWith("firmware") && filename.endsWith(".bin")) {
                    enqueueRequest(filesApplyFirmware);
                }
            }
        }
        snprintf(buffer, UPLOADREQUEST_BUFFER_SIZE, "{\"upload_result\":\"%d\"}", uploadFileResult);
        request->send(200, "application/json", buffer);
    }
}

int restart(const char* const mode) {
    if (strcmp("reboot", mode) == 0) {
        return enqueueRequest(reboot);
    } else if (strcmp("factoryReset", mode) == 0) {
        return enqueueRequest(factoryReset);
    } else {
        BEDROCK_ERROR("Unknown restart mode: %s", mode);
        return 400;
    }
}

void serverRegister(const char* const path, const char* const arg, const char* const (*callback)(const char* const param)) {
    if (sseEventCallbackIndex == BEDROCK_SSE_MAX_EVENTS) {
        BEDROCK_ERROR("Too many sse events registered. Increase BEDROCK_SSE_MAX_EVENTS");
    } else {
        BEDROCK_DEBUG("path = %s, arg = %s, callback = %lu", path, (arg != NULL)?arg:"NULL", (unsigned long)callback);
        sseEventCallbacks[sseEventCallbackIndex].path = path;
        sseEventCallbacks[sseEventCallbackIndex].arg = arg;
        sseEventCallbacks[sseEventCallbackIndex].callback = callback;

        sseEventCallbackIndex++;

        webServer.on(path, HTTP_GET, [](AsyncWebServerRequest* request) {
            if (isAuthenticated(request)) {
                int cIndex = 0;
                while (cIndex < sseEventCallbackIndex) {
                    if (request->url().equals(sseEventCallbacks[cIndex].path)) {
                        String param;
                        if (sseEventCallbacks[cIndex].arg != nullptr) {
                            param = request->arg(sseEventCallbacks[cIndex].arg);
                            BEDROCK_DEBUG("Requested %s, with %s = %s", sseEventCallbacks[cIndex].path, sseEventCallbacks[cIndex].arg, param.c_str());
                        } else {
                            BEDROCK_DEBUG("Requested %s, with no param", sseEventCallbacks[cIndex].path);
                            param = "";
                        }
                        const char* const result = sseEventCallbacks[cIndex].callback(param.c_str());
                        request->send(200, "text/plain", result);
                        return;
                    }
                    cIndex++;
                }
                request->send(404);
            }
        });
    }
}

bool serverSetup() {
    BEDROCK_INFO("Connecting to %s...", FPSTR(paramWifiNetwork));
    WiFi.begin(FPSTR(paramWifiNetwork), FPSTR(paramWifiPassword));

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    WiFi.setAutoReconnect(true);
    BEDROCK_INFO("Connected. IP: %s", WiFi.localIP().toString().c_str());

    timeClient.ruleDST("CEST", Last, Sun, Mar, 2, 120);  // last sunday in march 2:00, timetone +120min (+1 GMT + 1h summertime offset)
    timeClient.ruleSTD("CET", Last, Sun, Oct, 3, 60);    // last sunday in october 3:00, timezone +60min (+1 GMT)
    char ntpAddress[BEDROCK_VALUE_MAX_LENGTH];
    strncpy_P(ntpAddress, paramNtpServerAddress, BEDROCK_VALUE_MAX_LENGTH);
    timeClient.begin(ntpAddress);

    strncpy_P(username, paramWebUser, BEDROCK_VALUE_MAX_LENGTH);
    strncpy_P(password, paramWebPassword, BEDROCK_VALUE_MAX_LENGTH);

    initRequestQueue();

    events.onConnect([](AsyncEventSourceClient* client) {
        if (client->lastId()) {
            BEDROCK_DEBUG("Client reconnected! Last message ID that it got is: %u", client->lastId());
        }
        // send event with message "hello!", id current millis
        // and set reconnect delay to 1 second
        client->send("hello!", NULL, millis(), 10000);
    });
    webServer.addHandler(&events);

    webServer.onNotFound(notFound);

    webServer.serveStatic("/", LittleFS, "/")
        .setTemplateProcessor(templateProcessor)
        .setDefaultFile("index.html")
        .setAuthentication(username, password);

    webServer.on("/upload", HTTP_POST, uploadRequest, onUpload);

    webServer.on("/logHistory", HTTP_GET, [](AsyncWebServerRequest* request) {
        BEDROCK_DEBUG("logHistory request.");
        if (isAuthenticated(request)) {
            uint32_t skip = 0;
            if (request->hasParam("skip")) {
                AsyncWebParameter* p = request->getParam("skip");
                skip = p->value().toInt();
            }
            logJsonStart(skip);
            AsyncWebServerResponse* response = request->beginChunkedResponse("application/json", [](uint8_t* buffer, size_t maxLen, size_t index) {
                return logJsonNext((char*)buffer, maxLen);
            });
            request->send(response);
        } else {
            request->send(401);
        }
    });

    webServer.on("/params", HTTP_GET, [](AsyncWebServerRequest* request) {
        BEDROCK_DEBUG("params GET request.");
        if (isAuthenticated(request)) {
            int params = request->params();
            if (params == 0) {
                BEDROCK_DEBUG("Empty data.");
                request->send(404);
            } else {
                if (request->hasParam("key")) {
                    AsyncWebParameter* p = request->getParam("key");
                    char value[BEDROCK_VALUE_MAX_LENGTH];
                    filesGetKeyValue(p->value().c_str(), value, BEDROCK_VALUE_MAX_LENGTH);
                    BEDROCK_DEBUG("Got request for testParam. Sending response %s", value);
                    request->send(200, "text/plain", value);
                } else {
                    BEDROCK_DEBUG("Not asking for a parameter.");
                    request->send(404);
                }
            }
        } else {
            BEDROCK_DEBUG("Not authenticated.");
            request->send(401);
        }
    });

    webServer.on("/params", HTTP_PUT, [](AsyncWebServerRequest* request) {
        BEDROCK_DEBUG("params PUT request.");
        if (isAuthenticated(request)) {
            int params = request->params();
            if (params == 0) {
                BEDROCK_DEBUG("No keyValue.");
                request->send(400);
            } else if (params != 1) {
                BEDROCK_DEBUG("Can only handle one keyValue per call.");
                request->send(413);
            } else {
                AsyncWebParameter* p = request->getParam(0);
                if (p->isFile()) {  // p->isPost() is also true
                    BEDROCK_DEBUG("FILE not supported");
                    request->send(403);
                } else if (p->isPost()) {
                    bool isSet = filesSetKeyValue(p->name().c_str(), p->value().c_str());
                    if (isSet) {
                        request->send(200, "text/plain", p->value().c_str());
                    } else {
                        request->send(500);
                    }
                } else {
                    BEDROCK_DEBUG("GET is not supported.");
                    request->send(403);
                }
            }
        } else {
            BEDROCK_DEBUG("Not authenticated.");
            request->send(401);
        }
    });

    webServer.on("/restart", HTTP_GET, [](AsyncWebServerRequest* request) {
        BEDROCK_DEBUG("restart request.");
        while (!isAuthenticated(request))
            ;
        if (request->params() == 1) {
            AsyncWebParameter* p = request->getParam(0);
            if (!p->isPost()) {
                if (strncmp_P(p->name().c_str(), PSTR("mode"), 4) == 0) {
                    request->send(restart(p->value().c_str()));
                } else {
                    BEDROCK_ERROR("Unknown parameter: %s", p->name().c_str());
                    request->send(400);
                }
            } else {
                BEDROCK_ERROR("Not a GET request: %s", p->name().c_str());
                request->send(400);
            }
        } else {
            BEDROCK_ERROR("Wrong number of params: %d", request->params());
            request->send(400);
        }
    });

    webServer.on("/listContents", HTTP_GET, [](AsyncWebServerRequest* request) {
        BEDROCK_DEBUG("Got a request for file list.");
        while (!isAuthenticated(request))
            ;
        int result = enqueueRequest(filesListContents);
        request->send(result);
    });

    webServer.on("/logout", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(401);
    });

    webServer.begin();

    bootTime = serverGetFormattedTime();
    return true;
}

void serverLoop() {
    void (*callback)(void) = dequeueRequest();
    if (callback != nullptr) {
        callback();
    }

    long currentTime = millis();
    if (currentTime > lastTime + NTP_UPDATE_DELAY_MS) {
        lastTime = currentTime;
        timeClient.update();
    }
}
