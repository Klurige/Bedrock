
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <mqtt.h>
#include <params.h>

#if defined(BEDROCK_DEBUG_MQTT) && BEDROCK_DEBUG_MQTT
#define BEDROCK_LOG
#endif
#include <log.h>

#ifndef BEDROCK_MQTT_MAX_SUBSCRIPTIONS
#error "BEDROCK_MQTT_MAX_SUBSCRIPTIONS must be defined at compile time (platformio.ini)."
#endif
typedef struct subscription {
    void (*funptr)(const uint8_t *const, unsigned int);
    const char *topic;
} subscription;

subscription subscriptions[BEDROCK_MQTT_MAX_SUBSCRIPTIONS];

#define LENGTH 100          // class length, name length, mac addr(12), 2 delimiters, terminator.
char mqttClientId[LENGTH];  // length of targetName-deviceName-macAddress.
char expandedTopic[LENGTH];
WiFiClient espClient;
PubSubClient client(espClient);

void createClientId(const uint8_t *macAddress) {
    BEDROCK_DEBUG("Creating client id...");
    char devName[BEDROCK_VALUE_MAX_LENGTH];
    strncpy_P(devName, paramMqttName, BEDROCK_VALUE_MAX_LENGTH);
    const char *const devClass = paramsGetTargetName();
    BEDROCK_DEBUG("Got devClass: %s", devClass);
    int i = 0;
    int j = 0;

    int devClassLen = strlen(devClass);
    int devNameLen = strlen(devName);
    while (i < devClassLen) {
        mqttClientId[i++] = devClass[j++];
    }
    mqttClientId[i++] = '-';
    j = 0;
    while (i < devNameLen + devClassLen + 1) {
        mqttClientId[i++] = devName[j++];
    }
    mqttClientId[i++] = '-';
    j = 0;
    while (i < devNameLen + devClassLen + 1 + 12 + 1) {
        char digit = macAddress[j] / 16 + '0';
        if (digit > '9') {
            digit = digit - '0' + 'A' - 10;
        }
        mqttClientId[i++] = digit;
        digit = macAddress[j++] % 16 + '0';
        if (digit > '9') {
            digit = digit - '0' + 'A' - 10;
        }
        mqttClientId[i++] = digit;
    }
    mqttClientId[i] = '\0';
    BEDROCK_DEBUG("Mac Address: %02x:%02x:%02x:%02x:%02x:%02x", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
    BEDROCK_DEBUG("Client ID: %s", mqttClientId);
}

void createTopic(const char *tag, const char *id, char *topic) {
    int i = 0;
    int j = 0;
    int idLen = strlen(id);
    int tagLen = strlen(tag);

    while (i < idLen) {
        topic[i++] = id[j++];
    }
    topic[i++] = '/';
    j = 0;
    while (i < tagLen + idLen + 1) {
        topic[i++] = tag[j++];
    }
    topic[i] = '\0';
}

boolean mqttSubscribe(const char *const topic, void (*funptr)(const uint8_t *const, unsigned int)) {
    for (int i = 0; i < BEDROCK_MQTT_MAX_SUBSCRIPTIONS; i++) {
        if (subscriptions[i].topic != nullptr && strcmp(topic, subscriptions[i].topic) == 0) {
            return false;
        }
    }
    for (int i = 0; i < BEDROCK_MQTT_MAX_SUBSCRIPTIONS; i++) {
        if (subscriptions[i].funptr == nullptr) {
            subscriptions[i].topic = topic;
            subscriptions[i].funptr = funptr;
            if (client.connected()) {
                createTopic(topic, mqttClientId, expandedTopic);
                client.subscribe(expandedTopic);
            }

            return true;
        }
    }
    BEDROCK_ERROR("Failed to subscribe. (Please check BEDROCK_MQTT_MAX_SUBSCRIPTIONS)");
    return false;
}

void mqttUnsubscribe(const char *const topic) {
    BEDROCK_DEBUG("Unsubscribing to %s", topic);

    for (int i = 0; i < BEDROCK_MQTT_MAX_SUBSCRIPTIONS; i++) {
        if (subscriptions[i].topic != nullptr && strcmp(topic, subscriptions[i].topic) == 0) {
            client.unsubscribe(topic);
            subscriptions[i].topic = nullptr;
            subscriptions[i].funptr = nullptr;
            if (client.connected()) {
                createTopic(topic, mqttClientId, expandedTopic);
                client.unsubscribe(expandedTopic);
            }
        }
    }
}

void mqttPublish(const char *const topic, const uint8_t *const payload, unsigned int length, boolean isRetain) {
    if (client.connected()) {
        createTopic(topic, mqttClientId, expandedTopic);
        BEDROCK_DEBUG("Publishing: topic:%s", expandedTopic);
        client.publish(expandedTopic, payload, length, isRetain);
    }
}

void topicReceived(char *topic, byte *payload, unsigned int length) {
    BEDROCK_DEBUG("Message arrived in topic: %s", topic);
    char *delim = strrchr(topic, '/');
    char *t = delim + 1;
    for (int i = 0; i < BEDROCK_MQTT_MAX_SUBSCRIPTIONS; i++) {
        if (subscriptions[i].topic != nullptr && strcmp(t, subscriptions[i].topic) == 0) {
            subscriptions[i].funptr(payload, length);
            return;
        }
    }
}

long lastReconnectAttempt = 0;

boolean reconnect() {
    char username[BEDROCK_VALUE_MAX_LENGTH];
    char password[BEDROCK_VALUE_MAX_LENGTH];
    strncpy_P(username, paramMqttUser, BEDROCK_VALUE_MAX_LENGTH);
    strncpy_P(password, paramMqttPassword, BEDROCK_VALUE_MAX_LENGTH);
    if (client.connect(mqttClientId, username, password)) {
        BEDROCK_DEBUG("mqtt broker connected");
        for (int i = 0; i < BEDROCK_MQTT_MAX_SUBSCRIPTIONS; i++) {
            if (subscriptions[i].topic != nullptr) {
                createTopic(subscriptions[i].topic, mqttClientId, expandedTopic);
                client.subscribe(expandedTopic);
            }
        }
    } else {
        BEDROCK_ERROR("mqtt broker failed with state %d", client.state());
    }
    return client.connected();
}

bool mqttSetup() {
    static char mqttServer[BEDROCK_VALUE_MAX_LENGTH];

    uint8_t macAddress[6];

    WiFi.macAddress(macAddress);

    createClientId(macAddress);

    strncpy_P(mqttServer, paramMqttServer, BEDROCK_VALUE_MAX_LENGTH);
    client.setServer(mqttServer, paramMqttPort);
    client.setCallback(topicReceived);
    return true;
}

void mqttLoop() {
    if (!client.connected()) {
        BEDROCK_DEBUG("The client %s connects to mosquitto mqtt broker", mqttClientId);
        long now = millis();
        if (now - lastReconnectAttempt > 1000) {
            lastReconnectAttempt = now;
            reconnect();
        }
    } else {
        client.loop();
    }
}
