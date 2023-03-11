
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

#define MQTT_CLIENTID_LENGTH 2 * BEDROCK_VALUE_MAX_LENGTH + 15                          // class length, name length, mac addr(12), 2 delimiters, terminator.
#define MQTT_EXPANDED_TOPIC_LENGTH MQTT_CLIENTID_LENGTH + BEDROCK_VALUE_MAX_LENGTH + 2  // clientId length plus topic plus delimiter, terminator.
char mqttClientId[MQTT_CLIENTID_LENGTH];                                                // systemName-deviceName-macAddress.
WiFiClient espClient;
PubSubClient client(espClient);

void createClientId(const uint8_t *macAddress) {
    BEDROCK_DEBUG("Creating client id...");
    size_t sysSize = paramsGetSystemName(mqttClientId, MQTT_CLIENTID_LENGTH);
    mqttClientId[sysSize] = '-';
    size_t devSize = paramsGetDeviceName(&mqttClientId[sysSize + 1], MQTT_CLIENTID_LENGTH - sysSize);
    snprintf(&mqttClientId[sysSize + devSize + 1], MQTT_CLIENTID_LENGTH - (sysSize - devSize), "-%02X%02X%02X%02X%02X%02X", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
    BEDROCK_DEBUG("Mac Address: %02x:%02x:%02x:%02x:%02x:%02x", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
    BEDROCK_DEBUG("Client ID: %s", mqttClientId);
}

void createTopic(const char *tag, const char *id, char *topic) {
    strncpy(topic, id, MQTT_EXPANDED_TOPIC_LENGTH);
    size_t idLen = strlen(topic);
    topic[idLen] = '/';
    strncpy(&topic[idLen + 1], tag, MQTT_EXPANDED_TOPIC_LENGTH - idLen);
}

boolean mqttSubscribe(const char *const topic, void (*funptr)(const uint8_t *const, unsigned int)) {
    char expandedTopic[MQTT_EXPANDED_TOPIC_LENGTH];  // systemName-deviceName-macAddress/topic
    if (strlen(topic) >= BEDROCK_VALUE_MAX_LENGTH) {
        BEDROCK_ERROR("Topic is too long, max is %d chars [%s].", BEDROCK_VALUE_MAX_LENGTH, topic);
        return false;
    } else {
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
}

void mqttUnsubscribe(const char *const topic) {
    char expandedTopic[MQTT_EXPANDED_TOPIC_LENGTH];  // systemName-deviceName-macAddress/topic
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

bool mqttPublish(const char *const topic, const uint8_t *const payload, unsigned int length, bool isRetain) {
    char expandedTopic[MQTT_EXPANDED_TOPIC_LENGTH];  // systemName-deviceName-macAddress/topic
    if (strlen(topic) >= BEDROCK_VALUE_MAX_LENGTH) {
        BEDROCK_ERROR("Topic is too long, max is %d chars [%s].", BEDROCK_VALUE_MAX_LENGTH, topic);
        return false;
    } else {
        if (client.connected()) {
            createTopic(topic, mqttClientId, expandedTopic);
            BEDROCK_DEBUG("Publishing: topic: %s", expandedTopic);
            bool isPublished = client.publish(expandedTopic, payload, length, isRetain);
            if (!isPublished) BEDROCK_ERROR("Failed to publish topic: %s", expandedTopic);
            return isPublished;
        } else {
            BEDROCK_ERROR("Failed to publish - not connected.");
            return false;
        }
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
    char expandedTopic[MQTT_EXPANDED_TOPIC_LENGTH];  // systemName-deviceName-macAddress/topic
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
    bool isOk = true;
#if BEDROCK_MQTT_MAX_PACKET_SIZE > 256
    isOk = client.setBufferSize(BEDROCK_MQTT_MAX_PACKET_SIZE);
    if (!isOk) BEDROCK_ERROR("Failed to set MQTT buffer size to %d", BEDROCK_MQTT_MAX_PACKET_SIZE);
#endif
    return isOk;
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
